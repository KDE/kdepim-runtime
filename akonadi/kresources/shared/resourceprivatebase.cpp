/*
    This file is part of kdepim.
    Copyright (c) 2009 Kevin Krammer <kevin.krammer@gmx.at>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "resourceprivatebase.h"

#include "concurrentjobs.h"
#include "idarbiterbase.h"
#include "itemsavecontext.h"
#include "storecollectiondialog.h"
#include "subresourcebase.h"

#include <akonadi/control.h>
#include <akonadi/mimetypechecker.h>

#include <KDebug>
#include <KLocale>

ResourcePrivateBase::ResourcePrivateBase( IdArbiterBase *idArbiter, QObject *parent )
  : QObject( parent ),
    mIdArbiter( idArbiter ),
    mStoreCollectionDialog( 0 ),
    mState( Closed ),
    mLoadingInProgress( false )
{
}

ResourcePrivateBase::ResourcePrivateBase( const KConfigGroup &config, IdArbiterBase *idArbiter, QObject *parent )
  : QObject( parent ),
    mConfig( config ),
    mIdArbiter( idArbiter ),
    mStoreCollectionDialog( 0 ),
    mState( Closed ),
    mLoadingInProgress( false )
{
  const QLatin1String resourceKey( "DefaultAkonadiResourceIdentifier" );
  mDefaultResourceIdentifier = config.readEntry( resourceKey, QString() );

  const QLatin1String urlKey( "CollectionUrl" );
  KUrl url = config.readEntry( urlKey, KUrl() );
  if ( url.isValid() ) {
    mDefaultStoreCollection = Akonadi::Collection::fromUrl( url );
  }

  const KConfigGroup storeConfig = config.group( QLatin1String( "StoreConfig" ) );
  if ( storeConfig.isValid() ) {
    const QStringList groups = storeConfig.groupList();
    foreach ( const QString &group, groups ) {
      KConfigGroup mimeConfig = storeConfig.group( group );

      url = mimeConfig.readEntry( urlKey );
      kDebug( 5650 ) << "read MIME config pair: mimeType=" << group
                    << ", url=" << url;
      if ( url.isValid() ) {
        mStoreCollectionsByMimeType[ group ] = Akonadi::Collection::fromUrl( url );
      }
    }
  }
}

ResourcePrivateBase::~ResourcePrivateBase()
{
  delete mIdArbiter;
  delete mStoreCollectionDialog;
}

bool ResourcePrivateBase::doOpen()
{
  kDebug( 5650 );
  if ( mState == Opened ) {
    kWarning( 5650 ) << "Trying to open already opened resource";
    return true;
  }

  if ( !startAkonadi() ) {
    kError( 5650 ) << "Failed to start Akonadi";
    mState = Failed;
    return false;
  }

  if ( !openResource() ) {
    kError( 5650 ) << "Failed to do type specific open";
    mState = Failed;
    return false;
  }

  mState = Opened;
  return true;
}

bool ResourcePrivateBase::doClose()
{
  if ( mConfig.isValid() ) {
    writeConfig( mConfig );
  }

  bool result = closeResource();

  mState = Closed;

  return result;
}

bool ResourcePrivateBase::doLoad()
{
  kDebug( 5650 ) << "isLoading=" << isLoading();
  mLoadingInProgress = true;

  bool result = loadResource();

  Q_ASSERT( !mLoadingInProgress ); // subclass must have called loadingResult

  return result;
}

bool ResourcePrivateBase::doAsyncLoad()
{
  kDebug( 5650 ) << "isLoading=" << isLoading();
  mLoadingInProgress = true;

  return asyncLoadResource();
}

bool ResourcePrivateBase::doSave()
{
  kDebug( 5650 ) << mChanges.count() << "changes";

  if ( mState == Closed ) {
    const QString message = i18nc( "@info:status", "Cannot save to closed resource" );
    savingResult( false, message );
    return false;
  }

  if ( mState == Failed ) {
    const QString message = i18nc( "@info:status", "Cannot save while not connected to Akonadi" );
    savingResult( false, message );
    return false;
  }

  if ( mChanges.isEmpty() ) {
    return true;
  }

  ItemSaveContext saveContext;
  if ( !prepareItemSaveContext( saveContext ) ) {
    const QString message = i18nc( "@info:status", "Processing change set failed" );
    savingResult( false, message );
    return false;
  }

  ConcurrentItemSaveJob itemSaveJob( saveContext );
  if ( !itemSaveJob.exec() ) {
    savingResult( false, itemSaveJob->errorString() );
    return false;
  }

  return true;
}

bool ResourcePrivateBase::doAsyncSave()
{
  kDebug( 5650 ) << mChanges.count() << "changes";

  if ( mState == Closed ) {
    const QString message = i18nc( "@info:status", "Cannot save to closed resource" );
    savingResult( false, message );
    return false;
  }

  if ( mState == Failed ) {
    const QString message = i18nc( "@info:status", "Cannot save while not connected to Akonadi" );
    savingResult( false, message );
    return false;
  }

  if ( mChanges.isEmpty() ) {
    return true;
  }

  ItemSaveContext saveContext;
  if ( !prepareItemSaveContext( saveContext ) ) {
    const QString message = i18nc( "@info:status", "Processing change set failed" );
    savingResult( false, message );
    return false;
  }

  ItemSaveJob *itemSaveJob = new ItemSaveJob( saveContext );
  connect( itemSaveJob, SIGNAL( result( KJob* ) ),
           SLOT( savingResult( KJob* ) ) );

  return true;
}

void ResourcePrivateBase::writeConfig( KConfigGroup &config ) const
{
  const QLatin1String urlKey( "CollectionUrl" );
  const QLatin1String resourceKey( "DefaultAkonadiResourceIdentifier" );

  Akonadi::Collection defaultStoreCollection = mDefaultStoreCollection;
  QString defaultResourceIdentifier = mDefaultResourceIdentifier;

  // if we have new store config, then delete the old keys
  if ( !mStoreCollectionsByMimeType.isEmpty() ) {
    defaultStoreCollection = Akonadi::Collection();
    defaultResourceIdentifier = QString();
  }

  // if we have a valid default store collection, then delete the resource identifier key
  if ( defaultStoreCollection.isValid() ) {
    defaultResourceIdentifier = QString();
    config.writeEntry( urlKey, defaultStoreCollection.url() );
  } else {
    config.deleteEntry( urlKey );
  }

  if ( !defaultResourceIdentifier.isEmpty() ) {
    config.writeEntry( resourceKey, defaultResourceIdentifier );
  } else {
    config.deleteEntry( resourceKey );
  }

  KConfigGroup storeConfig = config.group( QLatin1String( "StoreConfig" ) );
  QSet<QString> mimeConfigs = QSet<QString>::fromList( storeConfig.groupList() );

  CollectionsByMimeType::const_iterator it    = mStoreCollectionsByMimeType.constBegin();
  CollectionsByMimeType::const_iterator endIt = mStoreCollectionsByMimeType.constEnd();
  for ( ; it != endIt; ++it ) {
    KConfigGroup mimeConfig = storeConfig.group( it.key() );

    mimeConfig.writeEntry( urlKey, it.value().url() );
    mimeConfigs.remove( it.key() );

    kDebug( 5650 ) << "wrote MIME config pair: mimeType=" << it.key()
                   << ", url=" << it.value().url();
  }

  foreach ( const QString &group, mimeConfigs ) {
    storeConfig.deleteGroup( group );
  }

  writeResourceConfig( config );
}

void ResourcePrivateBase::clear()
{
  mIdArbiter->clear();
  mChanges.clear();
  clearResource();
}

void ResourcePrivateBase::setStoreCollectionsByMimeType( const StoreConfigIface::CollectionsByMimeType &collections )
{
  mStoreCollectionsByMimeType = collections;

  mDefaultStoreCollection = Akonadi::Collection();
}

StoreConfigIface::CollectionsByMimeType ResourcePrivateBase::storeCollectionsByMimeType() const
{
  if ( mStoreCollectionsByMimeType.isEmpty() && mDefaultStoreCollection.isValid() ) {
    return storeCollectionsFromOldDefault();
  }

  return mStoreCollectionsByMimeType;
}

bool ResourcePrivateBase::addLocalItem( const QString &uid, const QString &mimeType )
{
  kDebug( 5650 ) << "uid=" << uid << ", mimeType=" << mimeType;

  // check if there is a sub resource with a mapped item for the identifier
  // if there is, we have a change, otherwise its an add
  const SubResourceBase *resource = findSubResourceForMappedItem( uid );
  if ( resource == 0 ) {
    mChanges[ uid ] = Added;

    if ( mStoreCollectionDialog == 0 ) {
      mStoreCollectionDialog = new StoreCollectionDialog();
      mStoreCollectionDialog->setSubResourceModel( subResourceModel() );
    }

    resource = storeSubResourceForMimeType( mimeType );
    if ( resource == 0 ) {
      // check possible store sub resources. if there is only one, use it
      // otherwise we need to ask the user
      QList<const SubResourceBase*> possibleStores = writableSubResourcesForMimeType( mimeType );
      if ( possibleStores.count() == 1 ) {
        kDebug( 5650 ) << "Only one possible sub resource for MIME type="
                       << mimeType;
        resource = possibleStores.first();
      } else {
        resource = storeSubResourceFromUser( uid, mimeType );
        if ( resource == 0 ) {
          mChanges.remove( uid );
          return false;
        }
      }
    }
  } else {
    mChanges[ uid ] = Changed;
  }

  Q_ASSERT( resource != 0 );
  mUidToResourceMap[ uid ] = resource->subResourceIdentifier();

  return true;
}

void ResourcePrivateBase::changeLocalItem( const QString &uid )
{
  const QString subResource = mUidToResourceMap.value( uid );
  kDebug( 5650 ) << "uid=" << uid << ", subResource=" << subResource;

  Q_ASSERT( !subResource.isEmpty() );

  const SubResourceBase *resource = subResourceBase( subResource );
  Q_ASSERT( resource != 0 );

  if ( resource->hasMappedItem( uid ) ) {
    mChanges[ uid ] = Changed;
  } else {
    mChanges[ uid ] = Added;
  }
}

void ResourcePrivateBase::removeLocalItem( const QString &uid )
{
  const QString subResource = mUidToResourceMap.value( uid );
  kDebug( 5650 ) << "uid=" << uid << ", subResource=" << subResource;

  Q_ASSERT( !subResource.isEmpty() );

  const SubResourceBase *resource = subResourceBase( subResource );
  Q_ASSERT( resource != 0 );

  if ( resource->hasMappedItem( uid ) ) {
    mChanges[ uid ] = Removed;
  } else {
    mChanges.remove( uid );
  }
}

ResourcePrivateBase::State ResourcePrivateBase::state() const
{
  return mState;
}

bool ResourcePrivateBase::isLoading() const
{
  return mLoadingInProgress;
}

bool ResourcePrivateBase::startAkonadi()
{
  return Akonadi::Control::start();
}

Akonadi::Collection ResourcePrivateBase::storeCollectionForMimeType( const QString &mimeType ) const
{
  kDebug( 5650 ) << "mimeType=" << mimeType;

  if ( mStoreCollectionsByMimeType.isEmpty() && mDefaultStoreCollection.isValid() ) {
    if ( Akonadi::MimeTypeChecker::isWantedCollection( mDefaultStoreCollection, mimeType ) ) {
      kDebug( 5650 ) << "Taking DefaultStoreCollection: id=" << mDefaultStoreCollection.id()
                    << ", remoteId=" << mDefaultStoreCollection.remoteId();
      return mDefaultStoreCollection;
    }
  }

  Akonadi::Collection collection = mStoreCollectionsByMimeType.value( mimeType );
  if ( collection.isValid() ) {
    kDebug( 5650 ) << "Found storage collection in map: id=" << collection.id()
                   << ", remoteId=" << collection.remoteId();
    return collection;
  }

  return Akonadi::Collection();
}

bool ResourcePrivateBase::prepareItemSaveContext( ItemSaveContext &saveContext )
{
  ChangeByKResId::const_iterator it    = mChanges.constBegin();
  ChangeByKResId::const_iterator endIt = mChanges.constEnd();
  for ( ; it != endIt; ++it ) {
    if ( !prepareItemSaveContext( it, saveContext ) ) {
      return false;
    }
  }

  return true;
}


bool ResourcePrivateBase::prepareItemSaveContext( const ChangeByKResId::const_iterator &it, ItemSaveContext &saveContext )
{
  const QString kresId = it.key();
  const SubResourceBase *resource = subResourceBase( mUidToResourceMap.value( kresId ) );
  Q_ASSERT( resource != 0 );

  switch ( it.value() ) {
    case Added: {
      ItemAddContext addContext;
      addContext.collection = resource->collection();
      addContext.item = createItem( kresId );

      saveContext.addedItems << addContext;
      break;
    }

    case Changed: {
      Akonadi::Item item = updateItem( resource->mappedItem( kresId ), kresId, mIdArbiter->mapToOriginalId( kresId ) );

      saveContext.changedItems << item;
      break;
    }

    case Removed:
      saveContext.removedItems << resource->mappedItem( kresId );
      break;

    case NoChange:
       Q_ASSERT( "Invalid ChangeType in change map" );
       break;
  }

  return true;
}


void ResourcePrivateBase::subResourceAdded( SubResourceBase *subResource )
{
  Q_ASSERT( mIdArbiter != 0 );

  subResource->setIdArbiter( mIdArbiter );
  subResource->readConfig( mConfig );

  // if we don't have a valid default store collection yet, check if our
  // config indicates a default resource and whether the newly added sub resource
  // maps to one of its collections
  if ( !mDefaultStoreCollection.isValid() ) {
    if ( !mDefaultResourceIdentifier.isEmpty() ) {
      if ( subResource->collection().resource() == mDefaultResourceIdentifier ) {
        mDefaultStoreCollection = subResource->collection();
        mDefaultResourceIdentifier = QString();
      }
    }
  } else if ( mDefaultStoreCollection == subResource->collection() ) {
    // update locally cached instance
    mDefaultStoreCollection = subResource->collection();
  }

  // check if any of the MIME type specific collections can be updated
  CollectionsByMimeType::iterator it    = mStoreCollectionsByMimeType.begin();
  CollectionsByMimeType::iterator endIt = mStoreCollectionsByMimeType.end();
  for (; it != endIt; ++it ) {
    if ( it.value() == subResource->collection() ) {
      // update locally cached instance
      it.value() = subResource->collection();
    }
  }
}

void ResourcePrivateBase::subResourceRemoved( SubResourceBase *subResource )
{
  Q_UNUSED( subResource );
}

void ResourcePrivateBase::loadingResult( bool ok, const QString &errorString )
{
  Q_UNUSED( ok );
  Q_UNUSED( errorString );

  Q_ASSERT( mLoadingInProgress );

  mLoadingInProgress = false;
}

void ResourcePrivateBase::savingResult( bool ok, const QString &errorString )
{
  Q_UNUSED( errorString );
  if ( ok ) {
    mChanges.clear();
  }
}

#include "resourceprivatebase.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
