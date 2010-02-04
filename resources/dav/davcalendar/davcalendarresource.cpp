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

#include "configdialog.h"
#include "davcollectionsfetchjob.h"
#include "davcollectionsmultifetchjob.h"
#include "davitemcreatejob.h"
#include "davitemdeletejob.h"
#include "davitemfetchjob.h"
#include "davitemmodifyjob.h"
#include "davitemslistjob.h"
#include "davmanager.h"
#include "etagattribute.h"
#include "settings.h"
#include "settingsadaptor.h"

#include <akonadi/attributefactory.h>
#include <akonadi/cachepolicy.h>
#include <akonadi/changerecorder.h>
#include <akonadi/collectionfetchscope.h>
#include <akonadi/entitydisplayattribute.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/kcal/incidencemimetypevisitor.h>
#include <boost/shared_ptr.hpp>
#include <kabc/addressee.h>
#include <kabc/vcardconverter.h>
#include <kcal/icalformat.h>
#include <kcal/incidence.h>
#include <kwindowsystem.h>

#include <QtDBus/QDBusConnection>

using namespace Akonadi;

typedef boost::shared_ptr<KCal::Incidence> IncidencePtr;

DavCalendarResource::DavCalendarResource( const QString &id )
  : ResourceBase( id ), mMimeVisitor( new Akonadi::IncidenceMimeTypeVisitor )
{
  new SettingsAdaptor( Settings::self() );
  QDBusConnection::sessionBus().registerObject( QLatin1String( "/Settings" ),
                                                Settings::self(), QDBusConnection::ExportAdaptors );

  AttributeFactory::registerAttribute<ETagAttribute>();
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
  changeRecorder()->itemFetchScope().fetchAttribute<ETagAttribute>();
  changeRecorder()->itemFetchScope().setAncestorRetrieval( ItemFetchScope::All );

  Settings::self()->setWinId( winIdForDialogs() );

  switch( Settings::self()->remoteProtocol() ) {
    case Settings::groupdav:
      DavManager::self()->setProtocol( DavManager::GroupDav );
      break;
    case Settings::caldav:
      DavManager::self()->setProtocol( DavManager::CalDav );
      break;
    default:
      break;
  }

  DavManager::self()->setUser( Settings::self()->username() );
  DavManager::self()->setPassword( Settings::self()->password() );
}

DavCalendarResource::~DavCalendarResource()
{
  delete mMimeVisitor;
}

void DavCalendarResource::configure( WId windowId )
{
  ConfigDialog dialog;
  Settings::self()->setWinId( windowId );

  if ( windowId )
    KWindowSystem::setMainWindow( &dialog, windowId );

  const int result = dialog.exec();

  if ( result == QDialog::Accepted ) {
    Settings::self()->writeConfig();

    switch( Settings::self()->remoteProtocol() ) {
      case Settings::groupdav:
        DavManager::self()->setProtocol( DavManager::GroupDav );
        emit status( Idle, i18n( "Using GroupDAV" ) );
        break;
      case Settings::caldav:
        DavManager::self()->setProtocol( DavManager::CalDav );
        emit status( Idle, i18n( "Using CalDAV" ) );
        break;
      case Settings::carddav:
        DavManager::self()->setProtocol( DavManager::CardDav );
        emit status( Idle, i18n( "Using CardDAV" ) );
        break;
      default:
        emit status( Broken, i18n( "No protocol configured" ) );
        break;
    }

    DavManager::self()->setUser( Settings::self()->username() );
    DavManager::self()->setPassword( Settings::self()->password() );

    clearCache();
    synchronize();

    emit configurationDialogAccepted();
  } else {
    emit configurationDialogRejected();
  }
}

void DavCalendarResource::retrieveCollections()
{
  kDebug() << "Retrieving collections list";

  if ( !configurationIsValid() ) {
    emit status( Broken, i18n( "The resource is not configured yet" ) );
    cancelTask( i18n( "The resource is not configured yet" ) );
    return;
  }

  emit status( Running, i18n( "Fetching collections" ) );

  DavCollectionsMultiFetchJob *job = new DavCollectionsMultiFetchJob( Settings::self()->remoteUrls() );
  connect( job, SIGNAL( result( KJob* ) ), SLOT( onRetrieveCollectionsFinished( KJob* ) ) );
  job->start();
}

