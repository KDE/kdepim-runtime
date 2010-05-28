/*  This file is part of the KDE project
    Copyright (C) 2010 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.net
    Author: Kevin Krammer, krake@kdab.com

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

#include "abstractlocalstore.h"

#include "collectioncreatejob.h"
#include "collectiondeletejob.h"
#include "collectionfetchjob.h"
#include "collectionmodifyjob.h"
#include "collectionmovejob.h"
#include "itemcreatejob.h"
#include "itemdeletejob.h"
#include "itemfetchjob.h"
#include "itemmodifyjob.h"
#include "itemmovejob.h"
#include "sessionimpls_p.h"
#include "storecompactjob.h"

#include <akonadi/collection.h>
#include <akonadi/entitydisplayattribute.h>

#include <KLocale>

#include <QFileInfo>

using namespace Akonadi;

class JobProcessingAdaptor : public FileStore::Job::Visitor
{
  public:
    explicit JobProcessingAdaptor( FileStore::AbstractJobSession *session )
      : mSession( session )
    {
    }

  public: // Job::Visitor interface implementation
    bool visit( FileStore::Job *job ) { Q_UNUSED( job ); return false ; }

    bool visit( FileStore::CollectionCreateJob *job ) { Q_UNUSED( job ); return false ; }

    bool visit( FileStore::CollectionDeleteJob *job ) { Q_UNUSED( job ); return false ; }

    bool visit( FileStore::CollectionFetchJob *job ) { Q_UNUSED( job ); return false ; }

    bool visit( FileStore::CollectionModifyJob *job ) { Q_UNUSED( job ); return false ; }

    bool visit( FileStore::CollectionMoveJob *job ) { Q_UNUSED( job ); return false ; }

    bool visit( FileStore::ItemCreateJob *job ) { Q_UNUSED( job ); return false ; }

    bool visit( FileStore::ItemDeleteJob *job ) { Q_UNUSED( job ); return false ; }

    bool visit( FileStore::ItemFetchJob *job ) { Q_UNUSED( job ); return false ; }

    bool visit( FileStore::ItemModifyJob *job ) { Q_UNUSED( job ); return false ; }

    bool visit( FileStore::ItemMoveJob *job ) { Q_UNUSED( job ); return false ; }

    bool visit( FileStore::StoreCompactJob *job ) { Q_UNUSED( job ); return false ; }

  protected:
    FileStore::AbstractJobSession *mSession;
};

class TopLevelCollectionFetcher : public JobProcessingAdaptor
{
  public:
    explicit TopLevelCollectionFetcher( FileStore::AbstractJobSession *session )
      : JobProcessingAdaptor( session )
    {
    }

    void setTopLevelCollection( const Collection &collection )
    {
      mTopLevelCollection = collection;
    }

  public:
    using JobProcessingAdaptor::visit;

    bool visit( FileStore::CollectionFetchJob *job )
    {
      if ( job->type() == FileStore::CollectionFetchJob::Base &&
          job->collection().remoteId() == mTopLevelCollection.remoteId() ) {
        mSession->notifyCollectionsReceived( job, Collection::List() << mTopLevelCollection );
        return true;
      }

      return false;
    }

  private:
    Collection mTopLevelCollection;
};

class CollectionsProcessedNotifier : public JobProcessingAdaptor
{
  public:
    explicit CollectionsProcessedNotifier( FileStore::AbstractJobSession *session )
      : JobProcessingAdaptor( session )
    {
    }

    void setCollections( const Collection::List &collections )
    {
      mCollections = collections;
    }

  public:
    using JobProcessingAdaptor::visit;

    bool visit( FileStore::CollectionCreateJob* job )
    {
      Q_ASSERT( !mCollections.isEmpty() );
      if ( mCollections.count() > 1 ) {
        kError() << "Processing collections for CollectionCreateJob "
                    "encountered more than one collection. Just processing the first one.";
      }

      mSession->notifyCollectionCreated( job, mCollections[ 0 ] );
      return true;
    }

    bool visit( FileStore::CollectionDeleteJob* job )
    {
      Q_ASSERT( !mCollections.isEmpty() );
      if ( mCollections.count() > 1 ) {
        kError() << "Processing collections for CollectionDeleteJob "
                    "encountered more than one collection. Just processing the first one.";
      }

      mSession->notifyCollectionDeleted( job, mCollections[ 0 ] );
      return true;
    }

    bool visit( FileStore::CollectionFetchJob* job )
    {
      mSession->notifyCollectionsReceived( job, mCollections );
      return true;
    }

    bool visit( FileStore::CollectionModifyJob* job )
    {
      Q_ASSERT( !mCollections.isEmpty() );
      if ( mCollections.count() > 1 ) {
        kError() << "Processing collections for CollectionModifyJob "
                    "encountered more than one collection. Just processing the first one.";
      }

      mSession->notifyCollectionModified( job, mCollections[ 0 ] );
      return true;
    }

    bool visit( FileStore::CollectionMoveJob* job )
    {
      Q_ASSERT( !mCollections.isEmpty() );
      if ( mCollections.count() > 1 ) {
        kError() << "Processing collections for CollectionMoveJob "
                    "encountered more than one collection. Just processing the first one.";
      }

      mSession->notifyCollectionMoved( job, mCollections[ 0 ] );
      return true;
    }

    bool visit( FileStore::StoreCompactJob* job )
    {
      mSession->notifyCollectionsChanged( job, mCollections );
      return true;
    }

  private:
    Collection::List mCollections;
};

class ItemsProcessedNotifier : public JobProcessingAdaptor
{
  public:
    explicit ItemsProcessedNotifier( FileStore::AbstractJobSession *session )
      : JobProcessingAdaptor( session )
    {
    }

    void setItems( const Item::List &items )
    {
      mItems = items;
    }

  public:
    using JobProcessingAdaptor::visit;

    bool visit( FileStore::ItemCreateJob* job )
    {
      Q_ASSERT( !mItems.isEmpty() );
      if ( mItems.count() > 1 ) {
        kError() << "Processing items for ItemCreateJob encountered more than one item. "
                    "Just processing the first one.";
      }

      mSession->notifyItemCreated( job, mItems[ 0 ] );
      return true;
    }

    bool visit( FileStore::ItemFetchJob* job )
    {
      mSession->notifyItemsReceived( job, mItems );
      return true;
    }

    bool visit( FileStore::ItemModifyJob* job )
    {
      Q_ASSERT( !mItems.isEmpty() );
      if ( mItems.count() > 1 ) {
        kError() << "Processing items for ItemModifyJob encountered more than one item. "
                    "Just processing the first one.";
      }

      mSession->notifyItemModified( job, mItems[ 0 ] );
      return true;
    }

    bool visit( FileStore::ItemMoveJob* job )
    {
      Q_ASSERT( !mItems.isEmpty() );
      if ( mItems.count() > 1 ) {
        kError() << "Processing items for ItemMoveJob encountered more than one item. "
                    "Just processing the first one.";
      }

      mSession->notifyItemMoved( job, mItems[ 0 ] );
      return true;
    }

    bool visit( FileStore::StoreCompactJob* job )
    {
      mSession->notifyItemsChanged( job, mItems );
      return true;
    }

  private:
    Item::List mItems;
};

class FileStore::AbstractLocalStore::Private
{
  AbstractLocalStore *const q;

  public:
    explicit Private( FileStore::AbstractLocalStore *parent )
      : q( parent ), mSession( new FileStore::FiFoQueueJobSession( q ) ), mCurrentJob( 0 ),
        mTopLevelCollectionFetcher( mSession ), mCollectionsProcessedNotifier( mSession ),
        mItemsProcessedNotifier( mSession )
    {
    }

  public:
    QFileInfo mPathFileInfo;
    Collection mTopLevelCollection;

    FileStore::AbstractJobSession *mSession;
    FileStore::Job *mCurrentJob;

    TopLevelCollectionFetcher mTopLevelCollectionFetcher;
    CollectionsProcessedNotifier mCollectionsProcessedNotifier;
    ItemsProcessedNotifier mItemsProcessedNotifier;

  public:
    void processJobs( const QList<FileStore::Job*> &jobs );
};

void FileStore::AbstractLocalStore::Private::processJobs( const QList<FileStore::Job*> &jobs )
{
  Q_FOREACH( FileStore::Job *job, jobs ) {
    mCurrentJob = job;

    if ( job->error() == 0 ) {
      if ( !job->accept( &mTopLevelCollectionFetcher ) ) {
        q->processJob( job );
      }
    }
    mSession->emitResult( job );
    mCurrentJob = 0;
  }
}

FileStore::AbstractLocalStore::AbstractLocalStore()
  : QObject(), d( new Private( this ) )
{
  connect( d->mSession, SIGNAL( jobsReady( QList<FileStore::Job*> ) ), this, SLOT( processJobs( QList<FileStore::Job*> ) ) );
}

FileStore::AbstractLocalStore::~AbstractLocalStore()
{
  delete d;
}

void FileStore::AbstractLocalStore::setPath( const QString &path )
{
  QFileInfo pathFileInfo( path );
  if ( pathFileInfo.fileName().isEmpty() ) {
    pathFileInfo = QFileInfo( pathFileInfo.path() );
  }
  pathFileInfo.makeAbsolute();

  if ( pathFileInfo.absoluteFilePath() == d->mPathFileInfo.absoluteFilePath() ) {
    return;
  }

  d->mPathFileInfo = pathFileInfo;

  Collection collection;
  collection.setRemoteId( d->mPathFileInfo.absoluteFilePath() );
  collection.setName( d->mPathFileInfo.fileName() );

  EntityDisplayAttribute *attribute = collection.attribute<EntityDisplayAttribute>();
  if ( attribute != 0 ) {
    attribute->setDisplayName( d->mPathFileInfo.fileName() );
  }

  setTopLevelCollection( collection );
}

QString FileStore::AbstractLocalStore::path() const
{
  return d->mPathFileInfo.absoluteFilePath();
}

Collection FileStore::AbstractLocalStore::topLevelCollection() const
{
  return d->mTopLevelCollection;
}

FileStore::CollectionCreateJob *FileStore::AbstractLocalStore::createCollection( const Collection &collection, const Collection &targetParent )
{
  FileStore::CollectionCreateJob *job = new FileStore::CollectionCreateJob( collection, targetParent, d->mSession );

  if ( d->mTopLevelCollection.remoteId().isEmpty() ) {
    const QString message = i18nc( "@info:status", "Configured storage location is empty" );
    kError() << message;
    kError() << collection << targetParent;
    d->mSession->setError( job, FileStore::Job::InvalidStoreState, message );
  } else if ( targetParent.remoteId().isEmpty() ) {
    const QString message = i18nc( "@info:status", "Given folder name is empty" );
    kError() << message;
    kError() << collection << targetParent;
    d->mSession->setError( job, FileStore::Job::InvalidJobContext, message );
  } else if ( ( targetParent.rights() & Collection::CanCreateCollection ) == 0 ) {
    const QString message = i18nc( "@info:status", "Access control prohibits folder creation in folder %1", targetParent.name() );
    kError() << message;
    kError() << collection << targetParent;
    d->mSession->setError( job, FileStore::Job::InvalidJobContext, message );
  }

  int errorCode = 0;
  QString errorText;
  checkCollectionCreate( job, errorCode, errorText );
  if ( errorCode != 0 ) {
    d->mSession->setError( job, errorCode, errorText );
  }

  return job;
}

FileStore::CollectionDeleteJob *FileStore::AbstractLocalStore::deleteCollection( const Collection &collection )
{
  FileStore::CollectionDeleteJob *job = new FileStore::CollectionDeleteJob( collection, d->mSession );

  if ( d->mTopLevelCollection.remoteId().isEmpty() ) {
    const QString message = i18nc( "@info:status", "Configured storage location is empty" );
    kError() << message;
    kError() << collection;
    d->mSession->setError( job, FileStore::Job::InvalidStoreState, message );
  } else if ( collection.remoteId().isEmpty() ||
              collection.parentCollection().remoteId().isEmpty() ) {
    const QString message = i18nc( "@info:status", "Given folder name is empty" );
    kError() << message;
    kError() << collection;
    d->mSession->setError( job, FileStore::Job::InvalidJobContext, message );
  } else if ( ( collection.parentCollection().rights() & Collection::CanDeleteCollection ) == 0 ) {
    const QString message = i18nc( "@info:status", "Access control prohibits folder deletion in folder %1", collection.parentCollection().name() );
    kError() << message;
    kError() << collection;
    d->mSession->setError( job, FileStore::Job::InvalidJobContext, message );
  }

  int errorCode = 0;
  QString errorText;
  checkCollectionDelete( job, errorCode, errorText );
  if ( errorCode != 0 ) {
    d->mSession->setError( job, errorCode, errorText );
  }

  return job;
}

FileStore::CollectionFetchJob *FileStore::AbstractLocalStore::fetchCollections( const Collection &collection, FileStore::CollectionFetchJob::Type type ) const
{
  FileStore::CollectionFetchJob *job = new FileStore::CollectionFetchJob( collection, type, d->mSession );

  if ( d->mTopLevelCollection.remoteId().isEmpty() ) {
    const QString message = i18nc( "@info:status", "Configured storage location is empty" );
    kError() << message;
    kError() << collection << "FetchType=" << type;
    d->mSession->setError( job, FileStore::Job::InvalidStoreState, message );
  } else if ( collection.remoteId().isEmpty() ) {
    const QString message = i18nc( "@info:status", "Given folder name is empty" );
    kError() << message;
    kError() << collection << "FetchType=" << type;
    d->mSession->setError( job, FileStore::Job::InvalidJobContext, message );
  }

  int errorCode = 0;
  QString errorText;
  checkCollectionFetch( job, errorCode, errorText );
  if ( errorCode != 0 ) {
    d->mSession->setError( job, errorCode, errorText );
  }

  return job;
}

FileStore::CollectionModifyJob *FileStore::AbstractLocalStore::modifyCollection( const Collection &collection )
{
  FileStore::CollectionModifyJob *job = new FileStore::CollectionModifyJob( collection, d->mSession );

  if ( d->mTopLevelCollection.remoteId().isEmpty() ) {
    const QString message = i18nc( "@info:status", "Configured storage location is empty" );
    kError() << message;
    kError() << collection;
    d->mSession->setError( job, FileStore::Job::InvalidStoreState, message );
  } else if ( collection.remoteId().isEmpty() ||
              collection.parentCollection().remoteId().isEmpty() ) {
    const QString message = i18nc( "@info:status", "Given folder name is empty" );
    kError() << message;
    kError() << collection;
    d->mSession->setError( job, FileStore::Job::InvalidJobContext, message );
  } else if ( ( collection.parentCollection().rights() & Collection::CanChangeCollection ) == 0 ) {
    const QString message = i18nc( "@info:status", "Access control prohibits folder modification in folder %1", collection.parentCollection().name() );
    kError() << message;
    kError() << collection;
    d->mSession->setError( job, FileStore::Job::InvalidJobContext, message );
  }

  int errorCode = 0;
  QString errorText;
  checkCollectionModify( job, errorCode, errorText );
  if ( errorCode != 0 ) {
    d->mSession->setError( job, errorCode, errorText );
  }

  return job;
}

FileStore::CollectionMoveJob *FileStore::AbstractLocalStore::moveCollection( const Collection &collection, const Collection &targetParent )
{
  FileStore::CollectionMoveJob *job = new FileStore::CollectionMoveJob( collection, targetParent, d->mSession );

  if ( d->mTopLevelCollection.remoteId().isEmpty() ) {
    const QString message = i18nc( "@info:status", "Configured storage location is empty" );
    kError() << message;
    kError() << collection << targetParent;
    d->mSession->setError( job, FileStore::Job::InvalidStoreState, message );
  } else if ( collection.remoteId().isEmpty() ||
              collection.parentCollection().remoteId().isEmpty() ||
              targetParent.remoteId().isEmpty() ) {
    const QString message = i18nc( "@info:status", "Given folder name is empty" );
    kError() << message;
    kError() << collection << targetParent;
    d->mSession->setError( job, FileStore::Job::InvalidJobContext, message );
  } else if ( ( targetParent.rights() & Collection::CanCreateCollection ) == 0 ) {
    const QString message = i18nc( "@info:status", "Access control prohibits folder creation in folder %1", targetParent.name() );
    kError() << message;
    kError() << collection << targetParent;
    d->mSession->setError( job, FileStore::Job::InvalidJobContext, message );
  } else if ( ( collection.parentCollection().rights() & Collection::CanDeleteCollection ) == 0 ) {
    const QString message = i18nc( "@info:status", "Access control prohibits folder deletion in folder %1", collection.parentCollection().name() );
    kError() << message;
    kError() << collection << targetParent;
    d->mSession->setError( job, FileStore::Job::InvalidJobContext, message );
  }

  int errorCode = 0;
  QString errorText;
  checkCollectionMove( job, errorCode, errorText );
  if ( errorCode != 0 ) {
    d->mSession->setError( job, errorCode, errorText );
  }

  return job;
}

FileStore::ItemFetchJob *FileStore::AbstractLocalStore::fetchItems( const Collection &collection ) const
{
  FileStore::ItemFetchJob *job = new FileStore::ItemFetchJob( collection, d->mSession );

  if ( d->mTopLevelCollection.remoteId().isEmpty() ) {
    const QString message = i18nc( "@info:status", "Configured storage location is empty" );
    kError() << message;
    kError() << collection;
    d->mSession->setError( job, FileStore::Job::InvalidStoreState, message );
  } else if ( collection.remoteId().isEmpty() ) {
    const QString message = i18nc( "@info:status", "Given folder name is empty" );
    kError() << message;
    kError() << collection;
    d->mSession->setError( job, FileStore::Job::InvalidJobContext, message );
  }

  int errorCode = 0;
  QString errorText;
  checkItemFetch( job, errorCode, errorText );
  if ( errorCode != 0 ) {
    d->mSession->setError( job, errorCode, errorText );
  }

  return job;
}

FileStore::ItemFetchJob *FileStore::AbstractLocalStore::fetchItem( const Item &item ) const
{
  FileStore::ItemFetchJob *job = new FileStore::ItemFetchJob( item, d->mSession );

  if ( d->mTopLevelCollection.remoteId().isEmpty() ) {
    const QString message = i18nc( "@info:status", "Configured storage location is empty" );
    kError() << message;
    kError() << "Item(remoteId=" << item.remoteId() << ", mimeType=" << item.mimeType()
             << ", parentCollection=" << item.parentCollection().remoteId() << ")";
    d->mSession->setError( job, FileStore::Job::InvalidStoreState, message );
  } else if ( item.remoteId().isEmpty() ) {
    const QString message = i18nc( "@info:status", "Given item identifier is empty" );
    kError() << message;
    kError() << "Item(remoteId=" << item.remoteId() << ", mimeType=" << item.mimeType()
             << ", parentCollection=" << item.parentCollection().remoteId() << ")";
    d->mSession->setError( job, FileStore::Job::InvalidJobContext, message );
  }

  int errorCode = 0;
  QString errorText;
  checkItemFetch( job, errorCode, errorText );
  if ( errorCode != 0 ) {
    d->mSession->setError( job, errorCode, errorText );
  }

  return job;
}

FileStore::ItemCreateJob *FileStore::AbstractLocalStore::createItem( const Item &item, const Collection &collection )
{
  FileStore::ItemCreateJob *job = new FileStore::ItemCreateJob( item, collection, d->mSession );

  if ( d->mTopLevelCollection.remoteId().isEmpty() ) {
    const QString message = i18nc( "@info:status", "Configured storage location is empty" );
    kError() << message;
    kError() << collection
             << "Item(remoteId=" << item.remoteId() << ", mimeType=" << item.mimeType()
             << ")";
    d->mSession->setError( job, FileStore::Job::InvalidStoreState, message );
  } else if ( collection.remoteId().isEmpty() ) {
    const QString message = i18nc( "@info:status", "Given folder name is empty" );
    kError() << message;
    kError() << collection
             << "Item(remoteId=" << item.remoteId() << ", mimeType=" << item.mimeType()
             << ")";
    d->mSession->setError( job, FileStore::Job::InvalidJobContext, message );
  } else if ( ( collection.rights() & Collection::CanCreateItem ) == 0 ) {
    const QString message = i18nc( "@info:status", "Access control prohibits item creation in folder %1", collection.name() );
    kError() << message;
    kError() << collection
             << "Item(remoteId=" << item.remoteId() << ", mimeType=" << item.mimeType()
             << ")";
    d->mSession->setError( job, FileStore::Job::InvalidJobContext, message );
  }

  int errorCode = 0;
  QString errorText;
  checkItemCreate( job, errorCode, errorText );
  if ( errorCode != 0 ) {
    d->mSession->setError( job, errorCode, errorText );
  }

  return job;
}

FileStore::ItemModifyJob *FileStore::AbstractLocalStore::modifyItem( const Item &item )
{
  FileStore::ItemModifyJob *job = new FileStore::ItemModifyJob( item, d->mSession );

  if ( d->mTopLevelCollection.remoteId().isEmpty() ) {
    const QString message = i18nc( "@info:status", "Configured storage location is empty" );
    kError() << message;
    kError() << "Item(remoteId=" << item.remoteId() << ", mimeType=" << item.mimeType()
             << ", parentCollection=" << item.parentCollection().remoteId() << ")";
    d->mSession->setError( job, FileStore::Job::InvalidStoreState, message );
  } else if ( item.remoteId().isEmpty() ) {
    const QString message = i18nc( "@info:status", "Given item identifier is empty" );
    kError() << message;
    kError() << "Item(remoteId=" << item.remoteId() << ", mimeType=" << item.mimeType()
             << ", parentCollection=" << item.parentCollection().remoteId() << ")";
    d->mSession->setError( job, FileStore::Job::InvalidJobContext, message );
  } else if ( ( item.parentCollection().rights() & Collection::CanChangeItem ) == 0 ) {
    const QString message = i18nc( "@info:status", "Access control prohibits item modification in folder %1", item.parentCollection().name() );
    kError() << message;
    kError() << "Item(remoteId=" << item.remoteId() << ", mimeType=" << item.mimeType()
             << ", parentCollection=" << item.parentCollection().remoteId() << ")";
    d->mSession->setError( job, FileStore::Job::InvalidJobContext, message );
  }

  int errorCode = 0;
  QString errorText;
  checkItemModify( job, errorCode, errorText );
  if ( errorCode != 0 ) {
    d->mSession->setError( job, errorCode, errorText );
  }

  return job;
}

FileStore::ItemDeleteJob *FileStore::AbstractLocalStore::deleteItem( const Item &item )
{
  FileStore::ItemDeleteJob *job = new FileStore::ItemDeleteJob( item, d->mSession );

  if ( d->mTopLevelCollection.remoteId().isEmpty() ) {
    const QString message = i18nc( "@info:status", "Configured storage location is empty" );
    kError() << message;
    kError() << "Item(remoteId=" << item.remoteId() << ", mimeType=" << item.mimeType()
             << ", parentCollection=" << item.parentCollection().remoteId() << ")";
    d->mSession->setError( job, FileStore::Job::InvalidStoreState, message );
  } else if ( item.remoteId().isEmpty() ) {
    const QString message = i18nc( "@info:status", "Given item identifier is empty" );
    kError() << message;
    kError() << "Item(remoteId=" << item.remoteId() << ", mimeType=" << item.mimeType()
             << ", parentCollection=" << item.parentCollection().remoteId() << ")";
    d->mSession->setError( job, FileStore::Job::InvalidJobContext, message );
  } else if ( ( item.parentCollection().rights() & Collection::CanDeleteItem ) == 0 ) {
    const QString message = i18nc( "@info:status", "Access control prohibits item deletion in folder %1", item.parentCollection().name() );
    kError() << message;
    kError() << "Item(remoteId=" << item.remoteId() << ", mimeType=" << item.mimeType()
             << ", parentCollection=" << item.parentCollection().remoteId() << ")";
    d->mSession->setError( job, FileStore::Job::InvalidJobContext, message );
  }

  int errorCode = 0;
  QString errorText;
  checkItemDelete( job, errorCode, errorText );
  if ( errorCode != 0 ) {
    d->mSession->setError( job, errorCode, errorText );
  }

  return job;
}

FileStore::ItemMoveJob *FileStore::AbstractLocalStore::moveItem( const Item &item, const Collection &targetParent )
{
  FileStore::ItemMoveJob *job = new FileStore::ItemMoveJob( item, targetParent, d->mSession );

  if ( d->mTopLevelCollection.remoteId().isEmpty() ) {
    const QString message = i18nc( "@info:status", "Configured storage location is empty" );
    kError() << message;
    kError() << "Item(remoteId=" << item.remoteId() << ", mimeType=" << item.mimeType()
             << ", parentCollection=" << item.parentCollection().remoteId() << ")"
             << targetParent;
    d->mSession->setError( job, FileStore::Job::InvalidStoreState, message );
  } else if ( item.parentCollection().remoteId().isEmpty() ||
              targetParent.remoteId().isEmpty() ) {
    const QString message = i18nc( "@info:status", "Given folder name is empty" );
    kError() << message;
    kError() << "Item(remoteId=" << item.remoteId() << ", mimeType=" << item.mimeType()
             << ", parentCollection=" << item.parentCollection().remoteId() << ")"
             << targetParent;
    d->mSession->setError( job, FileStore::Job::InvalidJobContext, message );
  } else if ( ( targetParent.rights() & Collection::CanCreateItem ) == 0 ) {
    const QString message = i18nc( "@info:status", "Access control prohibits item creation in folder %1", targetParent.name() );
    kError() << message;
    kError() << "Item(remoteId=" << item.remoteId() << ", mimeType=" << item.mimeType()
             << ", parentCollection=" << item.parentCollection().remoteId() << ")"
             << targetParent;
    d->mSession->setError( job, FileStore::Job::InvalidJobContext, message );
  } else if ( ( item.parentCollection().rights() & Collection::CanDeleteItem ) == 0 ) {
    const QString message = i18nc( "@info:status", "Access control prohibits item deletion in folder %1", item.parentCollection().name() );
    kError() << message;
    kError() << "Item(remoteId=" << item.remoteId() << ", mimeType=" << item.mimeType()
             << ", parentCollection=" << item.parentCollection().remoteId() << ")"
             << targetParent;
    d->mSession->setError( job, FileStore::Job::InvalidJobContext, message );
  } else if ( item.remoteId().isEmpty() ) {
    const QString message = i18nc( "@info:status", "Given item identifier is empty" );
    kError() << message;
    kError() << "Item(remoteId=" << item.remoteId() << ", mimeType=" << item.mimeType()
             << ", parentCollection=" << item.parentCollection().remoteId() << ")"
             << targetParent;
    d->mSession->setError( job, FileStore::Job::InvalidJobContext, message );
  }

  int errorCode = 0;
  QString errorText;
  checkItemMove( job, errorCode, errorText );
  if ( errorCode != 0 ) {
    d->mSession->setError( job, errorCode, errorText );
  }

  return job;
}

FileStore::StoreCompactJob *FileStore::AbstractLocalStore::compactStore()
{
  FileStore::StoreCompactJob *job = new FileStore::StoreCompactJob( d->mSession );

  if ( d->mTopLevelCollection.remoteId().isEmpty() ) {
    const QString message = i18nc( "@info:status", "Configured storage location is empty" );
    kError() << message;
    d->mSession->setError( job, FileStore::Job::InvalidStoreState, message );
  }

  int errorCode = 0;
  QString errorText;
  checkStoreCompact( job, errorCode, errorText );
  if ( errorCode != 0 ) {
    d->mSession->setError( job, errorCode, errorText );
  }

  return job;
}

FileStore::Job *FileStore::AbstractLocalStore::currentJob() const
{
  return d->mCurrentJob;
}

void FileStore::AbstractLocalStore::notifyError( int errorCode, const QString &errorText ) const
{
  Q_ASSERT( d->mCurrentJob != 0);

  d->mSession->setError( d->mCurrentJob, errorCode, errorText );
}

void FileStore::AbstractLocalStore::notifyCollectionsProcessed( const Collection::List &collections ) const
{
  Q_ASSERT( d->mCurrentJob != 0);

  d->mCollectionsProcessedNotifier.setCollections( collections );
  d->mCurrentJob->accept( &( d->mCollectionsProcessedNotifier ) );
}

void FileStore::AbstractLocalStore::notifyItemsProcessed( const Item::List &items ) const
{
  Q_ASSERT( d->mCurrentJob != 0);

  d->mItemsProcessedNotifier.setItems( items );
  d->mCurrentJob->accept( &( d->mItemsProcessedNotifier ) );
}

void FileStore::AbstractLocalStore::setTopLevelCollection( const Collection &collection )
{
  d->mTopLevelCollection = collection;
  d->mTopLevelCollectionFetcher.setTopLevelCollection( collection );
}

void FileStore::AbstractLocalStore::checkCollectionCreate( FileStore::CollectionCreateJob *job, int &errorCode, QString &errorText ) const
{
  Q_UNUSED( job );
  Q_UNUSED( errorCode );
  Q_UNUSED( errorText );
}

void FileStore::AbstractLocalStore::checkCollectionDelete( FileStore::CollectionDeleteJob *job, int &errorCode, QString &errorText ) const
{
  Q_UNUSED( job );
  Q_UNUSED( errorCode );
  Q_UNUSED( errorText );
}

void FileStore::AbstractLocalStore::checkCollectionFetch( FileStore::CollectionFetchJob *job, int &errorCode, QString &errorText ) const
{
  Q_UNUSED( job );
  Q_UNUSED( errorCode );
  Q_UNUSED( errorText );
}

void FileStore::AbstractLocalStore::checkCollectionModify( FileStore::CollectionModifyJob *job, int &errorCode, QString &errorText ) const
{
  Q_UNUSED( job );
  Q_UNUSED( errorCode );
  Q_UNUSED( errorText );
}

void FileStore::AbstractLocalStore::checkCollectionMove( FileStore::CollectionMoveJob *job, int &errorCode, QString &errorText ) const
{
  Q_UNUSED( job );
  Q_UNUSED( errorCode );
  Q_UNUSED( errorText );
}

void FileStore::AbstractLocalStore::checkItemCreate( FileStore::ItemCreateJob *job, int &errorCode, QString &errorText ) const
{
  Q_UNUSED( job );
  Q_UNUSED( errorCode );
  Q_UNUSED( errorText );
}

void FileStore::AbstractLocalStore::checkItemDelete( FileStore::ItemDeleteJob *job, int &errorCode, QString &errorText ) const
{
  Q_UNUSED( job );
  Q_UNUSED( errorCode );
  Q_UNUSED( errorText );
}

void FileStore::AbstractLocalStore::checkItemFetch( FileStore::ItemFetchJob *job, int &errorCode, QString &errorText ) const
{
  Q_UNUSED( job );
  Q_UNUSED( errorCode );
  Q_UNUSED( errorText );
}

void FileStore::AbstractLocalStore::checkItemModify( FileStore::ItemModifyJob *job, int &errorCode, QString &errorText ) const
{
  Q_UNUSED( job );
  Q_UNUSED( errorCode );
  Q_UNUSED( errorText );
}

void FileStore::AbstractLocalStore::checkItemMove( FileStore::ItemMoveJob *job, int &errorCode, QString &errorText ) const
{
  Q_UNUSED( job );
  Q_UNUSED( errorCode );
  Q_UNUSED( errorText );
}

void FileStore::AbstractLocalStore::checkStoreCompact( FileStore::StoreCompactJob *job, int &errorCode, QString &errorText ) const
{
  Q_UNUSED( job );
  Q_UNUSED( errorCode );
  Q_UNUSED( errorText );
}

#include "abstractlocalstore.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
