/*
    Copyright (c) 2009 Grégory Oestreicher <greg@kamago.net>
    
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include "davcalendarresource.h"

#include "settings.h"
#include "settingsadaptor.h"
#include "configdialog.h"
#include "groupdavcalendar.h"
#include "caldavcalendar.h"

#include <QtDBus/QDBusConnection>
#include <kcal/incidence.h>
#include <kcal/kcalmimetypevisitor.h>
#include <kcal/icalformat.h>
#include <boost/shared_ptr.hpp>
#include <KWindowSystem>
#include <akonadi/collectionfetchscope.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/changerecorder.h>
#include <akonadi/cachepolicy.h>
#include <akonadi/itemdeletejob.h>

using namespace KCal;
using namespace Akonadi;

typedef boost::shared_ptr<KCal::Incidence> IncidencePtr;

davCalendarResource::davCalendarResource( const QString &id )
  : ResourceBase( id ), mMimeVisitor( new KCalMimeTypeVisitor ), accessor( NULL )
{
  new SettingsAdaptor( Settings::self() );
  QDBusConnection::sessionBus().registerObject( QLatin1String( "/Settings" ),
                            Settings::self(), QDBusConnection::ExportAdaptors );

  changeRecorder()->fetchCollection( true );
  changeRecorder()->collectionFetchScope().setAncestorRetrieval( Akonadi::CollectionFetchScope::All );
  changeRecorder()->itemFetchScope().fetchFullPayload( true );
  changeRecorder()->itemFetchScope().setAncestorRetrieval( ItemFetchScope::All );
  createAccessor();
}

davCalendarResource::~davCalendarResource()
{
  if( accessor ) {
    delete accessor;
  }
}

void davCalendarResource::retrieveCollections()
{
  kDebug() << "Retrieving collections list";
  
  if( !accessor ) {
    emit status( Broken, i18n( "No protocol configured" ) );
    cancelTask( i18n( "The resource is not configured yet" ) );
    return;
  }
  
  setCollectionStreamingEnabled( true );
  
  emit status( Running, i18n( "Fetching collections" ) );
  accessor->retrieveCollections( Settings::self()->remoteUrl() );
}

void davCalendarResource::retrieveItems( const Akonadi::Collection &collection )
{
  kDebug() << "Retrieving items";
  
  if( !accessor ) {
    emit status( Broken, i18n( "No protocol configured" ) );
    cancelTask( i18n( "The resource is not configured yet" ) );
    return;
  }
  
  setItemStreamingEnabled( true );
  accessor->retrieveItems( KUrl( collection.remoteId() ) );
}

bool davCalendarResource::retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts )
{
  Q_UNUSED( parts );
  
  kDebug() << "Retrieving single item. Remote id = " << item.remoteId();
  
  if( !accessor ) {
    emit status( Broken, i18n( "No protocol configured" ) );
    cancelTask( i18n( "The resource is not configured yet" ) );
    return false;
  }
  
  // NOTE: do the call to davAccessor::retrieveItem
  itemRetrieved( item );
  return true;
}

void davCalendarResource::aboutToQuit()
{
  Settings::self()->writeConfig();
}

void davCalendarResource::configure( WId windowId )
{
  Q_UNUSED( windowId );
  
  ConfigDialog dialog;
  if( windowId )
    KWindowSystem::setMainWindow( &dialog, windowId );
  dialog.exec();
  
  if( accessor ) {
    delete accessor;
  }
  createAccessor();
}

void davCalendarResource::itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection )
{
  kDebug() << "Received notification for added item. Local id = "
      << item.id() << ". Remote id = " << item.remoteId()
      << ". Collection remote id = " << collection.remoteId();
  
  if( !accessor ) {
    emit status( Broken, i18n( "No protocol configured" ) );
    cancelTask( i18n( "The resource is not configured yet" ) );
    return;
  }
  
  if( !item.hasPayload<IncidencePtr>() ) {
    kError() << "Item " << item.id() << " doesn't has a valid payload";
    cancelTask( i18n( "Unable to retrieve added item %1." ).arg( item.id() ) );
    return;
  }
  
  IncidencePtr ptr = item.payload<IncidencePtr>();
  
  QString basePath = collection.remoteId();
  if( basePath.isEmpty() ) {
    kError() << "Invalid remote id for collection " << collection.id() << " = " << collection.remoteId();
    cancelTask( i18n( "Invalid collection for item %1." ).arg( item.id() ) );
    return;
  }
  
  QString fileName = ptr->uid();
  if( fileName.isEmpty() ) {
    kError() << "Invalid incidence uid";
    cancelTask( i18n( "Client didn't create an UID for item %1." ).arg( item.id() ) );
    return;
  }
  
  KUrl url( basePath + "/" + fileName );
  KCal::ICalFormat formatter;
  QByteArray rawData = formatter.toICalString( ptr.get() ).toUtf8();
  
  kDebug() << "Item " << item.id() << " will be put to " << url.url();
  
  putItems[url.url()] = item;
  accessor->putItem( url, "text/calendar", rawData );
}

void davCalendarResource::itemChanged( const Akonadi::Item &item, const QSet<QByteArray> &parts )
{
  Q_UNUSED( parts );
  
  kDebug() << "Received notification for changed item. Local id = " << item.id()
      << ". Remote id = " << item.remoteId();
  
  if( !accessor ) {
    emit status( Broken, i18n( "No protocol configured" ) );
    cancelTask( i18n( "The resource is not configured yet" ) );
    return;
  }
  
  if( !item.hasPayload<IncidencePtr>() ) {
    kError() << "Item " << item.id() << " doesn't has a valid payload";
    cancelTask( i18n( "Unable to retrieve added item %1." ).arg( item.id() ) );
    return;
  }
  
  IncidencePtr ptr = item.payload<IncidencePtr>();
  KUrl url( item.remoteId() );
  KCal::ICalFormat formatter;
  QByteArray rawData = formatter.toICalString( ptr.get() ).toUtf8();
  
  kDebug() << "Item " << item.id() << " will be put to " << url.url();
  
  putItems[url.url()] = item;
  accessor->putItem( url, "text/calendar", rawData, true );
}

void davCalendarResource::itemRemoved( const Akonadi::Item &item )
{
  if( !accessor ) {
    emit status( Broken, i18n( "No protocol configured" ) );
    cancelTask( i18n( "The resource is not configured yet" ) );
    return;
  }

  accessor->removeItem( KUrl( item.remoteId() ) );
}

void davCalendarResource::accessorRetrievedCollection( const QString &url, const QString &name )
{
  kDebug() << "Accessor retrieved a collection named " << name << " at " << url;
  Akonadi::Collection c;
  c.setParentCollection( Akonadi::Collection::root() );
  c.setRemoteId( url );
  if( name.isEmpty() )
    c.setName( this->name() );
  else
    c.setName( name );
  
  QStringList mimeTypes;
  mimeTypes << "text/calendar";
  mimeTypes += mMimeVisitor->allMimeTypes();
  c.setContentMimeTypes( mimeTypes );
  
  int refreshInterval = Settings::self()->refreshInterval();
  if( refreshInterval == 0 )
    refreshInterval = -1;
  
  Akonadi::CachePolicy cachePolicy;
  cachePolicy.setInheritFromParent( false );
  cachePolicy.setSyncOnDemand( true );
  cachePolicy.setCacheTimeout( -1 );
  cachePolicy.setIntervalCheckTime( refreshInterval );
  c.setCachePolicy( cachePolicy );
  
  Akonadi::Collection::List tmp;
  tmp << c;
  collectionsRetrievedIncremental( tmp, Akonadi::Collection::List() );
}

void davCalendarResource::accessorRetrievedCollections()
{
  kDebug() << "All collections retrieved";
  collectionsRetrievalDone();
}

void davCalendarResource::accessorRetrievedItem( const davItem &item )
{
  kDebug() << "Accessor retrieved a new item at " << item.url;
  
  Akonadi::Item i = createItem( item.data );
  i.setRemoteId( item.url );
  
  if( item.contentType.isEmpty() )
    i.setMimeType( "text/calendar" );
  else
    i.setMimeType( item.contentType );
  
  Akonadi::Item::List tmp;
  tmp << i;
  itemsRetrievedIncremental( tmp, Akonadi::Item::List() );
}

void davCalendarResource::accessorRetrievedItemFromCache( const davItem &item )
{
  kDebug() << "Accessor retrieved a cached item at " << item.url;
  this->accessorRetrievedItem( item );
}

void davCalendarResource::accessorRetrievedItems()
{
  kDebug() << "Accessor finished retrieving items";
  
  accessor->validateCache();
}

void davCalendarResource::accessorRemovedItem()
{
  emit status( Idle, i18n( "Using GroupDAV" ) );
  changeProcessed();
}

void davCalendarResource::accessorPutItem( const KUrl &oldUrl, const KUrl &newUrl )
{
  if( !putItems.contains( oldUrl.url() ) ) {
    emit error( i18n( "Unable to find an upload request for URL %1." ).arg( oldUrl.url() ) );
    return;
  }
  
  kDebug() << "Item URL changed from " << oldUrl << " to " << newUrl;
  
  Akonadi::Item i = putItems[oldUrl.url()];
  i.setRemoteId( newUrl.url() );
  changeCommitted( i );
}

void davCalendarResource::backendItemChanged( const davItem &item )
{
  kDebug() << "The item at " << item.url << " changed on the backend";
  
  Akonadi::Item i = createItem( item.data );
  i.setRemoteId( item.url );
  
  if( item.contentType.isEmpty() )
    i.setMimeType( "text/calendar" );
  else
    i.setMimeType( item.contentType );
  
  Akonadi::Item::List tmp;
  tmp << i;
  itemsRetrievedIncremental( tmp, Akonadi::Item::List() );
}

void davCalendarResource::backendItemsRemoved( const QList<davItem> &items )
{
  kDebug() << "Got " << items.size() << " items removed on the backend";
  
  Akonadi::Item::List removed;
  
  foreach( davItem item, items ) {
    kDebug() << "Item at " << item.url << " was removed";
    
    Akonadi::Item i = createItem( item.data );
    i.setRemoteId( item.url );
  
    if( item.contentType.isEmpty() )
      i.setMimeType( "text/calendar" );
    else
      i.setMimeType( item.contentType );
    
    removed << i;
  }
  
  itemsRetrievedIncremental( Akonadi::Item::List(), removed );
  itemsRetrievalDone();
}

void davCalendarResource::accessorStatus( const QString &s )
{
  emit status( Running, s );
}

void davCalendarResource::accessorError( const QString &err, bool cancelRequest )
{
  if( cancelRequest )
    cancelTask( err );
  emit error( err );
}

void davCalendarResource::createAccessor()
{
  switch( Settings::self()->remoteProtocol() ) {
    case Settings::groupdav:
      accessor = new groupdavCalendarAccessor();
      emit status( Idle, i18n( "Using GroupDAV" ) );
      break;
    case Settings::caldav:
      accessor = new caldavCalendarAccessor();
      emit status( Idle, i18n( "Using CalDAV" ) );
      break;
    default:
      emit status( Broken, i18n( "No protocol configured" ) );
      return;
  }
  
  connect( accessor, SIGNAL( accessorStatus( const QString& ) ),
           this, SLOT( accessorStatus( const QString& ) ) );
  connect( accessor, SIGNAL( accessorError( const QString&, bool ) ),
           this, SLOT( accessorError( const QString&, bool ) ) );
  
  connect( accessor, SIGNAL( collectionRetrieved( const QString&, const QString& ) ),
           this, SLOT( accessorRetrievedCollection( const QString&, const QString& ) ) );
  connect( accessor, SIGNAL( collectionsRetrieved() ),
           this, SLOT( accessorRetrievedCollections() ) );
  
  connect( accessor, SIGNAL( itemRetrieved( const davItem& ) ),
           this, SLOT( accessorRetrievedItem( const davItem& ) ) );
  connect( accessor, SIGNAL( itemsRetrieved() ),
           this, SLOT( accessorRetrievedItems() ) );
  
  connect( accessor, SIGNAL( itemRemoved() ),
           this, SLOT( accessorRemovedItem() ) );
  
  connect( accessor, SIGNAL( itemPut( const KUrl&, const KUrl& ) ),
           this, SLOT( accessorPutItem( const KUrl&, const KUrl& ) ) );
  
  connect( accessor, SIGNAL( backendItemChanged( const davItem& ) ),
           this, SLOT( backendItemChanged( const davItem& ) ) );
  
  connect( accessor, SIGNAL( backendItemsRemoved( const QList<davItem>& ) ),
           this, SLOT( backendItemsRemoved( const QList<davItem>& ) ) );
  
  synchronizeCollectionTree();
}

Akonadi::Item davCalendarResource::createItem( const QByteArray &data )
{
  Akonadi::Item ret;
  QString iCalData = QString::fromUtf8( data );
  KCal::ICalFormat formatter;
  IncidencePtr ptr( formatter.fromString( iCalData ) );
  ret.setPayload( ptr );
  return ret;
}

AKONADI_RESOURCE_MAIN( davCalendarResource )

#include "davcalendarresource.moc"
