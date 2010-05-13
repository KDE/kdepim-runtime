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
using namespace Akonadi::FileStore;

class JobProcessingAdaptor : public Job::Visitor
{
  public:
    explicit JobProcessingAdaptor( AbstractJobSession *session )
      : mSession( session )
    {
    }

  public: // Job::Visitor interface implementation
    bool visit( Job *job ) { Q_UNUSED( job ); return false ; }

    bool visit( CollectionCreateJob *job ) { Q_UNUSED( job ); return false ; }

    bool visit( CollectionDeleteJob *job ) { Q_UNUSED( job ); return false ; }

    bool visit( CollectionFetchJob *job ) { Q_UNUSED( job ); return false ; }

    bool visit( CollectionModifyJob *job ) { Q_UNUSED( job ); return false ; }

    bool visit( CollectionMoveJob *job ) { Q_UNUSED( job ); return false ; }

    bool visit( ItemCreateJob *job ) { Q_UNUSED( job ); return false ; }

    bool visit( ItemDeleteJob *job ) { Q_UNUSED( job ); return false ; }

    bool visit( ItemFetchJob *job ) { Q_UNUSED( job ); return false ; }

    bool visit( ItemModifyJob *job ) { Q_UNUSED( job ); return false ; }

    bool visit( ItemMoveJob *job ) { Q_UNUSED( job ); return false ; }

    bool visit( StoreCompactJob *job ) { Q_UNUSED( job ); return false ; }

  protected:
    AbstractJobSession *mSession;
};

class TopLevelCollectionFetcher : public JobProcessingAdaptor
{
  public:
    explicit TopLevelCollectionFetcher( AbstractJobSession *session )
      : JobProcessingAdaptor( session )
    {
    }

    void setTopLevelCollection( const Collection &collection )
    {
      mTopLevelCollection = collection;
    }

  public:
    using JobProcessingAdaptor::visit;

