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
#include <QMutexLocker>
#include <kcal/incidence.h>
#include <kcal/mimetypevisitor.h>
#include <kcal/icalformat.h>
#include <boost/shared_ptr.hpp>
#include <KWindowSystem>
#include <akonadi/collectionfetchscope.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/changerecorder.h>
#include <akonadi/cachepolicy.h>
#include <akonadi/collectiondeletejob.h>
#include <akonadi/itemdeletejob.h>

using namespace KCal;
using namespace Akonadi;

typedef boost::shared_ptr<KCal::Incidence> IncidencePtr;

davCalendarResource::davCalendarResource( const QString &id )
  : ResourceBase( id ), mMimeVisitor( new KCal::MimeTypeVisitor ), accessor( NULL ),
    nCollectionsRetrieval( 0 )
{
  new SettingsAdaptor( Settings::self() );
  QDBusConnection::sessionBus().registerObject( QLatin1String( "/Settings" ),
                            Settings::self(), QDBusConnection::ExportAdaptors );

  davCollectionRoot.setParentCollection( Akonadi::Collection::root() );
  davCollectionRoot.setName( name() );
  davCollectionRoot.setRemoteId( name() );
  
  int refreshInterval = Settings::self()->refreshInterval();
  if( refreshInterval == 0 )
    refreshInterval = -1;
  
  Akonadi::CachePolicy cachePolicy;
  cachePolicy.setInheritFromParent( false );
  cachePolicy.setSyncOnDemand( false );
  cachePolicy.setCacheTimeout( -1 );
  cachePolicy.setIntervalCheckTime( refreshInterval );
  davCollectionRoot.setCachePolicy( cachePolicy );
  
  QStringList mimeTypes;
  mimeTypes << Collection::mimeType();
//   mimeTypes << "text/calendar";
  davCollectionRoot.setContentMimeTypes( mimeTypes );
//   davCollectionRoot.setRights( Collection::ReadOnly );
  
  changeRecorder()->fetchCollection( true );
  changeRecorder()->collectionFetchScope().setAncestorRetrieval( Akonadi::CollectionFetchScope::All );
  changeRecorder()->itemFetchScope().fetchFullPayload( true );
  changeRecorder()->itemFetchScope().setAncestorRetrieval( ItemFetchScope::All );
  
  doResourceInitialization();
}

davCalendarResource::~davCalendarResource()
{
  delete accessor;
}

void davCalendarResource::cleanup()
{
  AgentBase::cleanup();
  
  if( accessor )
    accessor->removeCache( name() );
  
  // TODO: remove kwallet folder
}

void davCalendarResource::retrieveCollections()
{
  kDebug() << "Retrieving collections list";
  
  if( !configurationIsValid() ) {
    emit status( Broken, i18n( "The resource is not configured yet" ) );
    cancelTask( i18n( "The resource is not configured yet" ) );
    return;
  }
  
  setCollectionStreamingEnabled( true );
  
  emit status( Running, i18n( "Fetching collections" ) );
  
  Akonadi::Collection::List tmp;
  tmp << davCollectionRoot;
  collectionsRetrievedIncremental( tmp, Akonadi::Collection::List() );
  
  foreach( QString urlStr, Settings::self()->remoteUrls() ) {
    KUrl url( urlStr );
    url.setUser( Settings::self()->username() );
    url.setPass( Settings::self()->password() );
    ++nCollectionsRetrieval;
    accessor->retrieveCollections( url );
  }
}

void davCalendarResource::retrieveItems( const Akonadi::Collection &collection )
{
  kDebug() << "Retrieving items";
  
  if( !configurationIsValid() ) {
    emit status( Broken, i18n( "The resource is not configured yet" ) );
    cancelTask( i18n( "The resource is not configured yet" ) );
    return;
  }
  
  setItemStreamingEnabled( true );
  KUrl url = collection.remoteId();
  url.setUser( Settings::self()->username() );
  url.setPass( Settings::self()->password() );
  accessor->retrieveItems( url );
}

bool davCalendarResource::retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts )
{
  Q_UNUSED( parts );
  
  kDebug() << "Retrieving single item. Remote id = " << item.remoteId();
  
  if( !configurationIsValid() ) {
    emit status( Broken, i18n( "The resource is not configured yet" ) );
    cancelTask( i18n( "The resource is not configured yet" ) );
    return false;
  }
  
  itemRetrieved( item );
  return true;
}

void davCalendarResource::aboutToQuit()
{
  Settings::self()->writeConfig();
}

