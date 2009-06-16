/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "akonadisink.h"
#include "datasink.h"

#include <akonadi/control.h>
#include <akonadi/collection.h>
#include <akonadi/collectionfetchjob.h>
#include <akonadi/mimetypechecker.h>

#include <kabc/addressee.h>

#include <KComponentData>
#include <KDebug>
#include <KUrl>

#include <QCoreApplication>
#include <QStringList>

#include <opensync/opensync.h>
#include <opensync/opensync-plugin.h>
#include <opensync/opensync-format.h>

static KComponentData *kcd = 0;
static QCoreApplication *app = 0;

static int fakeArgc = 0;
static char** fakeArgv = 0;

extern "C"
{

static void* akonadi_initialize(OSyncPlugin *plugin, OSyncPluginInfo *info, OSyncError **error)
{
  osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, plugin, info, error);

  if ( !app )
    app = new QCoreApplication( fakeArgc, fakeArgv );
  if ( !kcd )
    kcd = new KComponentData( "akonadi_opensync" );

  // main sink
  AkonadiSink *mainSink = new AkonadiSink();
  if ( !mainSink->initialize( plugin, info, error ) ) {
    delete mainSink;
    osync_trace(TRACE_EXIT_ERROR,  "%s: NULL", __func__);
    return 0;
  }

  // object type sinks
  const int numobjs = osync_plugin_info_num_objtypes( info );
  for ( int i = 0; i < numobjs; ++i ) {
    OSyncObjTypeSink *sink = osync_plugin_info_nth_objtype( info, i );
    QString sinkName( osync_objtype_sink_get_name( sink ) );
    kDebug() << "###" << i << sinkName;

    DataSink *ds;
    if( sinkName == "event" )
      ds = new DataSink( DataSink::Calendar );
    else if( sinkName == "contact" )
      ds = new DataSink( DataSink::Contacts );

    if ( !ds->initialize( plugin, info, sink, error ) ) {
      delete ds;
      delete mainSink;
      osync_trace(TRACE_EXIT_ERROR, "%s: NULL", __func__);
      return 0;
    }
  }

  osync_trace(TRACE_EXIT, "%s: %p", __func__, mainSink);
  return mainSink;
}

static osync_bool akonadi_discover(void *userdata, OSyncPluginInfo *info, OSyncError **error)
{
  osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, userdata, info, error);
  kDebug();

  if ( !Akonadi::Control::start() )
    return false;

  // fetch all akonadi collections
  Akonadi::CollectionFetchJob *job = new Akonadi::CollectionFetchJob(
      Akonadi::Collection::root(), Akonadi::CollectionFetchJob::Recursive );
  if ( !job->exec() )
    return false;

  Akonadi::Collection::List cols = job->collections();
  kDebug() << "found" << cols.count() << "collections";

  OSyncPluginConfig *config = osync_plugin_info_get_config( info );

  Akonadi::MimeTypeChecker contactMimeChecker;
  contactMimeChecker.addWantedMimeType( KABC::Addressee::mimeType() );
  Akonadi::MimeTypeChecker calendarMimeTypeChecker;
  calendarMimeTypeChecker.addWantedMimeType( QLatin1String( "text/calendar" ) );

  const int num_objtypes = osync_plugin_info_num_objtypes(info);
  for ( int i = 0; i < num_objtypes; ++i ) {
    OSyncObjTypeSink *sink = osync_plugin_info_nth_objtype(info, i);
    foreach ( const Akonadi::Collection &col, cols ) {
      kDebug() << "creating resource for" << col.name() << col.contentMimeTypes();
//      if ( !contactMimeChecker.isWantedCollection( col ) ) // ### TODO
//         continue;
      if( col.contentMimeTypes().isEmpty() )
        continue;

      OSyncPluginResource *res = osync_plugin_resource_new( error );
      // TODO error handling?
      osync_plugin_resource_enable( res, TRUE );
      osync_plugin_resource_set_name( res, col.name().toUtf8() ); // TODO: full path instead of the name
      osync_plugin_resource_set_objtype( res, osync_objtype_sink_get_name( sink ) );
      osync_plugin_resource_set_url( res, col.url().url().toLatin1() );

      QString formatName;
      if( calendarMimeChecker.isWantedCollection( col ) )
        formatName = "vevent20";
      else if( contactMimeChecker.isWantedCollection( col ) )
        formatName = "vcard30";
      else
        continue; // if the collection is not calendar or contact one, skip it

      /*OSyncFormatEnv *formatenv = osync_plugin_info_get_format_env( info );
      OSyncObjFormat *format = osync_format_env_find_objformat( formatenv, formatName.toLatin1() );
      osync_plugin_resource_set_objformat( res, format );*/

      osync_plugin_resource_add_objformat_sink( res, osync_objformat_sink_new( formatName.toLatin1(), error ) );

      osync_plugin_config_add_resource( config, res );
    }
    osync_objtype_sink_set_available( sink, TRUE );
  }

  osync_trace(TRACE_EXIT, "%s", __func__);
  return TRUE;
}

static void akonadi_finalize(void *userdata)
{
  osync_trace(TRACE_ENTRY, "%s(%p)", __func__, userdata);
  kDebug();
  AkonadiSink *sink = reinterpret_cast<AkonadiSink*>( userdata );
  delete sink;
  delete kcd;
  kcd = 0;
  delete app;
  app = 0;
  osync_trace(TRACE_EXIT, "%s", __func__);
}

KDE_EXPORT osync_bool get_sync_info(OSyncPluginEnv *env, OSyncError **error)
{
  osync_trace(TRACE_ENTRY, "%s(%p)", __func__, env);

  OSyncPlugin *plugin = osync_plugin_new( error );
  if ( !plugin ) {
    osync_trace(TRACE_EXIT_ERROR, "%s: Unable to register: %s", __func__, osync_error_print(error));
    return FALSE;
  }

  osync_plugin_set_name(plugin, "akonadi-sync");
  osync_plugin_set_longname(plugin, "Akonadi");
  osync_plugin_set_description(plugin, "Plugin to synchronize with Akonadi");
//   osync_plugin_set_config_type(plugin, OSYNC_PLUGIN_NO_CONFIGURATION);
  osync_plugin_set_start_type(plugin, OSYNC_START_TYPE_PROCESS);

  osync_plugin_set_initialize(plugin, akonadi_initialize);
  osync_plugin_set_finalize(plugin, akonadi_finalize);
  osync_plugin_set_discover(plugin, akonadi_discover);

  osync_plugin_env_register_plugin(env, plugin);
  osync_plugin_unref(plugin);

  osync_trace(TRACE_EXIT, "%s", __func__);
  return TRUE;
}

KDE_EXPORT int get_version(void)
{
  return 1;
}

}// extern "C"
