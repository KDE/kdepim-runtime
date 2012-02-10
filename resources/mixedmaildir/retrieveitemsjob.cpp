/*  This file is part of the KDE project
    Copyright (c) 2011 Kevin Krammer <kevin.krammer@gmx.at>

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

#include "retrieveitemsjob.h"

#include "mixedmaildirstore.h"

#include "filestore/itemfetchjob.h"

#include <akonadi/kmime/messageparts.h>
#include <akonadi/kmime/messagestatus.h>

#include <akonadi/collection.h>
#include <akonadi/collectionmodifyjob.h>
#include <akonadi/item.h>
#include <akonadi/itemcreatejob.h>
#include <akonadi/itemdeletejob.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/itemmodifyjob.h>
#include <akonadi/transactionsequence.h>

#include <QDateTime>
#include <QQueue>
#include <QVariant>

using namespace Akonadi;

class RetrieveItemsJob::Private
{
  RetrieveItemsJob *const q;

  public:
    Private( RetrieveItemsJob *parent, const Collection &collection, MixedMaildirStore *store )
      : q( parent ), mCollection( collection ), mStore( store ),
        mTransaction( 0 ), mHighestModTime( -1 )
    {
    }

    TransactionSequence *transaction()
    {
      if ( !mTransaction ) {
        mTransaction = new TransactionSequence( q );
	mTransaction->setAutomaticCommittingEnabled( false );
        connect( mTransaction, SIGNAL( result( KJob* ) ),
                 q, SLOT( transactionResult( KJob* ) ) );
      }
      return mTransaction;
    }

  public:
    const Collection mCollection;
    MixedMaildirStore *const mStore;
    TransactionSequence *mTransaction;

    QHash<QString, Item> mServerItemsByRemoteId;

    QQueue<Item> mNewItems;
    QQueue<Item> mChangedItems;
    Item::List mAvailableItems;
    Item::List mItemsMarkedAsDeleted;

    qint64 mHighestModTime;

  public: // slots
    void akonadiFetchResult( KJob *job );
    void transactionResult( KJob *job );
    void storeListResult( KJob* );
    void processNewItem();
    void fetchNewResult( KJob* );
    void processChangedItem();
    void fetchChangedResult( KJob* );
};

void RetrieveItemsJob::Private::akonadiFetchResult( KJob *job )
{
  if ( job->error() != 0 ) return; // handled by base class

  ItemFetchJob *itemFetch = qobject_cast<ItemFetchJob*>( job );
  Q_ASSERT( itemFetch != 0 );

  const Item::List items = itemFetch->items();
  kDebug( KDE_DEFAULT_DEBUG_AREA ) << "Akonadi fetch got" << items.count() << "items";

  mServerItemsByRemoteId.reserve( items.size() );
  Q_FOREACH ( const Item &item, items ) {
    // items without remoteId have not been written to the resource yet
    if ( !item.remoteId().isEmpty() ) {
      // set the parent collection (with all ancestors) in every item
      Item copy( item );
      copy.setParentCollection( mCollection );
      mServerItemsByRemoteId.insert( copy.remoteId(), copy );
    }
  }

  kDebug( KDE_DEFAULT_DEBUG_AREA ) << "of which" << mServerItemsByRemoteId.count() << "have remoteId";

  FileStore::ItemFetchJob *storeFetch = mStore->fetchItems( mCollection );
  // just basic items, no data

  connect( storeFetch, SIGNAL( result( KJob* ) ), q, SLOT( storeListResult( KJob* ) ) );
}

void RetrieveItemsJob::Private::storeListResult( KJob *job )
{
  kDebug() << "storeList->error=" << job->error();
  FileStore::ItemFetchJob *storeList = qobject_cast<FileStore::ItemFetchJob*>( job );
  Q_ASSERT( storeList != 0 );

  if ( storeList->error() != 0 ) {
    q->setError( storeList->error() );
    q->setErrorText( storeList->errorText() );
    q->emitResult();
    return;
  }

  // if some items have tags, we need to complete the retrieval and schedule tagging
  // to a later time so we can then fetch the items to get their Akonadi URLs
  // forward the property to this instance so the resource can take care of that
  const QVariant var = storeList->property( "remoteIdToTagList" );
  if ( var.isValid() ) {
    q->setProperty( "remoteIdToTagList", var );
  }

  const qint64 collectionTimestamp = mCollection.remoteRevision().toLongLong();

  const Item::List storedItems = storeList->items();
  Q_FOREACH( const Item &item, storedItems ) {
    // messages marked as deleted have been deleted from mbox files but never got purged
    Akonadi::MessageStatus status;
    status.setStatusFromFlags( item.flags() );
    if ( status.isDeleted() ) {
      mItemsMarkedAsDeleted << item;
      continue;
    }

    mAvailableItems << item;

    const QHash<QString, Item>::iterator it = mServerItemsByRemoteId.find( item.remoteId() );
    if ( it == mServerItemsByRemoteId.end() ) {
      // item not in server items -> new
      mNewItems << item;
    } else {
      // item both on server and in store, check modification time
      const QDateTime modTime = item.modificationTime();
      if ( !modTime.isValid() || modTime.toMSecsSinceEpoch() > collectionTimestamp ) {
        mChangedItems << it.value();
      }

      // remove from hash so only no longer existing items remain
      mServerItemsByRemoteId.erase( it );
    }
  }

  kDebug( KDE_DEFAULT_DEBUG_AREA ) << "Store fetch got" << storedItems.count() << "items"
                                   << "of which" << mNewItems.count() << "are new and" << mChangedItems.count()
                                   << "are changed and" << mServerItemsByRemoteId.count()
                                   << "need to be removed";

  // all items remaining in mServerItemsByRemoteId are no longer in the store

  if ( !mServerItemsByRemoteId.isEmpty() ) {
    ItemDeleteJob *deleteJob = new ItemDeleteJob( mServerItemsByRemoteId.values(), transaction() );
    transaction()->setIgnoreJobFailure( deleteJob );
  }

  processNewItem();
}

void RetrieveItemsJob::Private::processNewItem()
{
  if ( mNewItems.isEmpty() ) {
    processChangedItem();
    return;
  }

  const Item item = mNewItems.dequeue();
  FileStore::ItemFetchJob *storeFetch = mStore->fetchItem( item );
  storeFetch->fetchScope().fetchPayloadPart( MessagePart::Envelope );

  connect( storeFetch, SIGNAL( result( KJob* ) ), q, SLOT( fetchNewResult( KJob* ) ) );
}

void RetrieveItemsJob::Private::fetchNewResult( KJob *job )
{
  FileStore::ItemFetchJob *fetchJob = qobject_cast<FileStore::ItemFetchJob*>( job );
  Q_ASSERT( fetchJob != 0 );

  if ( fetchJob->items().count() != 1 ) {
    const Item item = fetchJob->item();
    kWarning() << "Store fetch for new item" << item.remoteId()
               << "in collection" << item.parentCollection().id()
               << "," << item.parentCollection().remoteId()
               << "did not return the expected item. error="
               << fetchJob->error() << "," << fetchJob->errorText();
    processNewItem();
    return;
  }

  const Item item = fetchJob->items().first();
  const QDateTime modTime = item.modificationTime();
  if ( modTime.isValid() ) {
    mHighestModTime = qMax( modTime.toMSecsSinceEpoch(), mHighestModTime );
  }

  ItemCreateJob *itemCreate = new ItemCreateJob( item, mCollection, transaction() );
  Q_UNUSED( itemCreate );
  QMetaObject::invokeMethod( q, "processNewItem", Qt::QueuedConnection );
}

void RetrieveItemsJob::Private::processChangedItem()
{
  if ( mChangedItems.isEmpty() ) {
    if ( !mTransaction ) {
      // no jobs created here -> done
      q->emitResult();
      return;
    } 
    
    if ( mHighestModTime > -1 ) {
      Collection collection( mCollection );
      collection.setRemoteRevision( QString::number( mHighestModTime ) );
      CollectionModifyJob *job = new CollectionModifyJob( collection, transaction() );
      transaction()->setIgnoreJobFailure( job );
    }
    transaction()->commit();
    return;
  }

  const Item item = mChangedItems.dequeue();
  FileStore::ItemFetchJob *storeFetch = mStore->fetchItem( item );
  storeFetch->fetchScope().fetchPayloadPart( MessagePart::Envelope );

  connect( storeFetch, SIGNAL( result( KJob* ) ), q, SLOT( fetchChangedResult( KJob* ) ) );
}

void RetrieveItemsJob::Private::fetchChangedResult( KJob *job )
{
  FileStore::ItemFetchJob *fetchJob = qobject_cast<FileStore::ItemFetchJob*>( job );
  Q_ASSERT( fetchJob != 0 );

  if ( fetchJob->items().count() != 1 ) {
    const Item item = fetchJob->item();
    kWarning() << "Store fetch for changed item" << item.remoteId()
               << "in collection" << item.parentCollection().id()
               << "," << item.parentCollection().remoteId()
               << "did not return the expected item. error="
               << fetchJob->error() << "," << fetchJob->errorText();
    processChangedItem();
    return;
  }

  const Item item = fetchJob->items().first();
  const QDateTime modTime = item.modificationTime();
  if ( modTime.isValid() ) {
    mHighestModTime = qMax( modTime.toMSecsSinceEpoch(), mHighestModTime );
  }

  ItemModifyJob *itemModify = new ItemModifyJob( item, transaction() );
  Q_UNUSED( itemModify );
  QMetaObject::invokeMethod( q, "processChangedItem", Qt::QueuedConnection );
}

void RetrieveItemsJob::Private::transactionResult( KJob *job )
{
  if ( job->error() != 0 ) return; // handled by base class

  q->emitResult();
}

RetrieveItemsJob::RetrieveItemsJob( const Akonadi::Collection &collection, MixedMaildirStore *store, QObject* parent )
  : Job( parent ), d( new Private( this, collection, store ) )
{
  Q_ASSERT( d->mCollection.isValid() );
  Q_ASSERT( !d->mCollection.remoteId().isEmpty() );
  Q_ASSERT( d->mStore != 0 );
}

RetrieveItemsJob::~RetrieveItemsJob()
{
  delete d;
}

Collection RetrieveItemsJob::collection() const
{
  return d->mCollection;
}

Item::List RetrieveItemsJob::availableItems() const
{
  return d->mAvailableItems;
}

Item::List RetrieveItemsJob::itemsMarkedAsDeleted() const
{
  return d->mItemsMarkedAsDeleted;
}

void RetrieveItemsJob::doStart()
{
  ItemFetchJob *job = new Akonadi::ItemFetchJob( d->mCollection, this );
  connect( job, SIGNAL( result( KJob* ) ), this, SLOT( akonadiFetchResult( KJob* ) ) );
}

#include "retrieveitemsjob.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