void davCalendarResource::configure( WId windowId )
{
  ConfigDialog dialog;
  Settings::self()->setWinId( windowId );
  
  if( windowId )
    KWindowSystem::setMainWindow( &dialog, windowId );
  dialog.exec();
  
  if( dialog.result() == QDialog::Accepted ) {
    QStringList rmCollections = dialog.removedUrls();
    dialog.setRemovedUrls( QStringList() );
    
    if( Settings::self()->authenticationRequired() &&
        Settings::self()->useKWallet() &&
        !Settings::self()->username().isEmpty() &&
        !Settings::self()->password().isEmpty() )
      Settings::self()->storePassword();
      
    
    foreach( QString declaredUrl, rmCollections ) {
      if( seenCollections.contains( declaredUrl ) ) {
        seenCollections.remove( declaredUrl );
        Akonadi::Collection c;
        c.setRemoteId( declaredUrl );
        Akonadi::CollectionDeleteJob *j =
            new Akonadi::CollectionDeleteJob( c );
      }
      else {
        bool gotMatch = false;
        foreach( QString realUrl, seenCollections ) {
          if( realUrl.startsWith( declaredUrl ) ) {
            gotMatch = true;
            Akonadi::Collection c;
            c.setRemoteId( realUrl );
            Akonadi::CollectionDeleteJob *j =
                new Akonadi::CollectionDeleteJob( c, this );
          }
        }
        if( gotMatch )
          seenCollections.remove( declaredUrl );
      }
    }
    
    doResourceInitialization();
    emit configurationDialogAccepted();
  }
  else {
    emit configurationDialogRejected();
  }
}

void davCalendarResource::itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection )
{
  kDebug() << "Received notification for added item. Local id = "
      << item.id() << ". Remote id = " << item.remoteId()
      << ". Collection remote id = " << collection.remoteId();
  
  if( !configurationIsValid() ) {
    emit status( Broken, i18n( "The resource is not configured yet" ) );
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
    cancelTask( i18n( "Client did not create a UID for item %1." ).arg( item.id() ) );
    return;
  }
  
  KUrl url( basePath + fileName + ".ics" );
  KCal::ICalFormat formatter;
  QByteArray rawData = formatter.toICalString( ptr.get() ).toUtf8();
  
  kDebug() << "Item " << item.id() << " will be put to " << url.prettyUrl();
  
  QString urlStr = url.prettyUrl(); //QUrl::fromPercentEncoding( url.url().toAscii() );
  putItems[urlStr] = item;
  url.setUser( Settings::self()->username() );
  url.setPass( Settings::self()->password() );
  accessor->putItem( url, "text/calendar", rawData );
}

