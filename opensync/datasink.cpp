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


#include <KDebug>
#include <KLocale>
#include <KUrl>

using namespace Akonadi;

DataSink::DataSink( int features ) :
    SinkBase( features )
{
}

DataSink::~DataSink() {
  if( m_hashtable )
    osync_hashtable_unref( m_hashtable );
}

bool DataSink::initialize(OSyncPlugin * plugin, OSyncPluginInfo * info, OSyncObjTypeSink *sink, OSyncError ** error)
{
  Q_UNUSED( plugin );
  Q_UNUSED( info );
  Q_UNUSED( error );
  
  bool enabled = osync_objtype_sink_is_enabled( sink );
  if( ! enabled )
    return false;

  QString configdir( osync_plugin_info_get_configdir( info ) );
  QString hashfile = QString("%1/%2-hash.db").arg( configdir, osync_objtype_sink_get_name( sink ) );

  m_hashtable = osync_hashtable_new( hashfile.toUtf8(), osync_objtype_sink_get_name(sink), error );
  if( ! m_hashtable ) {
    osync_trace(TRACE_EXIT_ERROR, "%s: %s", __PRETTY_FUNCTION__, osync_error_print( error ) );
    return false;
  }

  wrapSink( sink );
  return true;
}

Akonadi::Collection DataSink::collection() const
{
  OSyncPluginConfig *config = osync_plugin_info_get_config( pluginInfo() );
  Q_ASSERT( config );

  const char *objtype = osync_objtype_sink_get_name( sink() );

  OSyncPluginResource *res = osync_plugin_config_find_active_resource( config, objtype );
  //kDebug() << objtype;
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

void DataSink::slotItemsReceived( const Item::List &items )
{
    kDebug() << "retrieved" << items.count() << "items";
    Q_FOREACH( const Item& item, items ) {
        reportChange( item );
    }
}

void DataSink::reportChange( const Item& item )
{
kDebug();
}

void DataSink::reportChange( const Item& item, const QString& formatName )
{
  kDebug() << "sinkname:" << osync_objtype_sink_get_name( sink() );

  OSyncFormatEnv *formatenv = osync_plugin_info_get_format_env( pluginInfo() );
  OSyncObjFormat *format = osync_format_env_find_objformat( formatenv, formatName.toLatin1() );

  OSyncError *error = 0;

  if( osync_objtype_sink_get_slowsync( sink() ) ) {
    osync_trace( TRACE_INTERNAL, "Got slow-sync, resetting hashtable" );
    if( ! osync_hashtable_slowsync( m_hashtable, &error ) ) {
      warning( error );
      osync_trace( TRACE_EXIT_ERROR, "%s: %s", __PRETTY_FUNCTION__, osync_error_print( &error ) );
      return;
    }
  }

  OSyncChange *change = osync_change_new( &error );
  if ( !change ) {
    warning( error );
    return;
  }

  osync_change_set_uid( change, QByteArray::number( item.id() ) );

  error = 0;

  OSyncData *odata = osync_data_new( item.payloadData().data(), item.payloadData().size(), format, &error );
  if ( !odata ) {
    osync_change_unref( change );
    warning( error );
    return;
  }

  osync_data_set_objtype( odata, osync_objtype_sink_get_name( sink() ) );

  osync_change_set_data( change, odata );
  osync_data_unref( odata );


  osync_change_set_hash( change, QString::number( item.revision() ).toLatin1() );

  OSyncChangeType changeType = osync_hashtable_get_changetype( m_hashtable, change );
  osync_change_set_changetype( change, changeType );

  osync_hashtable_update_change( m_hashtable, change );

  if ( changeType != OSYNC_CHANGE_TYPE_UNMODIFIED )
    osync_context_report_change( context(), change );

  //osync_change_unref( change ); // if this gets called, we get broken pipes. any ideas?
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
  
  QObject::connect( job, SIGNAL( itemsReceived( const Akonadi::Item::List & ) ), this, SLOT( slotItemsReceived( const Akonadi::Item::List & ) ) );
  QObject::connect( job, SIGNAL( result( KJob * ) ), this, SLOT( slotGetChangesFinished( KJob * ) ) );

  
  // FIXME give me a real eventloop please!
  if ( !job->exec() ) {
    error( OSYNC_ERROR_IO_ERROR, job->errorText() );
    return;
  }
}

void DataSink::slotGetChangesFinished( KJob * )
{
kDebug();
  success();
}

int DataSink::createItem( OSyncData *data )
{
  kDebug();
  char *plain = 0;
  osync_data_get_data( data, &plain, /*size*/0 );
  QString str = QString::fromUtf8( plain );
}

int DataSink::modifyItem( OSyncData *data )
{
kDebug();
}

/*
void DataSink::deleteItem( OSyncData *data )
{
kDebug();
}
*/

void DataSink::commit(OSyncChange * change)
{
  OSyncData *data = 0;
  const char *uid = osync_change_get_uid( change );
  kDebug() << "change uid:" << uid;
  kDebug() << "objtype:" << osync_change_get_objtype( change );
  kDebug() << "objform:" << osync_objformat_get_name (osync_change_get_objformat( change ) ); 

  int rev = -1; // FIXME
  
  switch( osync_change_get_changetype( change ) ) {
    case OSYNC_CHANGE_TYPE_ADDED:
      kDebug() << "data added";
      data = osync_change_get_data( change );
      rev = createItem( data );
      //osync_data_get_data( data, &plain, /*size*/0 );
      //QString data = QString::fromUtf8( plain );
      //kDebug() << "got data:" << data;
      break;

    case OSYNC_CHANGE_TYPE_MODIFIED:
      kDebug() << "data modified";
      data = osync_change_get_data( change );
      rev = modifyItem( data );
      break;

    case OSYNC_CHANGE_TYPE_DELETED:
      kDebug() << "data deleted";
      //deleteItem();
      break;

    case OSYNC_CHANGE_TYPE_UNMODIFIED:
      // should we do something here?
      break;
    default:
      kDebug() << "got invalid changetype?";
  }
  osync_change_set_hash( change, QString::number( rev ).toLatin1() );
  osync_hashtable_update_change( m_hashtable, change );
  
  success();
}

void DataSink::syncDone()
{
  kDebug();
  success();
}

#include "datasink.moc"
