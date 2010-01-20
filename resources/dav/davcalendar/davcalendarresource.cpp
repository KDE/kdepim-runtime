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

#include "caldavcalendar.h"
#include "configdialog.h"
#include "groupdavcalendar.h"
#include "settings.h"
#include "settingsadaptor.h"

#include <akonadi/attribute.h>
#include <akonadi/attributefactory.h>
#include <akonadi/cachepolicy.h>
#include <akonadi/changerecorder.h>
#include <akonadi/collectiondeletejob.h>
#include <akonadi/collectionfetchjob.h>
#include <akonadi/collectionfetchscope.h>
#include <akonadi/entitydisplayattribute.h>
#include <akonadi/itemdeletejob.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/kcal/incidencemimetypevisitor.h>
#include <boost/shared_ptr.hpp>
#include <kcal/icalformat.h>
#include <kcal/incidence.h>
#include <kwindowsystem.h>

#include <QtCore/QMutexLocker>
#include <QtDBus/QDBusConnection>

using namespace KCal;
using namespace Akonadi;

typedef boost::shared_ptr<KCal::Incidence> IncidencePtr;

class etagAttribute : public Akonadi::Attribute
{
public:
  etagAttribute( const QString &etag = QString() ) : _etag( etag )
  {
  }

  QString etag() const
  {
    return _etag;
  }

  void setEtag( const QString &etag )
  {
    _etag = etag;
  }

  virtual Akonadi::Attribute* clone() const
  {
    return new etagAttribute( _etag );
  }

  virtual QByteArray type() const
  {
    return "etag";
  }

  virtual QByteArray serialized() const
  {
    return _etag.toAscii();
  }

  virtual void deserialize( const QByteArray &data )
  {
    _etag = data;
  }

private:
  QString _etag;
};

static Akonadi::Item createAkonadiItem( const QByteArray &data )
{
  Akonadi::Item item;

  const QString iCalData = QString::fromUtf8( data );

  KCal::ICalFormat formatter;
  const IncidencePtr ptr( formatter.fromString( iCalData ) );

  item.setPayload( ptr );

  return item;
}

DavCalendarResource::DavCalendarResource( const QString &id )
  : ResourceBase( id ), mMimeVisitor( new Akonadi::IncidenceMimeTypeVisitor ), mAccessor( 0 ),
    mCollectionsRetrievalCount( 0 ), mItemsRetrievedCount( 0 )
{
  new SettingsAdaptor( Settings::self() );
  QDBusConnection::sessionBus().registerObject( QLatin1String( "/Settings" ),
                                                Settings::self(), QDBusConnection::ExportAdaptors );

  AttributeFactory::registerAttribute<etagAttribute>();
  AttributeFactory::registerAttribute<EntityDisplayAttribute>();

  mDavCollectionRoot.setParentCollection( Collection::root() );
  mDavCollectionRoot.setName( name() );
  mDavCollectionRoot.setRemoteId( name() );
  mDavCollectionRoot.setContentMimeTypes( QStringList() << Collection::mimeType() );

  int refreshInterval = Settings::self()->refreshInterval();
  if ( refreshInterval == 0 )
    refreshInterval = -1;

  Akonadi::CachePolicy cachePolicy;
  cachePolicy.setInheritFromParent( false );
  cachePolicy.setSyncOnDemand( false );
  cachePolicy.setCacheTimeout( -1 );
  cachePolicy.setIntervalCheckTime( refreshInterval );
  mDavCollectionRoot.setCachePolicy( cachePolicy );

  changeRecorder()->fetchCollection( true );
  changeRecorder()->collectionFetchScope().setAncestorRetrieval( Akonadi::CollectionFetchScope::All );
  changeRecorder()->itemFetchScope().fetchFullPayload( true );
  changeRecorder()->itemFetchScope().setAncestorRetrieval( ItemFetchScope::All );

  Settings::self()->setWinId( winIdForDialogs() );
  doResourceInitialization();
}

DavCalendarResource::~DavCalendarResource()
{
  delete mAccessor;
}

