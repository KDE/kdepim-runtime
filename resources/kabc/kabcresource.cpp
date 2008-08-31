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
#include <akonadi/itemfetchscope.h>

#include <kabc/addressbook.h>
#include <kabc/addressee.h>
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
    mDelayedSaveTimer( new QTimer( this ) )
{
  mAddressBook->setErrorHandler( mErrorHandler );
  connect( this, SIGNAL( reloadConfiguration() ), SLOT( reloadConfiguration() ) );

  connect( mAddressBook, SIGNAL( addressBookChanged( AddressBook* ) ),
           this, SLOT( addressBookChanged() ) );

  connect( mDelayedSaveTimer, SIGNAL( timeout() ),
           this, SLOT( delayedSaveAddressBook() ) );

  changeRecorder()->itemFetchScope().fetchFullPayload();
  changeRecorder()->fetchCollection( true );

  mDelayedSaveTimer->setSingleShot( true );
}

KABCResource::~KABCResource()
{
  delete mAddressBook;
}

void KABCResource::configure( WId windowId )
{
  KRES::Manager<KABC::Resource> *manager = mAddressBook->getResourceManager();

  if ( mBaseResource != 0 ) {
  emit status( Running,
               i18nc( "@info:status", "Changing address book plugin configuration" ) );
    KRES::ConfigDialog dlg( 0, QLatin1String( "contact" ), mBaseResource );
    KWindowSystem::setMainWindow( &dlg, windowId );
    if ( dlg.exec() )
      manager->writeConfig( KGlobal::config().data() );

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

  QStringList mimeTypes;
  mimeTypes << QLatin1String( "text/directory" );

  QStringList topLevelMimeTypes = mimeTypes;

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
    foreach( const QString& subResource, subResources ) {
      Collection childCollection;
      childCollection.setParent( topLevelCollection );
      childCollection.setRemoteId( subResource );
      childCollection.setName( mFolderResource->subresourceLabel( subResource ) );
      childCollection.setContentMimeTypes( mimeTypes );
      bool readOnly = !mFolderResource->subresourceWritable( subResource );
      childCollection.setRights( readOnly ? readOnlyRights : readWriteRights );

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
  KABC::AddressBook::const_iterator addrIt    = mAddressBook->begin();
  KABC::AddressBook::const_iterator addrEndIt = mAddressBook->end();
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
    item.setMimeType( QLatin1String( "text/directory" ) );
    if ( mFullItemRetrieve ) item.setPayload<KABC::Addressee>( *addrIt );
    items.append( item );
  }

  // TODO: handle distribution lists once we have an Akonadi payload for them

  mFullItemRetrieve = false;

  itemsRetrieved( items );
}

bool KABCResource::retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts )
{
  kDebug() << "item id="  << item.id() << ", remoteId=" << item.remoteId()
           << "mimeType=" << item.mimeType() << "part=" << parts;
  Q_UNUSED( parts );
  const QString rid = item.remoteId();

  // TODO: handle distribution lists once we have an Akonadi payload for them

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

  kDebug() << "item.hasPayload<Addressee>() " << item.hasPayload<KABC::Addressee>();
  if ( item.hasPayload<KABC::Addressee>() ) {
    KABC::Addressee addressee = item.payload<KABC::Addressee>();

    if ( addressee.uid().isEmpty() )
      addressee.setUid( KRandom::randomString( 10 ) );

    mAddressBook->insertAddressee( addressee );

    // TODO: proper error reporting
    if ( scheduleSaveAddressBook() ) {
      Item i( item );
      i.setRemoteId( addressee.uid() );
      i.setPayload<KABC::Addressee>( addressee );

      changeCommitted( i );
    } else {
      changeProcessed();
    }
  } else {
    // TODO: handle distribution lists once we have an Akonadi payload for them
    changeProcessed();
  }
}

void KABCResource::itemChanged( const Akonadi::Item &item, const QSet<QByteArray>& parts )
{
  kDebug() << "item id="  << item.id() << ", remoteId=" << item.remoteId()
           << "mimeType=" << item.mimeType();
  // we store the whole addressee any way
  Q_UNUSED( parts );

  kDebug() << "item.hasPayload<Addressee>() " << item.hasPayload<KABC::Addressee>();
  if ( item.hasPayload<KABC::Addressee>() ) {
    KABC::Addressee addressee = item.payload<KABC::Addressee>();
    Q_ASSERT( !addressee.uid().isEmpty() );

    mAddressBook->insertAddressee( addressee );

    // TODO: proper error reporting
    if ( scheduleSaveAddressBook() ) {
      changeCommitted( item );
    } else {
      changeProcessed();
    }
  } else {
    // TODO: handle distribution lists once we have an Akonadi payload for them
    changeProcessed();
  }
}

void KABCResource::itemRemoved( const Akonadi::Item &item )
{
  kDebug() << "item id="  << item.id() << ", remoteId=" << item.remoteId()
           << "mimeType=" << item.mimeType();
  KABC::Addressee addressee = mAddressBook->findByUid( item.remoteId() );
  if ( !addressee.isEmpty() ) {
    mAddressBook->removeAddressee( addressee );
    // TODO: proper error reporting
    if ( scheduleSaveAddressBook() ) {
      changeCommitted( item );
    } else {
      changeProcessed();
    }
  } else {
    // TODO: handle distribution lists once we have an Akonadi payload for them
    changeProcessed();
  }
}

void KABCResource::collectionChanged( const Akonadi::Collection &collection )
{
  Q_ASSERT( mBaseResource != 0);
  kDebug() << "collection.name=" << collection.name()
           << ", resource name=" << mBaseResource->resourceName();

  if ( collection.parent() == Collection::root().id() ) {
    if ( collection.name() != mBaseResource->resourceName() ) {
      mBaseResource->setResourceName( collection.name() );
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
    }
  }

  // do not react on addressbook changes until we have finished its initial loading
  mAddressBook->blockSignals( true );

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

  synchronizeCollectionTree();
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

  // TODO: currently addressBookChanged() handles this already
}

void KABCResource::subResourceRemoved( KABC::ResourceABC *resource,
        const QString &type, const QString &subResource )
{
  kDebug() << "subResource" << subResource;
  Q_ASSERT( resource == mFolderResource );
  Q_ASSERT( type.toLower() == QLatin1String( "contact" ) );
  Q_ASSERT( !subResource.isEmpty() );

  // TODO: currently addressBookChanged() handles this already
}

void KABCResource::reloadConfiguration()
{
  if ( mDelayedSaveTimer->isActive() ) {
    if ( !saveAddressBook() ) {
      kError() << "Saving of address book failed:" << mErrorHandler->mLastError;
    }
  }
  closeConfiguration();

  KSharedConfig::Ptr config = KGlobal::config();
  Q_ASSERT( !config.isNull() );

  KGlobal::config()->reparseConfiguration();

  KRES::Manager<KABC::Resource> *manager = mAddressBook->getResourceManager();
  manager->readConfig( config.data() );

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

AKONADI_RESOURCE_MAIN( KABCResource )

#include "kabcresource.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