void DavCalendarResource::retrieveItems( const Akonadi::Collection &collection )
{
  kDebug() << "Retrieving items";

  if ( !configurationIsValid() ) {
    emit status( Broken, i18n( "The resource is not configured yet" ) );
    cancelTask( i18n( "The resource is not configured yet" ) );
    return;
  }

  DavCollection davCollection;
  davCollection.setUrl( collection.remoteId() );

  DavItemsListJob *job = new DavItemsListJob( davCollection );
  job->setProperty( "collection", QVariant::fromValue( collection ) );
  connect( job, SIGNAL( result( KJob* ) ), SLOT( onRetrieveItemsFinished( KJob* ) ) );
  job->start();
}

bool DavCalendarResource::retrieveItem( const Akonadi::Item &item, const QSet<QByteArray>& )
{
  kDebug() << "Retrieving single item. Remote id = " << item.remoteId();

  if ( !configurationIsValid() ) {
    emit status( Broken, i18n( "The resource is not configured yet" ) );
    cancelTask( i18n( "The resource is not configured yet" ) );
    return false;
  }

  DavItem davItem;
  davItem.setUrl( item.remoteId() );
  davItem.setContentType( "text/calendar" );
  davItem.setEtag( item.attribute<ETagAttribute>()->etag() );

  DavItemFetchJob *job = new DavItemFetchJob( davItem );
  job->setProperty( "item", QVariant::fromValue( item ) );
  connect( job, SIGNAL( result( KJob* ) ), SLOT( onRetrieveItemFinished( KJob* ) ) );
  job->start();

  return true;
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

  const QString basePath = collection.remoteId();
  if ( basePath.isEmpty() ) {
    kError() << "Invalid remote id for collection " << collection.id() << " = " << collection.remoteId();
    cancelTask( i18n( "Invalid collection for item %1." ).arg( item.id() ) );
    return;
  }

  KUrl url;
  QByteArray rawData;
  QString mimeType;

  if ( item.hasPayload<KABC::Addressee>() ) {
    const KABC::Addressee contact = item.payload<KABC::Addressee>();

    const QString fileName = contact.uid();
    if ( fileName.isEmpty() ) {
      kError() << "Invalid contact uid";
      cancelTask( i18n( "Client did not create a UID for item %1." ).arg( item.id() ) );
      return;
    }

    url = KUrl( basePath + fileName + ".vcf" );
    mimeType = KABC::Addressee::mimeType();

    KABC::VCardConverter converter;
    rawData = converter.createVCard( contact );
  } else if ( item.hasPayload<IncidencePtr>() ) {
    const IncidencePtr ptr = item.payload<IncidencePtr>();

    const QString fileName = ptr->uid();
    if ( fileName.isEmpty() ) {
      kError() << "Invalid incidence uid";
      cancelTask( i18n( "Client did not create a UID for item %1." ).arg( item.id() ) );
      return;
    }

    url = KUrl( basePath + fileName + ".ics" );
    mimeType = "text/calendar";

    KCal::ICalFormat formatter;
    rawData = formatter.toICalString( ptr.get() ).toUtf8();
  } else {
    kError() << "Item " << item.id() << " doesn't has a valid payload";
    cancelTask( i18n( "Unable to retrieve added item %1." ).arg( item.id() ) );
    return;
  }

  url.setUser( Settings::self()->username() );

  const QString urlStr = url.prettyUrl();
  kDebug() << "Item " << item.id() << " will be put to " << urlStr;

  DavItem davItem;
  davItem.setUrl( urlStr );
  davItem.setContentType( mimeType );
  davItem.setData( rawData );

  DavItemCreateJob *job = new DavItemCreateJob( davItem );
  job->setProperty( "item", QVariant::fromValue( item ) );
  connect( job, SIGNAL( result( KJob* ) ), SLOT( onItemAddedFinished( KJob* ) ) );
  job->start();
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

  QByteArray rawData;
  QString mimeType;

  if ( item.hasPayload<KABC::Addressee>() ) {
    const KABC::Addressee contact = item.payload<KABC::Addressee>();

    KABC::VCardConverter converter;
    rawData = converter.createVCard( contact );
    mimeType = KABC::Addressee::mimeType();
  } else if ( item.hasPayload<IncidencePtr>() ) {
    const IncidencePtr ptr = item.payload<IncidencePtr>();

    KCal::ICalFormat formatter;
    rawData = formatter.toICalString( ptr.get() ).toUtf8();
    mimeType = "text/calendar";
  } else {
    kError() << "Item " << item.id() << " doesn't has a valid payload";
    cancelTask( i18n( "Unable to retrieve added item %1." ).arg( item.id() ) );
    return;
  }

  DavItem davItem;
  davItem.setUrl( item.remoteId() );
  davItem.setEtag( item.attribute<ETagAttribute>()->etag() );
  davItem.setContentType( mimeType );
  davItem.setData( rawData );

  DavItemModifyJob *job = new DavItemModifyJob( davItem );
  job->setProperty( "item", QVariant::fromValue( item ) );
  connect( job, SIGNAL( result( KJob* ) ), SLOT( onItemChangedFinished( KJob* ) ) );
  job->start();
}