void DavCalendarResource::retrieveCollections()
{
  kDebug() << "Retrieving collections list";

  if ( !configurationIsValid() ) {
    emit status( Broken, i18n( "The resource is not configured yet" ) );
    cancelTask( i18n( "The resource is not configured yet" ) );
    return;
  }

  setCollectionStreamingEnabled( true );

  emit status( Running, i18n( "Fetching collections" ) );

  Akonadi::Collection::List collections;
  collections << mDavCollectionRoot;
  collectionsRetrievedIncremental( collections, Akonadi::Collection::List() );

  foreach ( const QString &urlStr, Settings::self()->remoteUrls() ) {
    KUrl url( urlStr );
    url.setUser( Settings::self()->username() );
    url.setPass( Settings::self()->password() );

    ++mCollectionsRetrievalCount;
    mAccessor->retrieveCollections( url );
  }
}

void DavCalendarResource::retrieveItems( const Akonadi::Collection &collection )
{
  kDebug() << "Retrieving items";

  if ( !configurationIsValid() ) {
    emit status( Broken, i18n( "The resource is not configured yet" ) );
    cancelTask( i18n( "The resource is not configured yet" ) );
    return;
  }

  setItemStreamingEnabled( true );

  KUrl url = collection.remoteId();
  url.setUser( Settings::self()->username() );
  url.setPass( Settings::self()->password() );

  mAccessor->retrieveItems( url );
}

bool DavCalendarResource::retrieveItem( const Akonadi::Item &item, const QSet<QByteArray>& )
{
  kDebug() << "Retrieving single item. Remote id = " << item.remoteId();

  if ( !configurationIsValid() ) {
    emit status( Broken, i18n( "The resource is not configured yet" ) );
    cancelTask( i18n( "The resource is not configured yet" ) );
    return false;
  }

  itemRetrieved( item );

  return true;
}

void DavCalendarResource::aboutToQuit()
{
  Settings::self()->writeConfig();
}

void DavCalendarResource::configure( WId windowId )
{
  ConfigDialog dialog;
  Settings::self()->setWinId( windowId );

  if ( windowId )
    KWindowSystem::setMainWindow( &dialog, windowId );

  dialog.exec();

  if ( dialog.result() == QDialog::Accepted ) {
    const QStringList rmCollections = dialog.removedUrls();
    dialog.setRemovedUrls( QStringList() );

    foreach ( const QString &declaredUrl, rmCollections ) {
      if ( mSeenCollections.contains( declaredUrl ) ) {
        mSeenCollections.remove( declaredUrl );

        Akonadi::Collection collection;
        collection.setRemoteId( declaredUrl );

        new Akonadi::CollectionDeleteJob( collection );
      } else {
        bool gotMatch = false;
        foreach ( const QString &realUrl, mSeenCollections ) {
          if ( realUrl.startsWith( declaredUrl ) ) {
            gotMatch = true;

            Akonadi::Collection collection;
            collection.setRemoteId( realUrl );

            new Akonadi::CollectionDeleteJob( collection, this );
          }
        }

        if ( gotMatch )
          mSeenCollections.remove( declaredUrl );
      }
    }

    doResourceInitialization();
    emit configurationDialogAccepted();
  } else {
    emit configurationDialogRejected();
  }
}

void DavCalendarResource::itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection )
{
  kDebug() << "Received notification for added item. Local id = "
      << item.id() << ". Remote id = " << item.remoteId()
      << ". Collection remote id = " << collection.remoteId();

  if ( !configurationIsValid() ) {
    emit status( Broken, i18n( "The resource is not configured yet" ) );
    cancelTask( i18n( "The resource is not configured yet" ) );
    return;
  }

  if ( !item.hasPayload<IncidencePtr>() ) {
    kError() << "Item " << item.id() << " doesn't has a valid payload";
    cancelTask( i18n( "Unable to retrieve added item %1." ).arg( item.id() ) );
    return;
  }

  const IncidencePtr ptr = item.payload<IncidencePtr>();

  const QString basePath = collection.remoteId();
  if ( basePath.isEmpty() ) {
    kError() << "Invalid remote id for collection " << collection.id() << " = " << collection.remoteId();
    cancelTask( i18n( "Invalid collection for item %1." ).arg( item.id() ) );
    return;
  }

  const QString fileName = ptr->uid();
  if ( fileName.isEmpty() ) {
    kError() << "Invalid incidence uid";
    cancelTask( i18n( "Client did not create a UID for item %1." ).arg( item.id() ) );
    return;
  }

  KUrl url( basePath + fileName + ".ics" );
  url.setUser( Settings::self()->username() );
  url.setPass( Settings::self()->password() );

  const QString urlStr = url.prettyUrl();
  kDebug() << "Item " << item.id() << " will be put to " << urlStr;

  mPutItems[ urlStr ] = item;

  KCal::ICalFormat formatter;
  const QByteArray rawData = formatter.toICalString( ptr.get() ).toUtf8();

  mAccessor->putItem( url, "text/calendar", rawData );
}

