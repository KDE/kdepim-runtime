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

#include "datasink.h"

#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>

#include <KDebug>
#include <KLocale>
#include <KUrl>

#include <opensync/opensync.h>
#include <opensync/opensync-data.h>
#include <opensync/opensync-format.h>

using namespace Akonadi;

DataSink::DataSink() :
    SinkBase( GetChanges | Commit | SyncDone )
{
}

bool DataSink::initialize(OSyncPlugin * plugin, OSyncPluginInfo * info, OSyncObjTypeSink *sink, OSyncError ** error)
{
  Q_UNUSED( plugin );
  Q_UNUSED( info );
  Q_UNUSED( error );
  wrapSink( sink );
  return true;
}

Akonadi::Collection DataSink::collection() const
{
  OSyncPluginConfig *config = osync_plugin_info_get_config( pluginInfo() );
  Q_ASSERT( config );

  const char *objtype = osync_objtype_sink_get_name( sink() );

  OSyncPluginResource *res = osync_plugin_config_find_active_resource( config, objtype );
  if ( !res ) {
    error( OSYNC_ERROR_MISCONFIGURATION, i18n("No active resource for type \"%1\" found", objtype ) );
    return Collection();
  }

  const KUrl url = KUrl( osync_plugin_resource_get_url( res ) );
  if ( url.isEmpty() ) {
    error( OSYNC_ERROR_MISCONFIGURATION, i18n("Url for object type \"%1\" is not configured.", objtype ) );
    return Collection();
  }

  return Collection::fromUrl( url );
}

void DataSink::getChanges()
{
  // ### broken in OpenSync, I don't get valid configuration here!
#if 1
  Collection col = collection();
  if ( !col.isValid() )
    return;
#else
  Collection col( 409 );
#endif

  // TODO changes since when??
  // TODO what about deletes since then??

  ItemFetchJob *job = new ItemFetchJob( col );
  job->fetchScope().fetchFullPayload();
  // FIXME give me a real eventloop please!
  if ( !job->exec() ) {
    error( OSYNC_ERROR_IO_ERROR, job->errorText() );
    return;
  }

  OSyncFormatEnv *formatenv = osync_plugin_info_get_format_env( pluginInfo() );
  OSyncObjFormat *format = osync_format_env_find_objformat( formatenv, "vcard30" );

  Item::List items = job->items();
  kDebug() << "retrieved" << items.count() << "items";

  // report changes, should be in a result slot instead
  foreach ( const Item &item, items ) {
    OSyncError *error = 0;
    OSyncChange *change = osync_change_new( &error );
    if ( !change ) {
      warning( error );
      continue;
    }

    osync_change_set_uid( change, QByteArray::number( item.id() ) );
    osync_change_set_changetype( change, OSYNC_CHANGE_TYPE_MODIFIED ); // TODO

    QByteArray payloadData = item.payloadData();
    error = 0;
    kDebug() << item.id() << payloadData;
    OSyncData *odata = osync_data_new( payloadData.data(), payloadData.size(), format, &error );
    if ( !odata ) {
      osync_change_unref( change );
      warning( error );
      continue;
    }
    osync_data_set_objtype( odata, osync_objtype_sink_get_name( sink() ) );
    osync_change_set_data( change, odata );
    osync_data_unref( odata );

    osync_context_report_change( context(), change );
    osync_change_unref( change );
  }

  success();
}

void DataSink::commit(OSyncChange * change)
{
  kDebug() << change;
  success();
}

void DataSink::syncDone()
{
  kDebug();
  success();
}

#include "datasink.moc"