void davCalendarResource::itemChanged( const Akonadi::Item &item, const QSet<QByteArray> &parts )
{
  Q_UNUSED( parts );
  
  kDebug() << "Received notification for changed item. Local id = " << item.id()
      << ". Remote id = " << item.remoteId();
  
  if( !configurationIsValid() ) {
    emit status( Broken, i18n( "The resource is not configured yet" ) );
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
  
  kDebug() << "Item " << item.id() << " will be put to " << url.prettyUrl();
  
  QString urlStr = url.prettyUrl(); //QUrl::fromPercentEncoding( url.url().toAscii() );
  putItems[urlStr] = item;
  url.setUser( Settings::self()->username() );
  url.setPass( Settings::self()->password() );
  accessor->putItem( url, "text/calendar", rawData, true );
}

void davCalendarResource::itemRemoved( const Akonadi::Item &item )
{
  if( !configurationIsValid() ) {
    emit status( Broken, i18n( "The resource is not configured yet" ) );
    cancelTask( i18n( "The resource is not configured yet" ) );
    return;
  }
  
  KUrl url( item.remoteId() );
  QString urlStr = url.prettyUrl(); //QUrl::fromPercentEncoding( url.url().toAscii() );
  delItems[urlStr] = item;

  kDebug() << "Requesting deletion of item at " << urlStr;
  url.setUser( Settings::self()->username() );
  url.setPass( Settings::self()->password() );
  accessor->removeItem( url );
}

void davCalendarResource::accessorRetrievedCollection( const QString &url, const QString &name )
{
  kDebug() << "Accessor retrieved a collection named " << name << " at " << url;
  
  Akonadi::Collection c;
  c.setParentCollection( davCollectionRoot );
  c.setRemoteId( url );
  if( name.isEmpty() )
    c.setName( this->name() + " (" + url + ")" );
  else
    c.setName( name + " (" + url + ")" );
  
  QStringList mimeTypes;
  mimeTypes << "text/calendar";
  mimeTypes += mMimeVisitor->allMimeTypes();
  c.setContentMimeTypes( mimeTypes );
  
  Akonadi::CachePolicy cachePolicy;
  cachePolicy.setInheritFromParent( true );
  c.setCachePolicy( cachePolicy );
  
  Akonadi::Collection::List tmp;
  tmp << c;
  collectionsRetrievedIncremental( tmp, Akonadi::Collection::List() );
  seenCollections << url;
}

void davCalendarResource::accessorRetrievedCollections()
{
  if( --nCollectionsRetrieval == 0 ) {
    kDebug() << "All collections retrieved";
    collectionsRetrievalDone();
  }
}

void davCalendarResource::accessorRetrievedItem( const davItem &item )
{
  kDebug() << "Accessor retrieved an item at " << item.url;
  
  Akonadi::Item i = createItem( item.data );
  
  IncidencePtr ptr = i.payload<IncidencePtr>();
  if( !ptr.get() )
    return;
  
  i.setRemoteId( item.url );
  
  if( item.contentType.isEmpty() )
    i.setMimeType( "text/calendar" );
  else
    i.setMimeType( item.contentType );
  
  retrievedItems << i;
}

void davCalendarResource::accessorRetrievedItems()
{
  kDebug() << "Accessor finished retrieving items";
  
  QMutexLocker locker( &retrievedItemsMtx );
  Akonadi::Item::List tmp = retrievedItems;
  retrievedItems.clear();
  locker.unlock();
  itemsRetrievedIncremental( tmp, Akonadi::Item::List() );
  itemsRetrievalDone();
  accessor->validateCache();
}

void davCalendarResource::accessorRemovedItem( const KUrl &url )
{
  QString urlStr = url.prettyUrl(); //QUrl::fromPercentEncoding( url.url().toAscii() );
  
  QMutexLocker locker( &delItemsMtx );
  
  if( !delItems.contains( urlStr ) ) {
    emit error( i18n( "Unable to find an upload request for URL %1." ).arg( urlStr ) );
    return;
  }
  
  changeProcessed();
  
  Akonadi::Item i = delItems[urlStr];
  delItems.remove( urlStr );
  locker.unlock();
  
  // Force a refresh of the collection containing the item
  Akonadi::Entity::Id collId = i.storageCollectionId();
  if( collId != -1 )
    synchronizeCollection( collId );
  else
    synchronize();
}

void davCalendarResource::accessorPutItem( const KUrl &oldUrl, const KUrl &newUrl )
{
  QString oldUrlStr = oldUrl.prettyUrl(); //QUrl::fromPercentEncoding( oldUrl.url().toAscii() );
  QString newUrlStr = newUrl.prettyUrl(); //QUrl::fromPercentEncoding( newUrl.url().toAscii() );
  
  QMutexLocker locker( &putItemsMtx );
  
  if( !putItems.contains( oldUrlStr ) ) {
    emit error( i18n( "Unable to find an upload request for URL %1." ).arg( oldUrlStr ) );
    return;
  }
  
  kDebug() << "Item URL changed from " << oldUrlStr << " to " << newUrlStr;
  
  Akonadi::Item i = putItems[oldUrlStr];
  putItems.remove( oldUrlStr );
  locker.unlock();
  
  i.setRemoteId( newUrlStr );
  changeCommitted( i );
  
  // Force a refresh of the collection containing the item
  Akonadi::Entity::Id collId = i.storageCollectionId();
  if( collId != -1 )
    synchronizeCollection( collId );
  else
    synchronize();
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
  
  Akonadi::ItemDeleteJob *job = new Akonadi::ItemDeleteJob( removed, this );
  accessor->saveCache( name() );
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

void davCalendarResource::doResourceInitialization()
{
  if( !configurationIsValid() ) {
    emit status( Broken, i18n( "Resource not configured" ) );
    return;
  }
  
  delete accessor;
  
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
  
  connect( accessor, SIGNAL( itemRemoved( const KUrl& ) ),
           this, SLOT( accessorRemovedItem( const KUrl& ) ) );
  
  connect( accessor, SIGNAL( itemPut( const KUrl&, const KUrl& ) ),
           this, SLOT( accessorPutItem( const KUrl&, const KUrl& ) ) );
  
  connect( accessor, SIGNAL( backendItemsRemoved( const QList<davItem>& ) ),
           this, SLOT( backendItemsRemoved( const QList<davItem>& ) ) );
  connect( accessor, SIGNAL( backendItemChanged( const davItem& ) ),
           this, SLOT( backendItemChanged( const davItem& ) ) );
  
  accessor->loadCache( name() );
  synchronizeCollectionTree();
}

bool davCalendarResource::configurationIsValid()
{
  if( !(Settings::self()->remoteProtocol() == Settings::groupdav ||
      Settings::self()->remoteProtocol() == Settings::caldav ) )
    return false;
  
  if( Settings::self()->remoteUrls().empty() )
    return false;
  
  QStringList urls;
  foreach( QString url, Settings::self()->remoteUrls() ) {
    if( !url.endsWith( "/" ) )
      url.append( "/" );
    urls << url;
  }
  Settings::self()->setRemoteUrls( urls );
  
  if( Settings::self()->authenticationRequired() ) {
    if( Settings::self()->username().isEmpty() )
      return false;
    
    if( Settings::self()->password().isEmpty() )
      Settings::self()->getPassword();
    
    if( Settings::self()->password().isEmpty() )
      return false;
  }
  
  int newICT = Settings::self()->refreshInterval();
  if( newICT == 0 )
    newICT = -1;
  if( newICT != davCollectionRoot.cachePolicy().intervalCheckTime() ) {
    Akonadi::CachePolicy cachePolicy = davCollectionRoot.cachePolicy();
    cachePolicy.setIntervalCheckTime( newICT );
    davCollectionRoot.setCachePolicy( cachePolicy );
  }
  
  return true;
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