void DavCalendarResource::itemChanged( const Akonadi::Item &item, const QSet<QByteArray>& )
{
  kDebug() << "Received notification for changed item. Local id = " << item.id()
      << ". Remote id = " << item.remoteId();

  if ( !configurationIsValid() ) {
    emit status( Broken, i18n( "The resource is not configured yet" ) );
    cancelTask( i18n( "The resource is not configured yet" ) );
    return;
  }

  if ( !item.hasPayload<IncidencePtr>() ) {
    kError() << "Item " << item.id() << " doesn't has a valid payload";
    cancelTask( i18n( "Unable to retrieve added item %1." ).arg( item.id() ) );
    return;
  }

  const IncidencePtr ptr = item.payload<IncidencePtr>();
  KUrl url( item.remoteId() );
  url.setUser( Settings::self()->username() );
  url.setPass( Settings::self()->password() );

  const QString urlStr = url.prettyUrl();
  kDebug() << "Item " << item.id() << " will be put to " << urlStr;

  mPutItems[ urlStr ] = item;

  KCal::ICalFormat formatter;
  const QByteArray rawData = formatter.toICalString( ptr.get() ).toUtf8();

  mAccessor->putItem( url, "text/calendar", rawData, true );
}

void DavCalendarResource::itemRemoved( const Akonadi::Item &item )
{
  if ( !configurationIsValid() ) {
    emit status( Broken, i18n( "The resource is not configured yet" ) );
    cancelTask( i18n( "The resource is not configured yet" ) );
    return;
  }

  KUrl url( item.remoteId() );
  url.setUser( Settings::self()->username() );
  url.setPass( Settings::self()->password() );

  const QString urlStr = url.prettyUrl();
  kDebug() << "Requesting deletion of item at " << urlStr;

  mDelItems[ urlStr ] = item;

  mAccessor->removeItem( url );
}

void DavCalendarResource::accessorRetrievedCollection( const QString &url, const QString &collectionName )
{
  kDebug() << "Accessor retrieved a collection named " << collectionName << " at " << url;

  Akonadi::Collection collection;
  collection.setParentCollection( mDavCollectionRoot );
  collection.setRemoteId( url );
  if ( collectionName.isEmpty() )
    collection.setName( name() + " (" + url + ")" );
  else
    collection.setName( collectionName + " (" + url + ")" );

  QStringList mimeTypes;
  mimeTypes << "text/calendar";
  mimeTypes += mMimeVisitor->allMimeTypes();
  collection.setContentMimeTypes( mimeTypes );

  collectionsRetrievedIncremental( Akonadi::Collection::List() << collection, Akonadi::Collection::List() );

  mSeenCollections << url;
}

void DavCalendarResource::accessorRetrievedCollections()
{
  if ( --mCollectionsRetrievalCount == 0 ) {
    kDebug() << "All collections retrieved";
    collectionsRetrievalDone();
  }
}

void DavCalendarResource::accessorRetrievedItem( const DavItem &davItem )
{
  kDebug() << "Accessor retrieved an item at " << davItem.url << davItem.etag;

  Akonadi::Item item = createAkonadiItem( davItem.data );

  IncidencePtr ptr = item.payload<IncidencePtr>();
  if ( !ptr.get() )
    return;

  item.setRemoteId( davItem.url );

  if ( davItem.contentType.isEmpty() )
    item.setMimeType( "text/calendar" );
  else
    item.setMimeType( davItem.contentType );

  etagAttribute *attr = item.attribute<etagAttribute>( Item::AddIfMissing );
  attr->setEtag( davItem.etag );

  mAccessor->addItemToCache( davItem );

  mRetrievedItems << item;
}