void DavCalendarResource::itemRemoved( const Akonadi::Item &item )
{
  if ( !configurationIsValid() ) {
    emit status( Broken, i18n( "The resource is not configured yet" ) );
    cancelTask( i18n( "The resource is not configured yet" ) );
    return;
  }

  DavItem davItem;
  davItem.setUrl( item.remoteId() );
  davItem.setEtag( item.attribute<ETagAttribute>()->etag() );

  DavItemDeleteJob *job = new DavItemDeleteJob( davItem );
  connect( job, SIGNAL( result( KJob* ) ), SLOT( onItemRemovedFinished( KJob* ) ) );
  job->start();
}

void DavCalendarResource::onRetrieveCollectionsFinished( KJob *job )
{
  if ( job->error() ) {
    cancelTask( i18n( "Unable to retrieve collections: %1", job->errorText() ) );
    return;
  }

  const DavCollectionsMultiFetchJob *fetchJob = qobject_cast<DavCollectionsMultiFetchJob*>( job );

  Akonadi::Collection::List collections;
  collections << mDavCollectionRoot;

  const DavCollection::List davCollections = fetchJob->collections();
  foreach ( const DavCollection &davCollection, davCollections ) {
    Akonadi::Collection collection;
    collection.setParentCollection( mDavCollectionRoot );
    collection.setRemoteId( davCollection.url() );
    if ( davCollection.displayName().isEmpty() )
      collection.setName( name() + " (" + davCollection.url() + ")" );
    else
      collection.setName( davCollection.displayName() + " (" + davCollection.url() + ")" );

    QStringList mimeTypes;

    const DavCollection::ContentTypes contentTypes = davCollection.contentTypes();
    if ( contentTypes & DavCollection::Events )
      mimeTypes << IncidenceMimeTypeVisitor::eventMimeType();

    if ( contentTypes & DavCollection::Todos )
      mimeTypes << IncidenceMimeTypeVisitor::todoMimeType();

    if ( contentTypes & DavCollection::Contacts )
      mimeTypes << KABC::Addressee::mimeType();

    // CalDAV collections can contain events and todos, however in onRetrieveItemsFinished() we
    // can't figure out what type of items are provided. For this case we'll set the
    // temporary mime type to 'text/calendar' and fix it in onRetrieveItemFinished() where we can
    // find out the mime type from the payload. The Akonadi collections from CalDAV need content
    // mime type 'text/calendar' to temporary store the items.
    if ( (contentTypes & DavCollection::Events) && (contentTypes & DavCollection::Todos) ) // only valid for CalDAV
      mimeTypes << QLatin1String( "text/calendar" );

    collection.setContentMimeTypes( mimeTypes );

    collections << collection;
  }

  collectionsRetrieved( collections );
}

void DavCalendarResource::onRetrieveItemsFinished( KJob *job )
{
  if ( job->error() ) {
    cancelTask( i18n( "Unable to retrieve items: %1", job->errorText() ) );
    return;
  }

  const Collection collection = job->property( "collection" ).value<Collection>();

  const DavItemsListJob *listJob = qobject_cast<DavItemsListJob*>( job );

  Akonadi::Item::List items;

  const DavItem::List davItems = listJob->items();
  foreach ( const DavItem &davItem, davItems ) {
    Akonadi::Item item;
    item.setRemoteId( davItem.url() );

    const QStringList contentMimeTypes = collection.contentMimeTypes();
    if ( contentMimeTypes.contains( KABC::Addressee::mimeType() ) )
      item.setMimeType( KABC::Addressee::mimeType() );
    else if ( contentMimeTypes.contains( "text/calendar" ) )
      item.setMimeType( "text/calendar" );
    else if ( contentMimeTypes.contains( IncidenceMimeTypeVisitor::eventMimeType() ) )
      item.setMimeType( IncidenceMimeTypeVisitor::eventMimeType() );
    else if ( contentMimeTypes.contains( IncidenceMimeTypeVisitor::todoMimeType() ) )
      item.setMimeType( IncidenceMimeTypeVisitor::todoMimeType() );

    ETagAttribute *attr = item.attribute<ETagAttribute>( Item::AddIfMissing );
    attr->setEtag( davItem.etag() );

    items << item;
  }

  itemsRetrieved( items );
}

