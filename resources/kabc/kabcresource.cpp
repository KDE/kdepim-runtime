/*
    Copyright (c) 2008 Kevin Krammer <kevin.krammer@gmx.at>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "kabcresource.h"

#include "kresourceassistant.h"

#include <akonadi/cachepolicy.h>
#include <akonadi/changerecorder.h>
#include <akonadi/entitydisplayattribute.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/mimetypechecker.h>

#include <kabc/addressbook.h>
#include <kabc/addressee.h>
#include <kabc/contactgroup.h>
#include <kabc/distributionlist.h>
#include <kabc/errorhandler.h>
#include <kabc/resource.h>
#include <kabc/resourceabc.h>

#include <kresources/factory.h>
#include <kresources/configdialog.h>

#include <kconfig.h>
#include <kinputdialog.h>
#include <krandom.h>
#include <kwindowsystem.h>

#include <QTimer>

typedef QMap<QString, QString> UidToResourceMap;

using namespace Akonadi;

using KABC::Resource;

class KABCResource::ErrorHandler : public KABC::ErrorHandler
{
  public:
    explicit ErrorHandler( KABCResource* parent )
      : mParent( parent ) {}

    virtual void error( const QString &message ) {
      mLastError = message;
      emit mParent->error( message );
    }

  public:
    KABCResource *mParent;

    QString mLastError;
};

// workaround to access protected method
class KABCResource::AddressBook : public KABC::AddressBook
{
  public:
    AddressBook() {}

    KRES::Manager<KABC::Resource> *getResourceManager()
    {
      return resourceManager();
    }
};

KABCResource::KABCResource( const QString &id )
  : ResourceBase( id ),
    mAddressBook( new AddressBook() ),
    mBaseResource( 0 ),
    mFolderResource( 0 ),
    mErrorHandler( new ErrorHandler( this ) ),
    mFullItemRetrieve( false ),
    mDelayedSaveTimer( new QTimer( this ) ),
    mContactGroupMimeChecker( new MimeTypeChecker() )
{
  KGlobal::locale()->insertCatalog("akonadi_kresourceassistant");

  mAddressBook->setErrorHandler( mErrorHandler );
  connect( this, SIGNAL( reloadConfiguration() ), SLOT( reloadConfiguration() ) );

  connect( mAddressBook, SIGNAL( addressBookChanged( AddressBook* ) ),
           this, SLOT( addressBookChanged() ) );

  connect( mDelayedSaveTimer, SIGNAL( timeout() ),
           this, SLOT( delayedSaveAddressBook() ) );

  changeRecorder()->itemFetchScope().fetchFullPayload();
  changeRecorder()->fetchCollection( true );

  mDelayedSaveTimer->setSingleShot( true );

  mContactGroupMimeChecker->addWantedMimeType( KABC::ContactGroup::mimeType() );
}

KABCResource::~KABCResource()
{
  delete mAddressBook;
  delete mContactGroupMimeChecker;
}

void KABCResource::configure( WId windowId )
{
  KRES::Manager<KABC::Resource> *manager = mAddressBook->getResourceManager();

  if ( mBaseResource != 0 ) {
    emit status( Running,
                 i18nc( "@info:status", "Changing address book plugin configuration" ) );
    KRES::ConfigDialog dlg( 0, QLatin1String( "contact" ), mBaseResource );
    KWindowSystem::setMainWindow( &dlg, windowId );
    if ( dlg.exec() ) {
      setName( mBaseResource->resourceName() );
      manager->writeConfig( KGlobal::config().data() );
    }

    emit status( Idle, QString() );
    // TODO: need to react on name changes, but do we need to react on data changes?
    // as a workaround lets sync the collection tree
    synchronizeCollectionTree();
    return;
  }

  emit status( Running,
               i18nc( "@info:status", "Acquiring address book plugin configuration" ) );
  KResourceAssistant kresAssistant( QLatin1String( "Contact" ) );
  KWindowSystem::setMainWindow( &kresAssistant, windowId );

  connect( &kresAssistant, SIGNAL( error( const QString& ) ),
           this, SIGNAL( error( const QString& ) ) );

  if ( kresAssistant.exec() != QDialog::Accepted ) {
    emit status( Broken, i18nc( "@info:status", "No KDE address book plugin configured yet" ) );
    return;
  }

  KABC::Resource *resource = dynamic_cast<KABC::Resource*>( kresAssistant.resource() );
  Q_ASSERT( resource != 0 );

  setResourcePointers( resource );

  mBaseResource->setAddressBook( mAddressBook );

  manager->writeConfig( KGlobal::config().data() );

  mAddressBook->addResource( mBaseResource );

  if ( !openConfiguration() ) {
    const QString message = i18nc( "@info:status", "Initialization based on newly created configuration failed." );
    emit error( message );
    emit status( Broken, message );
    return;
  }

  if ( !mAddressBook->asyncLoad() ) {
    const QString message = i18nc( "@info:status", "Loading of address book failed." );
    emit error( message );
    emit status( Broken, message );
    return;
  }

  emit status( Running, i18nc( "@info:status", "Loading address book" ) );
}

void KABCResource::retrieveCollections()
{
  kDebug();
  if ( mBaseResource == 0 ) {
    kError() << "No KABC resource";

    const QString message = i18nc( "@info:status", "No KDE address book plugin configured yet" );
    emit error( message );

    emit status( Broken, message );
    return;
  }

  Collection topLevelCollection;
  topLevelCollection.setParent( Collection::root() );
  topLevelCollection.setRemoteId( mBaseResource->identifier() );
  topLevelCollection.setName( mBaseResource->resourceName() );

  EntityDisplayAttribute* attr =
    topLevelCollection.attribute<EntityDisplayAttribute>( Collection::AddIfMissing );
  attr->setDisplayName( mBaseResource->resourceName() );
  attr->setIconName( QLatin1String( "office-address-book" ) );

  QStringList mimeTypes;
  mimeTypes << KABC::Addressee::mimeType();

  QStringList topLevelMimeTypes = mimeTypes;
  topLevelMimeTypes << KABC::ContactGroup::mimeType();

  if ( mFolderResource != 0 ) {
    topLevelMimeTypes << Collection::mimeType();
  }

  Collection::Rights readOnlyRights;

  Collection::Rights readWriteRights;
  readWriteRights |= Collection::CanCreateItem;
  readWriteRights |= Collection::CanChangeItem;
  readWriteRights |= Collection::CanDeleteItem;
  topLevelCollection.setContentMimeTypes( topLevelMimeTypes );
  topLevelCollection.setRights( mBaseResource->readOnly() ? readOnlyRights : readWriteRights );

  Collection::List list;
  list << topLevelCollection;

  if ( mFolderResource != 0 ) {
    const QStringList subResources = mFolderResource->subresources();
    kDebug() << "subResources" << subResources;
    foreach ( const QString &subResource, subResources ) {
      Collection childCollection;
      childCollection.setParent( topLevelCollection );
      childCollection.setRemoteId( subResource );
      childCollection.setName( mFolderResource->subresourceLabel( subResource ) );
      childCollection.setContentMimeTypes( mimeTypes );
      bool readOnly = !mFolderResource->subresourceWritable( subResource );
      childCollection.setRights( readOnly ? readOnlyRights : readWriteRights );

      attr = childCollection.attribute<EntityDisplayAttribute>( Collection::AddIfMissing );
      attr->setDisplayName( childCollection.name() );
      attr->setIconName( QLatin1String( "office-address-book" ) );

      list << childCollection;
    }
  }

  collectionsRetrieved( list );
}

void KABCResource::retrieveItems( const Akonadi::Collection &col )
{
  kDebug() << "full items:" << mFullItemRetrieve;
  const UidToResourceMap uidToResourceMap =
    mFolderResource != 0 ? mFolderResource->uidToResourceMap() : UidToResourceMap();

  Item::List items;

  // check for each addressee whether there is a sub resource mapping.
  // if there is a mapping, skip it if the mapping does not equal the collection's
  // remoteId.
  // if there is none, skip it if the collection is not the top level collection
  KABC::AddressBook::const_iterator addrIt    = mAddressBook->constBegin();
  KABC::AddressBook::const_iterator addrEndIt = mAddressBook->constEnd();
  for ( ; addrIt != addrEndIt; ++addrIt ) {
    UidToResourceMap::const_iterator findIt = uidToResourceMap.find( addrIt->uid() );
    if ( findIt != uidToResourceMap.end() ) {
      if ( findIt.value() != col.remoteId() )
        continue;
    } else {
      if ( col.parent() != Collection::root().id() )
        continue;
    }

    Item item;
    item.setRemoteId( addrIt->uid() );
    item.setMimeType( KABC::Addressee::mimeType() );
    if ( mFullItemRetrieve ) item.setPayload<KABC::Addressee>( *addrIt );
    items.append( item );
  }

  QList<KABC::DistributionList*> distLists = mBaseResource->allDistributionLists();
  foreach ( const KABC::DistributionList *list, distLists ) {
    Item item;
    item.setRemoteId( list->identifier() );
    item.setMimeType( KABC::ContactGroup::mimeType() );

    if ( mFullItemRetrieve ) {
      item.setPayload<KABC::ContactGroup>( contactGroupFromDistList( list ) );
    }

    items.append( item );
  }

  mFullItemRetrieve = false;

  itemsRetrieved( items );
}

bool KABCResource::retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts )
{
  kDebug() << "item id="  << item.id() << ", remoteId=" << item.remoteId()
           << "mimeType=" << item.mimeType() << "part=" << parts;
  Q_UNUSED( parts );
  const QString rid = item.remoteId();

  if ( mContactGroupMimeChecker->isWantedItem( item ) ) {
    KABC::DistributionList *list =
      mAddressBook->findDistributionListByIdentifier( rid );
    if ( list == 0 ) {
      kError() << "No distributionlist with identifier" << rid;
      emit error( i18nc( "@info:status",
                          "Request for data of a specific distribution list failed "
                          "because there is no such list" ) );
      return false;
    }

    Item i( item );
    i.setPayload<KABC::ContactGroup>( contactGroupFromDistList( list ) );
    itemRetrieved( i );
  } else {
    KABC::Addressee addressee = mAddressBook->findByUid( item.remoteId() );
    if ( addressee.isEmpty() ) {
      kError() << "No addressee with uid" << rid;
      emit error( i18nc( "@info:status",
                          "Request for data of a specific address book entry failed "
                          "because there is no such entry" ) );
      return false;
    }

    Item i( item );
    i.setPayload<KABC::Addressee>( addressee );
    itemRetrieved( i );
  }
  return true;
}

void KABCResource::aboutToQuit()
{
  saveAddressBook();
}

void KABCResource::doSetOnline( bool online )
{
  kDebug() << "online" << online << "resource" << (void*) mBaseResource;

  if ( online ) {
    reloadConfiguration();
  } else {
    closeConfiguration();
  }

  ResourceBase::doSetOnline( online );
}

void KABCResource::itemAdded( const Akonadi::Item &item, const Akonadi::Collection& col )
{
  kDebug() << "item id="  << item.id() << ", remoteId=" << item.remoteId()
           << "mimeType=" << item.mimeType();
  // KABC::Resource only has one collection and
  // KABC::ResourceABC does not have API for setting the storage sub resource
  Q_UNUSED( col );

  if ( mBaseResource == 0 ) {
    kError() << "Resource not fully operational yet";
    emit status( Broken, i18nc( "@info:status", "No KDE address book plugin configured yet" ) );
    return;
  }

  kDebug() << "item.hasPayload<Addressee>() " << item.hasPayload<KABC::Addressee>();
  kDebug() << "item.hasPayload<ContactGroup>() " << item.hasPayload<KABC::ContactGroup>();
  if ( item.hasPayload<KABC::Addressee>() ) {
    KABC::Addressee addressee = item.payload<KABC::Addressee>();

    if ( addressee.uid().isEmpty() )
      addressee.setUid( KRandom::randomString( 10 ) );

    addressee.setResource( mBaseResource );
    mAddressBook->insertAddressee( addressee );

    // TODO: proper error reporting
    if ( scheduleSaveAddressBook() ) {
      Item i( item );
      i.setRemoteId( addressee.uid() );
      i.setPayload<KABC::Addressee>( addressee );

      changeCommitted( i );
      return;
    }
  } else if ( item.hasPayload<KABC::ContactGroup>() ) {
    KABC::ContactGroup contactGroup = item.payload<KABC::ContactGroup>();

    if ( contactGroup.id().isEmpty() )
      contactGroup.setId( KRandom::randomString( 10 ) );

    // also inserts list into resource
    distListFromContactGroup( contactGroup );

    // TODO: proper error reporting
    if ( scheduleSaveAddressBook() ) {
      Item i( item );
      i.setRemoteId( contactGroup.id() );
      i.setPayload<KABC::ContactGroup>( contactGroup );

      changeCommitted( i );
      return;
    }
  }

  changeProcessed();
}

void KABCResource::itemChanged( const Akonadi::Item &item, const QSet<QByteArray>& parts )
{
  kDebug() << "item id="  << item.id() << ", remoteId=" << item.remoteId()
           << "mimeType=" << item.mimeType();
  // we store the whole addressee/contactgroup anyway
  Q_UNUSED( parts );

  if ( mBaseResource == 0 ) {
    kError() << "Resource not fully operational yet";
    emit status( Broken, i18nc( "@info:status", "No KDE address book plugin configured yet" ) );
    return;
  }

  kDebug() << "item.hasPayload<Addressee>() " << item.hasPayload<KABC::Addressee>();
  kDebug() << "item.hasPayload<ContactGroup>() " << item.hasPayload<KABC::ContactGroup>();
  if ( item.hasPayload<KABC::Addressee>() ) {
    KABC::Addressee addressee = item.payload<KABC::Addressee>();
    Q_ASSERT( !addressee.uid().isEmpty() );

    addressee.setResource( mBaseResource );
    mAddressBook->insertAddressee( addressee );

    // TODO: proper error reporting
    if ( scheduleSaveAddressBook() ) {
      changeCommitted( item );
      return;
    }
  } else if ( item.hasPayload<KABC::ContactGroup>() ) {
    KABC::ContactGroup contactGroup = item.payload<KABC::ContactGroup>();
    Q_ASSERT( !contactGroup.id().isEmpty() );

    KABC::DistributionList *list =
      mAddressBook->findDistributionListByIdentifier( contactGroup.id() );
    if ( list == 0 ) {
      // TODO: rather report an error?

      // also inserts list into resource
      distListFromContactGroup( contactGroup );
    } else {
      // TODO: might be better to update the already existing instance
      mBaseResource->removeDistributionList( list );
      delete list;

      // also inserts list into resource
      distListFromContactGroup( contactGroup );
    }

    // TODO: proper error reporting
    if ( scheduleSaveAddressBook() ) {
      changeCommitted( item );
      return;
    }
  }

  changeProcessed();
}

void KABCResource::itemRemoved( const Akonadi::Item &item )
{
  kDebug() << "item id="  << item.id() << ", remoteId=" << item.remoteId()
           << "mimeType=" << item.mimeType();

  if ( mBaseResource == 0 ) {
    kError() << "Resource not fully operational yet";
    emit status( Broken, i18nc( "@info:status", "No KDE address book plugin configured yet" ) );
    return;
  }

  if ( mContactGroupMimeChecker->isWantedItem( item ) ) {
    KABC::DistributionList *list =
      mAddressBook->findDistributionListByIdentifier( item.remoteId() );
    if ( list != 0 ) {
      mAddressBook->removeDistributionList( list );
      delete list;

      // TODO: proper error reporting
      if ( scheduleSaveAddressBook() ) {
        changeCommitted( item );
        return;
      }
    }
  } else {
    KABC::Addressee addressee = mAddressBook->findByUid( item.remoteId() );
    if ( !addressee.isEmpty() ) {
      mAddressBook->removeAddressee( addressee );
      // TODO: proper error reporting
      if ( scheduleSaveAddressBook() ) {
        changeCommitted( item );
        return;
      }
    }
  }
  changeProcessed();
}

void KABCResource::collectionChanged( const Akonadi::Collection &collection )
{
  kDebug() << "collection.name=" << collection.name()
           << ", resource name=" << mBaseResource->resourceName();

  if ( mBaseResource == 0 ) {
    kError() << "Resource not fully operational yet";
    emit status( Broken, i18nc( "@info:status", "No KDE address book plugin configured yet" ) );
    return;
  }

  if ( collection.parent() == Collection::root().id() ) {
    QString newName = collection.name();
    if ( collection.hasAttribute<EntityDisplayAttribute>() ) {
      EntityDisplayAttribute *attr = collection.attribute<EntityDisplayAttribute>();
      if ( !attr->displayName().isEmpty() )
        newName = attr->displayName();
    }

    if ( newName != mBaseResource->resourceName() ) {
      mBaseResource->setResourceName( newName );
      setName( newName );
      mAddressBook->getResourceManager()->writeConfig( KGlobal::config().data() );
    }
  } else
    kWarning() << "Got collection change for a sub resource which we cannot change";

  changeCommitted( collection );
}

void KABCResource::setResourcePointers( KABC::Resource *resource )
{
  mBaseResource   = resource;
  mFolderResource = dynamic_cast<KABC::ResourceABC*>( resource );

  if ( mBaseResource != 0 )
    mBaseResource->setAddressBook( mAddressBook );
}

bool KABCResource::openConfiguration()
{
  if ( mBaseResource != 0 ) {
    if ( !mBaseResource->isOpen() ) {
      if ( !mBaseResource->open() ) {
        kError() << "Opening resource" << mBaseResource->identifier() << "failed";
        return false;
      }
    }

    connect( mBaseResource, SIGNAL( loadingError( Resource*, const QString& ) ),
             this, SLOT( loadingError( Resource*, const QString& ) ) );

    connect( mBaseResource, SIGNAL( loadingFinished( Resource* ) ),
             this, SLOT( initialLoadingFinished( Resource* ) ) );

    if ( mFolderResource != 0 ) {
        connect( mFolderResource,
                 SIGNAL( signalSubresourceAdded( KABC::ResourceABC*, const QString&, const QString& ) ),
                 this, SLOT( subResourceAdded( KABC::ResourceABC*, const QString&, const QString& ) ) );

        connect( mFolderResource,
                 SIGNAL( signalSubresourceRemoved( KABC::ResourceABC*, const QString&, const QString& ) ),
                 this, SLOT( subResourceRemoved( KABC::ResourceABC*, const QString&, const QString& ) ) );

        connect( mFolderResource,
                 SIGNAL( signalSubresourceChanged( KABC::ResourceABC*, const QString&, const QString& ) ),
                 this, SLOT( subResourceChanged( KABC::ResourceABC*, const QString&, const QString& ) ) );
    }
  }

  // do not react on addressbook changes until we have finished its initial loading
  mAddressBook->blockSignals( true );

  setName( mBaseResource->resourceName() );

  return true;
}

void KABCResource::closeConfiguration()
{
  mDelayedSaveTimer->stop();

  // do not react on addressbook changes until we have finished its initial loading
  mAddressBook->blockSignals( true );

  if ( mBaseResource != 0 ) {
    disconnect( mBaseResource, SIGNAL( loadingError( Resource*, const QString& ) ),
                this, SLOT( loadingError( Resource*, const QString& ) ) );

    disconnect( mBaseResource, SIGNAL( loadingFinished( Resource* ) ),
                this, SLOT( initialLoadingFinished( Resource* ) ) );

    if ( mFolderResource != 0 ) {
        disconnect( mFolderResource,
                    SIGNAL( signalSubresourceAdded( KABC::ResourceABC*, const QString&, const QString& ) ),
                    this, SLOT( subResourceAdded( KABC::ResourceABC*, const QString&, const QString& ) ) );

        disconnect( mFolderResource,
                    SIGNAL( signalSubresourceRemoved( KABC::ResourceABC*, const QString&, const QString& ) ),
                    this, SLOT( subResourceRemoved( KABC::ResourceABC*, const QString&, const QString& ) ) );

        disconnect( mFolderResource,
                    SIGNAL( signalSubresourceChanged( KABC::ResourceABC*, const QString&, const QString& ) ),
                    this, SLOT( subResourceChanged( KABC::ResourceABC*, const QString&, const QString& ) ) );
    }

    if ( mBaseResource->isOpen() )
      mBaseResource->close();
  }
}

bool KABCResource::saveAddressBook()
{
  mDelayedSaveTimer->stop();

  if ( !mBaseResource || mBaseResource->readOnly() )
    return false;

  mErrorHandler->mLastError.clear();

  KABC::Ticket *ticket = mAddressBook->requestSaveTicket();
  if ( ticket == 0 ) {
    kError() << "Could not get address book save ticket";
    emit error( i18nc( "@info:status",
                       "Request for saving the address book failed. "
                       "Probably locked by another application" ) );
    return false;
  }

  if ( !mAddressBook->save(ticket) ) {
    kError() << "Saving failed: " << mErrorHandler->mLastError;
    mAddressBook->releaseSaveTicket( ticket );
    return false;
  }

  kDebug() << "Saving succeeded";
  return true;
}

bool KABCResource::scheduleSaveAddressBook()
{
  if ( !mBaseResource || mBaseResource->readOnly() )
    return false;

  if ( !mDelayedSaveTimer->isActive() )
    mDelayedSaveTimer->start( 5000 );

  return true;
}

void KABCResource::loadingError( KABC::Resource *resource, const QString &message )
{
  Q_UNUSED( resource );

  kError() << "Loading error: " << message;
  emit error( message );
  emit status( Broken, message );
}

void KABCResource::initialLoadingFinished( KABC::Resource *resource )
{
  kDebug() << resource;
  Q_ASSERT( mBaseResource != 0 );
  Q_ASSERT( resource == mBaseResource );


  disconnect( mBaseResource, SIGNAL( loadingFinished( Resource* ) ),
              this, SLOT( initialLoadingFinished( Resource* ) ) );

  emit status( Idle, QString() );

  mAddressBook->blockSignals( false );

  // Let Akonadi retrieve all items but only the base information, no payloads yet
  mFullItemRetrieve = false;
  synchronize();
}

void KABCResource::addressBookChanged()
{
  kDebug();
  if ( mDelayedSaveTimer->isActive() ) {
    // TODO should record changes for delayed saving
    kError() << "Delayed saving scheduled when resource changed. We might have lost changes";
    mDelayedSaveTimer->stop();
  }
  // FIXME: there must be a better way to do this
  mFullItemRetrieve = true;
  synchronize();
}

void KABCResource::subResourceAdded( KABC::ResourceABC *resource,
        const QString &type, const QString &subResource )
{
  kDebug() << "subResource" << subResource;
  Q_ASSERT( resource == mFolderResource );
  Q_ASSERT( type.toLower() == QLatin1String( "contact" ) );
  Q_ASSERT( !subResource.isEmpty() );

  // synchronizeCollectionTree just in case the resource does not make
  // the address book emit addressBookChanged()
  synchronizeCollectionTree();
}

void KABCResource::subResourceRemoved( KABC::ResourceABC *resource,
        const QString &type, const QString &subResource )
{
  kDebug() << "subResource" << subResource;
  Q_ASSERT( resource == mFolderResource );
  Q_ASSERT( type.toLower() == QLatin1String( "contact" ) );
  Q_ASSERT( !subResource.isEmpty() );

  // synchronizeCollectionTree just in case the resource does not make
  // the address book emit addressBookChanged()
  synchronizeCollectionTree();
}

void KABCResource::subResourceChanged( KABC::ResourceABC *resource,
        const QString &type, const QString &subResource )
{
  kDebug() << "subResource" << subResource;
  Q_ASSERT( resource == mFolderResource );
  Q_ASSERT( type.toLower() == QLatin1String( "contact" ) );
  Q_ASSERT( !subResource.isEmpty() );

  // synchronizeCollectionTree just in case the resource does not make
  // the address book emit addressBookChanged()
  synchronizeCollectionTree();
}

void KABCResource::reloadConfiguration()
{
  if ( mDelayedSaveTimer->isActive() ) {
    if ( !saveAddressBook() ) {
      kError() << "Saving of address book failed:" << mErrorHandler->mLastError;
    }
  }
  closeConfiguration();

  KGlobal::config()->reparseConfiguration();

  if ( KGlobal::config()->groupList().isEmpty() ) {
    emit status( Broken, i18nc( "@info:status", "No KDE address book plugin configured yet" ) );
    return;
  }

  Q_ASSERT( KGlobal::config().data() != 0);

  KRES::Manager<KABC::Resource> *manager = mAddressBook->getResourceManager();
  manager->readConfig( KGlobal::config().data() );

  KRES::Resource *resource = manager->standardResource();
  if ( resource != 0 ) {
    if ( resource->type().toLower() == QLatin1String( "akonadi" ) ) {
      kError() << "Resource config points to an Akonadi bridge resource";
      emit status( Broken, i18nc( "@info:status", "No KDE address book plugin configured yet" ) );
      return;
    }
  }

  setResourcePointers( manager->standardResource() );
  if ( mBaseResource == 0 ) {
    emit status( Broken, i18nc( "@info:status", "No KDE address book plugin configured yet" ) );
    return;
  }

  if ( !isOnline() )
    return;

  if ( !openConfiguration() ) {
    kError() << "openConfiguration() failed";

    const QString message = i18nc( "@info:status", "Initialization based on stored configuration failed." );
    emit error( message );
    emit status( Broken, message );
    return;
  }

  if ( !mAddressBook->asyncLoad() ) {
    kError() << "asyncLoad() failed";

    const QString message = i18nc( "@info:status", "Loading of address book failed." );
    emit error( message );
    emit status( Broken, message );
    return;
  }

  emit status( Running, i18nc( "@info:status", "Loading address book" ) );
}

void KABCResource::delayedSaveAddressBook()
{
  if ( !saveAddressBook() ) {
    kError() << "Saving failed, rescheduling delayed save. Error was: "
             << mErrorHandler->mLastError;
    if ( !scheduleSaveAddressBook() ) {
      kError() << "Scheduling failed as well, giving up";
    }
  }
}

KABC::DistributionList *KABCResource::distListFromContactGroup( const KABC::ContactGroup& contactGroup )
{
  KABC::DistributionList *list =
    new KABC::DistributionList( mBaseResource, contactGroup.id(), contactGroup.name() );

  for ( unsigned int refIndex = 0; refIndex < contactGroup.contactReferenceCount(); ++refIndex ) {
    const KABC::ContactGroup::ContactReference &reference = contactGroup.contactReference( refIndex );

    KABC::Addressee addressee = mBaseResource->findByUid( reference.uid() );
    if ( addressee.isEmpty() ) {
      addressee.setUid( reference.uid() );
      // TODO any way to set a good name?
    }

    // TODO how to handle ContactGroup::ContactReference custom fields?

    list->insertEntry( addressee, reference.preferredEmail() );
  }

  for ( unsigned int dataIndex = 0; dataIndex < contactGroup.dataCount(); ++dataIndex ) {
    const KABC::ContactGroup::Data &data = contactGroup.data( dataIndex );

    KABC::Addressee addressee;
    addressee.setName( data.name() );
    addressee.insertEmail( data.email() );

    // TODO how to handle ContactGroup::Data custom fields?

    list->insertEntry( addressee );
  }

  return list;
}

KABC::ContactGroup KABCResource::contactGroupFromDistList( const KABC::DistributionList* list ) const
{
  kDebug() << "name=" << list->name() << ", identifier=" << list->identifier()
           << ", entries.count=" << list->entries().count();

  KABC::ContactGroup contactGroup( list->name() );
  contactGroup.setId( list->identifier() );

  KABC::DistributionList::Entry::List entries = list->entries();
  foreach ( const KABC::DistributionList::Entry &entry, entries ) {
    KABC::Addressee addressee = entry.addressee();
    const QString email = entry.email();
    if ( addressee.isEmpty() ) {
      if ( email.isEmpty() )
        continue;

      KABC::ContactGroup::Data data( email, email );
      contactGroup.append( data );
    } else {
      KABC::Addressee baseAddressee = mBaseResource->findByUid( addressee.uid() );
      if ( baseAddressee.isEmpty() ) {
        KABC::ContactGroup::Data data( email, email );
        // TODO: transer custom fields?
        contactGroup.append( data );
      } else {
        KABC::ContactGroup::ContactReference reference( addressee.uid() );
        reference.setPreferredEmail( email );
        // TODO: transer custom fields?
        contactGroup.append( reference );
      }
    }
  }

  return contactGroup;
}

AKONADI_RESOURCE_MAIN( KABCResource )

#include "kabcresource.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