void DavCalendarResource::accessorRetrievedItems()
{
  kDebug() << "Accessor finished retrieving items";

  QMutexLocker locker( &mRetrievedItemsMutex );

  const Akonadi::Item::List collections = mRetrievedItems;
  mRetrievedItems.clear();

  locker.unlock();

  itemsRetrievedIncremental( collections, Akonadi::Item::List() );
  itemsRetrievalDone();

  if ( ++mItemsRetrievedCount == mSeenCollections.size() ) {
    mAccessor->validateCache();
    mItemsRetrievedCount = 0;
  }
}

void DavCalendarResource::accessorRemovedItem( const KUrl &url )
{
  const QString urlStr = url.prettyUrl();

  QMutexLocker locker( &mDelItemsMutex );

  if ( !mDelItems.contains( urlStr ) ) {
    emit error( i18n( "Unable to find an upload request for URL %1." ).arg( urlStr ) );
    return;
  }

  changeProcessed();

  const Akonadi::Item item = mDelItems[ urlStr ];
  mDelItems.remove( urlStr );
  locker.unlock();

  // Force a refresh of the collection containing the item
  const Akonadi::Collection::Id collectionId = item.storageCollectionId();
  if ( collectionId != -1 )
    synchronizeCollection( collectionId );
  else
    synchronize();
}

void DavCalendarResource::accessorPutItem( const KUrl &oldUrl, DavItem davItem )
{
  const QString oldUrlStr = oldUrl.prettyUrl();
  const QString newUrlStr = davItem.url;

  kDebug() << "Accessor put item at (old) " << oldUrlStr;

  QMutexLocker locker( &mPutItemsMutex );

  if ( !mPutItems.contains( oldUrlStr ) ) {
    emit error( i18n( "Unable to find an upload request for URL %1." ).arg( oldUrlStr ) );
    kDebug() << "Unable to find an upload request for URL " << oldUrlStr;
    return;
  }

  kDebug() << "Item URL changed from " << oldUrlStr << " to " << newUrlStr;

  Akonadi::Item item = mPutItems[ oldUrlStr ];
  mPutItems.remove( oldUrlStr );

  locker.unlock();

  item.setRemoteId( newUrlStr );
  etagAttribute *attr = item.attribute<etagAttribute>( Item::AddIfMissing );
  attr->setEtag( davItem.etag );
  changeCommitted( item );

  // Force a refresh of the collection containing the item
  const Akonadi::Collection::Id collectionId = item.storageCollectionId();
  if ( collectionId != -1 )
    synchronizeCollection( collectionId );
  else
    synchronize();
}

void DavCalendarResource::backendItemsRemoved( const QList<DavItem> &items )
{
  kDebug() << "Got " << items.size() << " items removed on the backend";

  Akonadi::Item::List removedItems;

  foreach ( const DavItem &davItem, items ) {
    kDebug() << "Item at " << davItem.url << " was removed";

    Akonadi::Item item = createAkonadiItem( davItem.data );
    item.setRemoteId( davItem.url );

    if ( davItem.contentType.isEmpty() )
      item.setMimeType( "text/calendar" );
    else
      item.setMimeType( davItem.contentType );

    removedItems << item;
  }

  new Akonadi::ItemDeleteJob( removedItems, this );
}

void DavCalendarResource::accessorStatus( const QString &statusMsg )
{
  emit status( Running, statusMsg );
}

void DavCalendarResource::accessorError( const QString &errorMsg, bool cancelRequest )
{
  if ( cancelRequest )
    cancelTask( errorMsg );

  emit error( errorMsg );
}