    bool visit( CollectionFetchJob *job )
    {
      if ( job->type() == CollectionFetchJob::Base &&
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
    explicit CollectionsProcessedNotifier( AbstractJobSession *session )
      : JobProcessingAdaptor( session )
    {
    }

    void setCollections( const Collection::List &collections )
    {
      mCollections = collections;
    }

  public:
    using JobProcessingAdaptor::visit;

    bool visit( CollectionFetchJob* job )
    {
      mSession->notifyCollectionsReceived( job, mCollections );
      return true;
    }

  private:
    Collection::List mCollections;
};

class ItemsProcessedNotifier : public JobProcessingAdaptor
{
  public:
    explicit ItemsProcessedNotifier( AbstractJobSession *session )
      : JobProcessingAdaptor( session )
    {
    }

    void setItems( const Item::List &items )
    {
      mItems = items;
    }

  public:
    using JobProcessingAdaptor::visit;

    bool visit( ItemCreateJob* job )
    {
      Q_ASSERT( !mItems.isEmpty() );
      if ( mItems.count() > 1 ) {
        kError() << "Processing items for ItemCreateJob encountered more than one item. "
                    "Just processing the first one.";
      }

      mSession->notifyItemCreated( job, mItems[ 0 ] );
      return true;
    }

    bool visit( ItemFetchJob* job )
    {
      mSession->notifyItemsReceived( job, mItems );
      return true;
    }

    bool visit( ItemModifyJob* job )
    {
      Q_ASSERT( !mItems.isEmpty() );
      if ( mItems.count() > 1 ) {
        kError() << "Processing items for ItemModifyJob encountered more than one item. "
                    "Just processing the first one.";
      }

      mSession->notifyItemModified( job, mItems[ 0 ] );
      return true;
    }

  private:
    Item::List mItems;
};

class AbstractLocalStore::Private
{
  AbstractLocalStore *const q;

  public:
    explicit Private( AbstractLocalStore *parent )
      : q( parent ), mSession( new FiFoQueueJobSession( q ) ), mCurrentJob( 0 ),
        mTopLevelCollectionFetcher( mSession ), mCollectionsProcessedNotifier( mSession ),
        mItemsProcessedNotifier( mSession )
    {
    }

  public:
    QFileInfo mPathFileInfo;
    Collection mTopLevelCollection;

    AbstractJobSession *mSession;
    Job *mCurrentJob;

    TopLevelCollectionFetcher mTopLevelCollectionFetcher;
    CollectionsProcessedNotifier mCollectionsProcessedNotifier;
    ItemsProcessedNotifier mItemsProcessedNotifier;

  public:
    void processJobs( const QList<Job*> &jobs );
};

void AbstractLocalStore::Private::processJobs( const QList<Job*> &jobs )
{
  Q_FOREACH( Job *job, jobs ) {
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

AbstractLocalStore::AbstractLocalStore()
  : QObject(), d( new Private( this ) )
{
  connect( d->mSession, SIGNAL( jobsReady( QList<Job*> ) ), this, SLOT( processJobs( QList<Job*> ) ) );
}

AbstractLocalStore::~AbstractLocalStore()
{
  delete d;
}

void AbstractLocalStore::setPath( const QString &path )
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

QString AbstractLocalStore::path() const
{
  return d->mPathFileInfo.absoluteFilePath();
}

Collection AbstractLocalStore::topLevelCollection() const
{
  return d->mTopLevelCollection;
}

CollectionCreateJob *AbstractLocalStore::createCollection( const Collection &collection, const Collection &targetParent )
{
  CollectionCreateJob *job = new CollectionCreateJob( collection, targetParent, d->mSession );

  if ( d->mTopLevelCollection.remoteId().isEmpty() ) {
    const QString message = i18nc( "@info:status", "Configured storage location is empty" );
    kError() << message;
    kError() << collection << targetParent;
    d->mSession->setError( job, Job::InvalidStoreState, message );
  } else if ( targetParent.remoteId().isEmpty() ) {
    const QString message = i18nc( "@info:status", "Given folder name is empty" );
    kError() << message;
    kError() << collection << targetParent;
    d->mSession->setError( job, Job::InvalidJobContext, message );
  } else if ( ( targetParent.rights() & Collection::CanCreateCollection ) == 0 ) {
    const QString message = i18nc( "@info:status", "Access control prohibits folder creation in folder %1", targetParent.name() );
    kError() << message;
    kError() << collection << targetParent;
    d->mSession->setError( job, Job::InvalidJobContext, message );
  }

  int errorCode = 0;
  QString errorText;
  checkCollectionCreate( job, errorCode, errorText );
  if ( errorCode != 0 ) {
    d->mSession->setError( job, errorCode, errorText );
  }

  return job;
}

CollectionDeleteJob *AbstractLocalStore::deleteCollection( const Collection &collection )
{
  CollectionDeleteJob *job = new CollectionDeleteJob( collection, d->mSession );

  if ( d->mTopLevelCollection.remoteId().isEmpty() ) {
    const QString message = i18nc( "@info:status", "Configured storage location is empty" );
    kError() << message;
    kError() << collection;
    d->mSession->setError( job, Job::InvalidStoreState, message );
  } else if ( collection.remoteId().isEmpty() ||
              collection.parentCollection().remoteId().isEmpty() ) {
    const QString message = i18nc( "@info:status", "Given folder name is empty" );
    kError() << message;
    kError() << collection;
    d->mSession->setError( job, Job::InvalidJobContext, message );
  } else if ( ( collection.parentCollection().rights() & Collection::CanDeleteCollection ) == 0 ) {
    const QString message = i18nc( "@info:status", "Access control prohibits folder deletion in folder %1", collection.parentCollection().name() );
    kError() << message;
    kError() << collection;
    d->mSession->setError( job, Job::InvalidJobContext, message );
  }

  int errorCode = 0;
  QString errorText;
  checkCollectionDelete( job, errorCode, errorText );
  if ( errorCode != 0 ) {
    d->mSession->setError( job, errorCode, errorText );
  }

  return job;
}

CollectionFetchJob *AbstractLocalStore::fetchCollections( const Collection &collection,
                                                          CollectionFetchJob::Type type ) const
{
  CollectionFetchJob *job = new CollectionFetchJob( collection, type, d->mSession );

  if ( d->mTopLevelCollection.remoteId().isEmpty() ) {
    const QString message = i18nc( "@info:status", "Configured storage location is empty" );
    kError() << message;
    kError() << collection << "FetchType=" << type;
    d->mSession->setError( job, Job::InvalidStoreState, message );
  } else if ( collection.remoteId().isEmpty() ) {
    const QString message = i18nc( "@info:status", "Given folder name is empty" );
    kError() << message;
    kError() << collection << "FetchType=" << type;
    d->mSession->setError( job, Job::InvalidJobContext, message );
  }

  int errorCode = 0;
  QString errorText;
  checkCollectionFetch( job, errorCode, errorText );
  if ( errorCode != 0 ) {
    d->mSession->setError( job, errorCode, errorText );
  }

  return job;
}

CollectionModifyJob *AbstractLocalStore::modifyCollection( const Collection &collection )
{
  CollectionModifyJob *job = new CollectionModifyJob( collection, d->mSession );

  if ( d->mTopLevelCollection.remoteId().isEmpty() ) {
    const QString message = i18nc( "@info:status", "Configured storage location is empty" );
    kError() << message;
    kError() << collection;
    d->mSession->setError( job, Job::InvalidStoreState, message );
  } else if ( collection.remoteId().isEmpty() ||
              collection.parentCollection().remoteId().isEmpty() ) {
    const QString message = i18nc( "@info:status", "Given folder name is empty" );
    kError() << message;
    kError() << collection;
    d->mSession->setError( job, Job::InvalidJobContext, message );
  } else if ( ( collection.parentCollection().rights() & Collection::CanChangeCollection ) == 0 ) {
    const QString message = i18nc( "@info:status", "Access control prohibits folder modification in folder %1", collection.parentCollection().name() );
    kError() << message;
    kError() << collection;
    d->mSession->setError( job, Job::InvalidJobContext, message );
  }

  int errorCode = 0;
  QString errorText;
  checkCollectionModify( job, errorCode, errorText );
  if ( errorCode != 0 ) {
    d->mSession->setError( job, errorCode, errorText );
  }

  return job;
}

CollectionMoveJob *AbstractLocalStore::moveCollection( const Collection &collection, const Collection &targetParent )
{
  CollectionMoveJob *job = new CollectionMoveJob( collection, targetParent, d->mSession );

  if ( d->mTopLevelCollection.remoteId().isEmpty() ) {
    const QString message = i18nc( "@info:status", "Configured storage location is empty" );
    kError() << message;
    kError() << collection << targetParent;
    d->mSession->setError( job, Job::InvalidStoreState, message );
  } else if ( collection.remoteId().isEmpty() ||
              collection.parentCollection().remoteId().isEmpty() ||
              targetParent.remoteId().isEmpty() ) {
    const QString message = i18nc( "@info:status", "Given folder name is empty" );
    kError() << message;
    kError() << collection << targetParent;
    d->mSession->setError( job, Job::InvalidJobContext, message );
  } else if ( ( targetParent.rights() & Collection::CanCreateCollection ) == 0 ) {
    const QString message = i18nc( "@info:status", "Access control prohibits folder creation in folder %1", targetParent.name() );
    kError() << message;
    kError() << collection << targetParent;
    d->mSession->setError( job, Job::InvalidJobContext, message );
  } else if ( ( collection.parentCollection().rights() & Collection::CanDeleteCollection ) == 0 ) {
    const QString message = i18nc( "@info:status", "Access control prohibits folder deletion in folder %1", collection.parentCollection().name() );
    kError() << message;
    kError() << collection << targetParent;
    d->mSession->setError( job, Job::InvalidJobContext, message );
  }

  int errorCode = 0;
  QString errorText;
  checkCollectionMove( job, errorCode, errorText );
  if ( errorCode != 0 ) {
    d->mSession->setError( job, errorCode, errorText );
  }

  return job;
}

ItemFetchJob *AbstractLocalStore::fetchItems( const Collection &collection ) const
{
  ItemFetchJob *job = new ItemFetchJob( collection, d->mSession );

  if ( d->mTopLevelCollection.remoteId().isEmpty() ) {
    const QString message = i18nc( "@info:status", "Configured storage location is empty" );
    kError() << message;
    kError() << collection;
    d->mSession->setError( job, Job::InvalidStoreState, message );
  } else if ( collection.remoteId().isEmpty() ) {
    const QString message = i18nc( "@info:status", "Given folder name is empty" );
    kError() << message;
    kError() << collection;
    d->mSession->setError( job, Job::InvalidJobContext, message );
  }

  int errorCode = 0;
  QString errorText;
  checkItemFetch( job, errorCode, errorText );
  if ( errorCode != 0 ) {
    d->mSession->setError( job, errorCode, errorText );
  }

  return job;
}

ItemFetchJob *AbstractLocalStore::fetchItem( const Item &item ) const
{
  ItemFetchJob *job = new ItemFetchJob( item, d->mSession );

  if ( d->mTopLevelCollection.remoteId().isEmpty() ) {
    const QString message = i18nc( "@info:status", "Configured storage location is empty" );
    kError() << message;
    kError() << "Item(remoteId=" << item.remoteId() << ", mimeType=" << item.mimeType()
             << ", parentCollection=" << item.parentCollection().remoteId() << ")";
    d->mSession->setError( job, Job::InvalidStoreState, message );
  } else if ( item.remoteId().isEmpty() ) {
    const QString message = i18nc( "@info:status", "Given item identifier is empty" );
    kError() << message;
    kError() << "Item(remoteId=" << item.remoteId() << ", mimeType=" << item.mimeType()
             << ", parentCollection=" << item.parentCollection().remoteId() << ")";
    d->mSession->setError( job, Job::InvalidJobContext, message );
  }

  int errorCode = 0;
  QString errorText;
  checkItemFetch( job, errorCode, errorText );
  if ( errorCode != 0 ) {
    d->mSession->setError( job, errorCode, errorText );
  }

  return job;
}

ItemCreateJob *AbstractLocalStore::createItem( const Item &item, const Collection &collection )
{
  ItemCreateJob *job = new ItemCreateJob( item, collection, d->mSession );

  if ( d->mTopLevelCollection.remoteId().isEmpty() ) {
    const QString message = i18nc( "@info:status", "Configured storage location is empty" );
    kError() << message;
    kError() << collection
             << "Item(remoteId=" << item.remoteId() << ", mimeType=" << item.mimeType()
             << ")";
    d->mSession->setError( job, Job::InvalidStoreState, message );
  } else if ( collection.remoteId().isEmpty() ) {
    const QString message = i18nc( "@info:status", "Given folder name is empty" );
    kError() << message;
    kError() << collection
             << "Item(remoteId=" << item.remoteId() << ", mimeType=" << item.mimeType()
             << ")";
    d->mSession->setError( job, Job::InvalidJobContext, message );
  } else if ( ( collection.rights() & Collection::CanCreateItem ) == 0 ) {
    const QString message = i18nc( "@info:status", "Access control prohibits item creation in folder %1", collection.name() );
    kError() << message;
    kError() << collection
             << "Item(remoteId=" << item.remoteId() << ", mimeType=" << item.mimeType()
             << ")";
    d->mSession->setError( job, Job::InvalidJobContext, message );
  }

  int errorCode = 0;
  QString errorText;
  checkItemCreate( job, errorCode, errorText );
  if ( errorCode != 0 ) {
    d->mSession->setError( job, errorCode, errorText );
  }

  return job;
}

ItemModifyJob *AbstractLocalStore::modifyItem( const Item &item )
{
  ItemModifyJob *job = new ItemModifyJob( item, d->mSession );

  if ( d->mTopLevelCollection.remoteId().isEmpty() ) {
    const QString message = i18nc( "@info:status", "Configured storage location is empty" );
    kError() << message;
    kError() << "Item(remoteId=" << item.remoteId() << ", mimeType=" << item.mimeType()
             << ", parentCollection=" << item.parentCollection().remoteId() << ")";
    d->mSession->setError( job, Job::InvalidStoreState, message );
  } else if ( item.remoteId().isEmpty() ) {
    const QString message = i18nc( "@info:status", "Given item identifier is empty" );
    kError() << message;
    kError() << "Item(remoteId=" << item.remoteId() << ", mimeType=" << item.mimeType()
             << ", parentCollection=" << item.parentCollection().remoteId() << ")";
    d->mSession->setError( job, Job::InvalidJobContext, message );
  } else if ( ( item.parentCollection().rights() & Collection::CanChangeItem ) == 0 ) {
    const QString message = i18nc( "@info:status", "Access control prohibits item modification in folder %1", item.parentCollection().name() );
    kError() << message;
    kError() << "Item(remoteId=" << item.remoteId() << ", mimeType=" << item.mimeType()
             << ", parentCollection=" << item.parentCollection().remoteId() << ")";
    d->mSession->setError( job, Job::InvalidJobContext, message );
  }

  int errorCode = 0;
  QString errorText;
  checkItemModify( job, errorCode, errorText );
  if ( errorCode != 0 ) {
    d->mSession->setError( job, errorCode, errorText );
  }

  return job;
}

ItemDeleteJob *AbstractLocalStore::deleteItem( const Item &item )
{
  ItemDeleteJob *job = new ItemDeleteJob( item, d->mSession );

  if ( d->mTopLevelCollection.remoteId().isEmpty() ) {
    const QString message = i18nc( "@info:status", "Configured storage location is empty" );
    kError() << message;
    kError() << "Item(remoteId=" << item.remoteId() << ", mimeType=" << item.mimeType()
             << ", parentCollection=" << item.parentCollection().remoteId() << ")";
    d->mSession->setError( job, Job::InvalidStoreState, message );
  } else if ( item.remoteId().isEmpty() ) {
    const QString message = i18nc( "@info:status", "Given item identifier is empty" );
    kError() << message;
    kError() << "Item(remoteId=" << item.remoteId() << ", mimeType=" << item.mimeType()
             << ", parentCollection=" << item.parentCollection().remoteId() << ")";
    d->mSession->setError( job, Job::InvalidJobContext, message );
  } else if ( ( item.parentCollection().rights() & Collection::CanDeleteItem ) == 0 ) {
    const QString message = i18nc( "@info:status", "Access control prohibits item deletion in folder %1", item.parentCollection().name() );
    kError() << message;
    kError() << "Item(remoteId=" << item.remoteId() << ", mimeType=" << item.mimeType()
             << ", parentCollection=" << item.parentCollection().remoteId() << ")";
    d->mSession->setError( job, Job::InvalidJobContext, message );
  }

  int errorCode = 0;
  QString errorText;
  checkItemDelete( job, errorCode, errorText );
  if ( errorCode != 0 ) {
    d->mSession->setError( job, errorCode, errorText );
  }

  return job;
}

ItemMoveJob *AbstractLocalStore::moveItem( const Item &item, const Collection &targetParent )
{
  ItemMoveJob *job = new ItemMoveJob( item, targetParent, d->mSession );

  if ( d->mTopLevelCollection.remoteId().isEmpty() ) {
    const QString message = i18nc( "@info:status", "Configured storage location is empty" );
    kError() << message;
    kError() << "Item(remoteId=" << item.remoteId() << ", mimeType=" << item.mimeType()
             << ", parentCollection=" << item.parentCollection().remoteId() << ")"
             << targetParent;
    d->mSession->setError( job, Job::InvalidStoreState, message );
  } else if ( item.parentCollection().remoteId().isEmpty() ||
              targetParent.remoteId().isEmpty() ) {
    const QString message = i18nc( "@info:status", "Given folder name is empty" );
    kError() << message;
    kError() << "Item(remoteId=" << item.remoteId() << ", mimeType=" << item.mimeType()
             << ", parentCollection=" << item.parentCollection().remoteId() << ")"
             << targetParent;
    d->mSession->setError( job, Job::InvalidJobContext, message );
  } else if ( ( targetParent.rights() & Collection::CanCreateItem ) == 0 ) {
    const QString message = i18nc( "@info:status", "Access control prohibits item creation in folder %1", targetParent.name() );
    kError() << message;
    kError() << "Item(remoteId=" << item.remoteId() << ", mimeType=" << item.mimeType()
             << ", parentCollection=" << item.parentCollection().remoteId() << ")"
             << targetParent;
    d->mSession->setError( job, Job::InvalidJobContext, message );
  } else if ( ( item.parentCollection().rights() & Collection::CanDeleteItem ) == 0 ) {
    const QString message = i18nc( "@info:status", "Access control prohibits item deletion in folder %1", item.parentCollection().name() );
    kError() << message;
    kError() << "Item(remoteId=" << item.remoteId() << ", mimeType=" << item.mimeType()
             << ", parentCollection=" << item.parentCollection().remoteId() << ")"
             << targetParent;
    d->mSession->setError( job, Job::InvalidJobContext, message );
  } else if ( item.remoteId().isEmpty() ) {
    const QString message = i18nc( "@info:status", "Given item identifier is empty" );
    kError() << message;
    kError() << "Item(remoteId=" << item.remoteId() << ", mimeType=" << item.mimeType()
             << ", parentCollection=" << item.parentCollection().remoteId() << ")"
             << targetParent;
    d->mSession->setError( job, Job::InvalidJobContext, message );
  }

  int errorCode = 0;
  QString errorText;
  checkItemMove( job, errorCode, errorText );
  if ( errorCode != 0 ) {
    d->mSession->setError( job, errorCode, errorText );
  }

  return job;
}

StoreCompactJob *AbstractLocalStore::compactStore()
{
  StoreCompactJob *job = new StoreCompactJob( d->mSession );

  if ( d->mTopLevelCollection.remoteId().isEmpty() ) {
    const QString message = i18nc( "@info:status", "Configured storage location is empty" );
    kError() << message;
    d->mSession->setError( job, Job::InvalidStoreState, message );
  }

  int errorCode = 0;
  QString errorText;
  checkStoreCompact( job, errorCode, errorText );
  if ( errorCode != 0 ) {
    d->mSession->setError( job, errorCode, errorText );
  }

  return job;
}

Job *AbstractLocalStore::currentJob() const
{
  return d->mCurrentJob;
}

void AbstractLocalStore::notifyError( int errorCode, const QString &errorText ) const
{
  Q_ASSERT( d->mCurrentJob != 0);

  d->mSession->setError( d->mCurrentJob, errorCode, errorText );
}

void AbstractLocalStore::notifyCollectionsProcessed( const Collection::List &collections ) const
{
  Q_ASSERT( d->mCurrentJob != 0);

  d->mCollectionsProcessedNotifier.setCollections( collections );
  d->mCurrentJob->accept( &( d->mCollectionsProcessedNotifier ) );
}

void AbstractLocalStore::notifyItemsProcessed( const Item::List &items ) const
{
  Q_ASSERT( d->mCurrentJob != 0);

  d->mItemsProcessedNotifier.setItems( items );
  d->mCurrentJob->accept( &( d->mItemsProcessedNotifier ) );
}

void AbstractLocalStore::setTopLevelCollection( const Collection &collection )
{
  d->mTopLevelCollection = collection;
  d->mTopLevelCollectionFetcher.setTopLevelCollection( collection );
}

void AbstractLocalStore::checkCollectionCreate( CollectionCreateJob *job, int &errorCode, QString &errorText ) const
{
  Q_UNUSED( job );
  Q_UNUSED( errorCode );
  Q_UNUSED( errorText );
}

void AbstractLocalStore::checkCollectionDelete( CollectionDeleteJob *job, int &errorCode, QString &errorText ) const
{
  Q_UNUSED( job );
  Q_UNUSED( errorCode );
  Q_UNUSED( errorText );
}

void AbstractLocalStore::checkCollectionFetch( CollectionFetchJob *job, int &errorCode, QString &errorText ) const
{
  Q_UNUSED( job );
  Q_UNUSED( errorCode );
  Q_UNUSED( errorText );
}

void AbstractLocalStore::checkCollectionModify( CollectionModifyJob *job, int &errorCode, QString &errorText ) const
{
  Q_UNUSED( job );
  Q_UNUSED( errorCode );
  Q_UNUSED( errorText );
}

void AbstractLocalStore::checkCollectionMove( CollectionMoveJob *job, int &errorCode, QString &errorText ) const
{
  Q_UNUSED( job );
  Q_UNUSED( errorCode );
  Q_UNUSED( errorText );
}

void AbstractLocalStore::checkItemCreate( ItemCreateJob *job, int &errorCode, QString &errorText ) const
{
  Q_UNUSED( job );
  Q_UNUSED( errorCode );
  Q_UNUSED( errorText );
}

void AbstractLocalStore::checkItemDelete( ItemDeleteJob *job, int &errorCode, QString &errorText ) const
{
  Q_UNUSED( job );
  Q_UNUSED( errorCode );
  Q_UNUSED( errorText );
}

void AbstractLocalStore::checkItemFetch( ItemFetchJob *job, int &errorCode, QString &errorText ) const
{
  Q_UNUSED( job );
  Q_UNUSED( errorCode );
  Q_UNUSED( errorText );
}

void AbstractLocalStore::checkItemModify( ItemModifyJob *job, int &errorCode, QString &errorText ) const
{
  Q_UNUSED( job );
  Q_UNUSED( errorCode );
  Q_UNUSED( errorText );
}

void AbstractLocalStore::checkItemMove( ItemMoveJob *job, int &errorCode, QString &errorText ) const
{
  Q_UNUSED( job );
  Q_UNUSED( errorCode );
  Q_UNUSED( errorText );
}

void AbstractLocalStore::checkStoreCompact( StoreCompactJob *job, int &errorCode, QString &errorText ) const
{
  Q_UNUSED( job );
  Q_UNUSED( errorCode );
  Q_UNUSED( errorText );
}

#include "abstractlocalstore.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