void DavCalendarResource::onRetrieveItemFinished( KJob *job )
{
  if ( job->error() ) {
    cancelTask( i18n( "Unable to retrieve item: %1", job->errorText() ) );
    return;
  }

  const DavItemFetchJob *fetchJob = qobject_cast<DavItemFetchJob*>( job );

  const DavItem davItem = fetchJob->item();

  Akonadi::Item item = fetchJob->property( "item" ).value<Akonadi::Item>();

  // convert dav data into payload
  const QString data = QString::fromUtf8( davItem.data() );

  if ( item.mimeType() == KABC::Addressee::mimeType() ) {
    KABC::VCardConverter converter;
    const KABC::Addressee contact = converter.parseVCard( davItem.data() );

    item.setPayload<KABC::Addressee>( contact );
  } else {
    KCal::ICalFormat formatter;
    const IncidencePtr ptr( formatter.fromString( data ) );

    item.setPayload<IncidencePtr>( ptr );

    // fix mime type for CalDAV collections
    ptr->accept( *mMimeVisitor );
    item.setMimeType( mMimeVisitor->mimeType() );
  }

  // update etag
  item.attribute<ETagAttribute>( Item::AddIfMissing )->setEtag( davItem.etag() );

  itemRetrieved( item );
}

void DavCalendarResource::onItemAddedFinished( KJob *job )
{
  if ( job->error() ) {
    cancelTask( i18n( "Unable to add item: %1", job->errorText() ) );
    return;
  }

  const DavItemCreateJob *createJob = qobject_cast<DavItemCreateJob*>( job );

  const DavItem davItem = createJob->item();

  Akonadi::Item item = createJob->property( "item" ).value<Akonadi::Item>();
  item.setRemoteId( davItem.url() );
  item.attribute<ETagAttribute>( Item::AddIfMissing )->setEtag( davItem.etag() );

  changeCommitted( item );
}

void DavCalendarResource::onItemChangedFinished( KJob *job )
{
  if ( job->error() ) {
    cancelTask( i18n( "Unable to change item: %1", job->errorText() ) );
    return;
  }

  const DavItemModifyJob *modifyJob = qobject_cast<DavItemModifyJob*>( job );

  const DavItem davItem = modifyJob->item();

  Akonadi::Item item = modifyJob->property( "item" ).value<Akonadi::Item>();
  item.attribute<ETagAttribute>( Item::AddIfMissing )->setEtag( davItem.etag() );

  changeCommitted( item );
}

void DavCalendarResource::onItemRemovedFinished( KJob *job )
{
  if ( job->error() ) {
    cancelTask( i18n( "Unable to remove item: %1", job->errorText() ) );
    return;
  }

  changeProcessed();
}

void DavCalendarResource::doResourceInitialization()
{
  if ( !configurationIsValid() ) {
    emit status( Broken, i18n( "Resource not configured" ) );
    return;
  }

  switch( Settings::self()->remoteProtocol() ) {
    case Settings::groupdav:
      DavManager::self()->setProtocol( DavManager::GroupDav );
      emit status( Idle, i18n( "Using GroupDAV" ) );
      break;
    case Settings::caldav:
      DavManager::self()->setProtocol( DavManager::CalDav );
      emit status( Idle, i18n( "Using GroupDAV" ) );
      emit status( Idle, i18n( "Using CalDAV" ) );
      break;
    default:
      emit status( Broken, i18n( "No protocol configured" ) );
      return;
  }

  synchronize();
}

bool DavCalendarResource::configurationIsValid()
{
  if ( !(Settings::self()->remoteProtocol() == Settings::groupdav ||
         Settings::self()->remoteProtocol() == Settings::caldav ||
         Settings::self()->remoteProtocol() == Settings::carddav ) )
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