void DavCalendarResource::doResourceInitialization()
{
  if ( !configurationIsValid() ) {
    emit status( Broken, i18n( "Resource not configured" ) );
    return;
  }

  delete mAccessor;

  switch( Settings::self()->remoteProtocol() ) {
    case Settings::groupdav:
      mAccessor = new DavAccessor( new GroupdavCalendar() );
      emit status( Idle, i18n( "Using GroupDAV" ) );
      break;
    case Settings::caldav:
      mAccessor = new DavAccessor( new CaldavCalendar() );
      emit status( Idle, i18n( "Using CalDAV" ) );
      break;
    default:
      emit status( Broken, i18n( "No protocol configured" ) );
      return;
  }

  connect( mAccessor, SIGNAL( accessorStatus( const QString& ) ),
           this, SLOT( accessorStatus( const QString& ) ) );
  connect( mAccessor, SIGNAL( accessorError( const QString&, bool ) ),
           this, SLOT( accessorError( const QString&, bool ) ) );

  connect( mAccessor, SIGNAL( collectionRetrieved( const QString&, const QString& ) ),
           this, SLOT( accessorRetrievedCollection( const QString&, const QString& ) ) );
  connect( mAccessor, SIGNAL( collectionsRetrieved() ),
           this, SLOT( accessorRetrievedCollections() ) );

  connect( mAccessor, SIGNAL( itemRetrieved( const DavItem& ) ),
           this, SLOT( accessorRetrievedItem( const DavItem& ) ) );
  connect( mAccessor, SIGNAL( itemsRetrieved() ),
           this, SLOT( accessorRetrievedItems() ) );

  connect( mAccessor, SIGNAL( itemRemoved( const KUrl& ) ),
           this, SLOT( accessorRemovedItem( const KUrl& ) ) );

  connect( mAccessor, SIGNAL( itemPut( const KUrl&, DavItem ) ),
           this, SLOT( accessorPutItem( const KUrl&, DavItem ) ) );

  connect( mAccessor, SIGNAL( backendItemsRemoved( const QList<DavItem>& ) ),
           this, SLOT( backendItemsRemoved( const QList<DavItem>& ) ) );

  if ( mDavCollectionRoot.id() )
    loadCacheFromAkonadi();

  synchronize();
}

void DavCalendarResource::loadCacheFromAkonadi()
{
  CollectionFetchJob *fetchJob = new CollectionFetchJob( mDavCollectionRoot, CollectionFetchJob::FirstLevel );

  if ( fetchJob->exec() ) {
    const Collection::List collections = fetchJob->collections();

    foreach ( const Collection &collection, collections ) {
      ItemFetchJob job( collection );
      job.fetchScope().fetchFullPayload();
      job.fetchScope().fetchAllAttributes();

      if ( job.exec() ) {
        const Item::List items = job.items();
        foreach ( const Item &item, items ) {
          if ( !item.hasPayload<IncidencePtr>() )
            continue;

          if ( !item.hasAttribute<etagAttribute>() )
            continue;

          const IncidencePtr ptr = item.payload<IncidencePtr>();
          KCal::ICalFormat formatter;
          const QByteArray rawData = formatter.toICalString( ptr.get() ).toUtf8();

          const etagAttribute *attr = item.attribute<etagAttribute>();
          const QString etag = attr->etag();

          const DavItem davItem( item.remoteId(), "text/calendar", rawData, etag );
          kDebug() << "Adding item " << davItem.url << davItem.etag << " to accessor cache";
          mAccessor->addItemToCache( davItem );
        }
      }
    }
  }
}

bool DavCalendarResource::configurationIsValid()
{
  if ( !(Settings::self()->remoteProtocol() == Settings::groupdav ||
       Settings::self()->remoteProtocol() == Settings::caldav ) )
    return false;

  if ( Settings::self()->remoteUrls().empty() )
    return false;

  QStringList urls;
  foreach ( const QString &url, Settings::self()->remoteUrls() ) {
    if ( !url.endsWith( '/' ) )
      urls << url + QLatin1Char( '/' );
    else
      urls << url;
  }

  Settings::self()->setRemoteUrls( urls );

  if ( Settings::self()->authenticationRequired() ) {
    if ( Settings::self()->username().isEmpty() )
      return false;

    Settings::self()->askForPassword();

    if ( Settings::self()->password().isEmpty() )
      return false;
  }

  int newICT = Settings::self()->refreshInterval();
  if ( newICT == 0 )
    newICT = -1;

  if ( newICT != mDavCollectionRoot.cachePolicy().intervalCheckTime() ) {
    Akonadi::CachePolicy cachePolicy = mDavCollectionRoot.cachePolicy();
    cachePolicy.setIntervalCheckTime( newICT );

    mDavCollectionRoot.setCachePolicy( cachePolicy );
  }

  if ( !Settings::self()->displayName().isEmpty() ) {
    EntityDisplayAttribute *attribute = mDavCollectionRoot.attribute<EntityDisplayAttribute>( Collection::AddIfMissing );
    attribute->setDisplayName( Settings::self()->displayName() );
  }

  return true;
}

AKONADI_RESOURCE_MAIN( DavCalendarResource )

#include "davcalendarresource.moc"
