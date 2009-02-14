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

// calendar includes
#include <kcal/incidence.h>
#include <kcal/icalformat.h>

// contact includes
#include <kabc/vcardconverter.h>
#include <kabc/addressee.h>

// notes includes
// TODO

#include <KDebug>
#include <KLocale>
#include <KUrl>

using namespace Akonadi;

typedef boost::shared_ptr<KCal::Incidence> IncidencePtr;


DataSink::DataSink( int type ) :
    SinkBase( GetChanges | Commit | SyncDone )
{
  m_type = type;
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
  if( ! enabled ) {
    kDebug() << "sink is not enabled..";
    return false;
  }

  QString configdir( osync_plugin_info_get_configdir( info ) );
  QString hashfile = QString("%1/%2-hash.db").arg( configdir, osync_objtype_sink_get_name( sink ) );

  m_hashtable = osync_hashtable_new( hashfile.toUtf8(), osync_objtype_sink_get_name( sink ), error );

  if( ! osync_hashtable_load( m_hashtable, error ) ) {
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

  OSyncError *oerror = 0;

  if( osync_objtype_sink_get_slowsync( sink() ) ) {
    kDebug() << "we're in the middle of slow-syncing...";
    osync_trace( TRACE_INTERNAL, "Got slow-sync, resetting hashtable" );
    if( ! osync_hashtable_slowsync( m_hashtable, &oerror ) ) {
      warning( oerror );
      osync_trace( TRACE_EXIT_ERROR, "%s: %s", __PRETTY_FUNCTION__, osync_error_print( &oerror ) );
      return;
    }
  }

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

void DataSink::slotItemsReceived( const Item::List &items )
{
  kDebug() << "retrieved" << items.count() << "items";
  Q_FOREACH( const Item& item, items ) {
    //kDebug() << item.payloadData();
    reportChange( item );
  }
}

void DataSink::reportChange( const Item& item )
{
  OSyncFormatEnv *formatenv = osync_plugin_info_get_format_env( pluginInfo() );
  OSyncObjFormat *format = osync_format_env_find_objformat( formatenv, formatName().toLatin1() );

  OSyncError *error = 0;

  OSyncChange *change = osync_change_new( &error );
  if ( !change ) {
    warning( error );
    return;
  }

  osync_change_set_uid( change, QString::number( item.id() ).toLatin1() );

  error = 0;

  OSyncData *odata = osync_data_new( item.payloadData().data(), item.payloadData().size(), format, &error );
  if ( !odata ) {
    osync_change_unref( change );
    warning( error );
    return;
  }

  osync_data_set_objtype( odata, osync_objtype_sink_get_name( sink() ) );

  osync_change_set_data( change, odata );

  kDebug() << item.id() << "DATA:" << osync_data_get_printable( odata ) << "\n" << "ORIG:" << item.payloadData().data();

  osync_data_unref( odata );

  osync_change_set_hash( change, QString::number( item.revision() ).toLatin1() );

  OSyncChangeType changeType = osync_hashtable_get_changetype( m_hashtable, change );
  osync_change_set_changetype( change, changeType );

  osync_hashtable_update_change( m_hashtable, change );

/*
kDebug() << "changeid:" << osync_change_get_uid( change )
           << "itemid:" << item.id()
           << "revision:" << item.revision()
           << "changetype:" << changeType
           << "hash:" << osync_hashtable_get_hash( m_hashtable, osync_change_get_uid( change ) )
           << "objtype:" << osync_change_get_objtype( change )
           << "objform:" << osync_objformat_get_name( osync_change_get_objformat( change ) )
           << "sinkname:" << osync_objtype_sink_get_name( sink() );
*/
  if ( changeType != OSYNC_CHANGE_TYPE_UNMODIFIED )
    osync_context_report_change( context(), change );

  // perhaps this should be only called when an error has happened ie. never after _context_report_change()?
  //osync_change_unref( change ); // if this gets called, we get broken pipes. any ideas?
}

void DataSink::slotGetChangesFinished( KJob * )
{
  OSyncFormatEnv *formatenv = osync_plugin_info_get_format_env( pluginInfo() );
  OSyncObjFormat *format = osync_format_env_find_objformat( formatenv, formatName().toLatin1() );

  // after the items have been processed, see what's been deleted and send them to opensync
  OSyncError *error = 0;
  OSyncList *u, *uids = osync_hashtable_get_deleted( m_hashtable );

  for( u = uids; u; u = u->next) {
    QString uid( (char *)u->data );
    kDebug() << "going to delete with uid:" << uid;

    OSyncChange *change = osync_change_new( &error );
    if( !change ) {
      warning( error );
      continue;
    }

    osync_change_set_changetype( change, OSYNC_CHANGE_TYPE_DELETED );
    osync_change_set_uid( change, uid.toLatin1() );

    error = 0;
    OSyncData *data = osync_data_new( NULL, 0, format, &error );
    if( !data ) {
      osync_change_unref( change );
      warning( error );
      continue;
    }

    osync_data_set_objtype( data, osync_objtype_sink_get_name( sink() ) );
    osync_change_set_data( change, data );

    osync_hashtable_update_change( m_hashtable, change );

    osync_change_unref( change );
  }
  osync_list_free( uids );

  kDebug() << "got all changes..";
  success();
}

void DataSink::commit(OSyncChange *change)
{
  kDebug() << "change uid:" << osync_change_get_uid( change );
  kDebug() << "objtype:" << osync_change_get_objtype( change );
  kDebug() << "objform:" << osync_objformat_get_name (osync_change_get_objformat( change ) );

  switch( osync_change_get_changetype( change ) ) {
    case OSYNC_CHANGE_TYPE_ADDED: {
      const Item item = createItem( change );
      osync_change_set_uid( change, QString::number( item.id() ).toLatin1() );
      osync_change_set_hash( change, QString::number( item.revision() ).toLatin1() );
      kDebug() << "ADDED:" << osync_change_get_uid( change );
      break; }

    case OSYNC_CHANGE_TYPE_MODIFIED: {
      const Item item = modifyItem( change );
      osync_change_set_uid( change, QString::number( item.id() ).toLatin1() );
      osync_change_set_hash( change, QString::number( item.revision() ).toLatin1() );
      kDebug() << "MODIFIED:" << osync_change_get_uid( change );
      break; }

    case OSYNC_CHANGE_TYPE_DELETED: {
      deleteItem( change );
      kDebug() << "DELETED:" << osync_change_get_uid( change );
      break; }

    case OSYNC_CHANGE_TYPE_UNMODIFIED: {
      kDebug() << "UNMODIFIED";
      // should we do something here?
      break; }

    default:
      kDebug() << "got invalid changetype?";
  }

  osync_hashtable_update_change( m_hashtable, change );
  success();
}

const Item DataSink::createItem( OSyncChange *change )
{
  Collection col = collection();
  if ( !col.isValid() ) // error handling
    return Item();
  kDebug() << "cuid:" << osync_change_get_uid( change );

  ItemCreateJob *job = new Akonadi::ItemCreateJob( createAkonadiItem( change ), col );

  if( ! job->exec() )
    kDebug() << "creating an item failed";

  return job->item(); // handle !job->exec in return too..
}

const Item DataSink::modifyItem( OSyncChange *change )
{
  char *plain = 0;
  osync_data_get_data( osync_change_get_data( change ), &plain, /*size*/0 );
  QString str = QString::fromUtf8( plain );

  QString id = QString( osync_change_get_uid( change ) );
  Item item = fetchItem( id );
  if( ! item.isValid() ) // TODO proper error handling
    return Item();

  //event.setMimeType( "application/x-vnd.akonadi.calendar.event" );
  //Item newItem = createAkonadiItem( change );
  setPayload(&item, str);
  ItemModifyJob *modifyJob = new Akonadi::ItemModifyJob( item );
  if( modifyJob->exec() ) {
    kDebug() << "modification completed";
    return modifyJob->item();
  }
  else
    kDebug() << "unable to modify";


  return Item();
}

void DataSink::deleteItem( OSyncChange *change )
{
  QString id = QString( osync_change_get_uid( change ) );
  Item item = fetchItem( id );
  if( ! item.isValid() ) // TODO proper error handling
    return;

  kDebug() << "delete with id: " << item.id();
  /*ItemDeleteJob *job = new ItemDeleteJob( item );

 if( job->exec() ) {
   kDebug() << "item deleted";
 }
 else
   kDebug() << "unable to delete item";*/
}

bool DataSink::setPayload( Item *item, const QString &str )
{
  switch( m_type ) {
    case Calendar: {
      KCal::ICalFormat format;
      KCal::Incidence *calEntry = format.fromString( str );

      item->setMimeType( "application/x-vnd.akonadi.calendar.event" );
      item->setPayload<IncidencePtr>( IncidencePtr( calEntry->clone() ) );

      break;
    }
    case Contacts: {
      KABC::VCardConverter converter;
      KABC::Addressee vcard = converter.parseVCard( str.toUtf8() );
      kDebug() << vcard.uid() << vcard.name();

      item->setMimeType( Addressee::mimeType() );
      item->setPayload<KABC::Addressee>( vcard );
      break;
    }
    case Notes: {
      kDebug() << "notes";
      break;
    }
    default:
      // should not happen
     return false;
  }

  return true;
}

const Item DataSink::createAkonadiItem( OSyncChange *change )
{
  char *plain = 0;
  osync_data_get_data( osync_change_get_data( change ), &plain, /*size*/0 );
  QString str = QString::fromUtf8( plain );
  Akonadi::Item item;
  setPayload(&item, str);
  return item;
}

const QString DataSink::formatName()
{
  QString formatName;
  switch( m_type ) {
    case Calendar:
      formatName = "vevent20";
      break;
    case Contacts:
      formatName = "vcard10";
      break;
    case Notes:
      formatName = "vnote10";
      break;
    default:
      kDebug() << "invalid datasink type";
      return QString();
  }

  return formatName;
}

const Item DataSink::fetchItem( const QString& id )
{
  ItemFetchJob *fetchJob = new ItemFetchJob( Item( id ) );
  fetchJob->fetchScope().fetchFullPayload();

  if( fetchJob->exec() ) {
    foreach ( Item item, fetchJob->items() ) {
      if( QString::number( item.id() ) == id ) {
        kDebug() << "got item";
        return item;
      }
    }
  }

  // no such item found?
  return Item();
}

void DataSink::syncDone()
{
  kDebug();
  OSyncError *error = 0;
  osync_hashtable_save( m_hashtable, &error );

  //TODO check for errors
  success();
}

#include "datasink.moc"
