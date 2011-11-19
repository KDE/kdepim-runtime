/*
    Copyright (C) 2011  Christian Mollekopf <chrigi_1@fastmail.fm>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "feederqueue.h"

#include <dms-copy/datamanagement.h>
#include <dms-copy/simpleresourcegraph.h>
#include <Nepomuk/ResourceManager>
#include <Soprano/Model>
#include <nie.h>

#include <Akonadi/ItemFetchJob>

#include <KLocalizedString>
#include <KUrl>
#include <KJob>

#include <aneo.h>

#include "nepomukhelpers.h"

using namespace Akonadi;

FeederQueue::FeederQueue( QObject* parent )
: QObject( parent ),
  mTotalAmount( 0 ),
  mProcessedAmount( 0 ),
  mPendingJobs( 0 ),
  mReIndex( false ),
  lowPrioQueue(1, this),
  highPrioQueue(1, this)
{
  mProcessItemQueueTimer.setInterval( 0 );
  mProcessItemQueueTimer.setSingleShot( true );
  connect( &mProcessItemQueueTimer, SIGNAL(timeout()), SLOT(processItemQueue()) );

  connect( &lowPrioQueue, SIGNAL(finished()), SLOT(prioQueueFinished()));
  connect( &highPrioQueue, SIGNAL(finished()), SLOT(prioQueueFinished()));
  connect( &lowPrioQueue, SIGNAL(batchFinished()), SLOT(batchFinished()));
  connect( &highPrioQueue, SIGNAL(batchFinished()), SLOT(batchFinished()));
}

FeederQueue::~FeederQueue()
{
  
}

void FeederQueue::setReindexing( bool reindex )
{
  mReIndex = reindex;
}

void FeederQueue::setOnline( bool online )
{
  mOnline = online;
  if (online)
    continueIndexing();
}

void FeederQueue::addCollection( const Akonadi::Collection &collection )
{
  //kDebug();
  mCollectionQueue.append( collection );
  if ( mPendingJobs == 0 ) {
    processNextCollection();
  }
}

void FeederQueue::processNextCollection()
{
  //kDebug();
  if ( mCurrentCollection.isValid() )
    return;
  mTotalAmount = 0;
  mProcessedAmount = 0;
  if ( mCollectionQueue.isEmpty() ) {
    //kDebug() << "fully indexed";
    mReIndex = false;
    emit fullyIndexed();
    return;
  }
  mCurrentCollection = mCollectionQueue.takeFirst();
  emit running( i18n( "Indexing collection '%1'...", mCurrentCollection.name() ));
  //kDebug() << "Indexing collection" << mCurrentCollection.name();
  //TODO maybe reindex anyways to be sure that type etc is correct
  if ( !Nepomuk::ResourceManager::instance()->mainModel()->containsAnyStatement( Soprano::Node(), Vocabulary::NIE::url(), mCurrentCollection.url() ) ) {
    //kDebug() << "adding collection to nepomuk";
    KJob *job = NepomukHelpers::addCollectionToNepomuk(mCurrentCollection);
    connect( job, SIGNAL(result(KJob*)), this, SLOT(jobResult(KJob*)));
  }

  ItemFetchJob *itemFetch = new ItemFetchJob( mCurrentCollection, this );
  itemFetch->fetchScope().setCacheOnly( true );
  connect( itemFetch, SIGNAL(itemsReceived(Akonadi::Item::List)), SLOT( itemHeadersReceived(Akonadi::Item::List)) );
  connect( itemFetch, SIGNAL(result(KJob*)), SLOT(itemFetchResult(KJob*)) );
  ++mPendingJobs;
}

void FeederQueue::itemHeadersReceived( const Akonadi::Item::List& items )
{
  //kDebug() << items.count();
  Akonadi::Item::List itemsToUpdate;
  foreach( const Item &item, items ) {
    if ( item.storageCollectionId() != mCurrentCollection.id() )
      continue; // stay away from links

    // update item if it does not exist
    if ( !Nepomuk::ResourceManager::instance()->mainModel()->containsAnyStatement( Soprano::Node(),  Vocabulary::NIE::url(), item.url() ) ) {
      itemsToUpdate.append( item );

    // the item exists. Check if it has an item ID property, otherwise re-index it.
    } else { //TODO maybe reindex anyways to be sure that type etc is correct
      if ( !Nepomuk::ResourceManager::instance()->mainModel()->containsAnyStatement( Soprano::Node(),
                                   Vocabulary::ANEO::akonadiItemId(), Soprano::LiteralValue( QUrl( QString::number( item.id() ) ) ) ) || mReIndex ) {
        itemsToUpdate.append( item );
      }
    }
  }

  if ( !itemsToUpdate.isEmpty() ) {
    ItemFetchJob *itemFetch = new ItemFetchJob( itemsToUpdate, this );
    itemFetch->setFetchScope( mItemFetchScope );
    connect( itemFetch, SIGNAL(itemsReceived(Akonadi::Item::List)), SLOT(itemsReceived(Akonadi::Item::List)) );
    connect( itemFetch, SIGNAL(result(KJob*)), SLOT(itemFetchResult(KJob*)) );
    ++mPendingJobs;
    mTotalAmount += itemsToUpdate.size();
  }
}

void FeederQueue::itemsReceived(const Akonadi::Item::List& items)
{
  //kDebug() << items.size();
  foreach ( Item item, items ) {
    item.setParentCollection( mCurrentCollection );
    lowPrioQueue.addItem( item );
  }
  mProcessItemQueueTimer.start();
}

void FeederQueue::itemFetchResult(KJob* job)
{
  if ( job->error() )
    kWarning() << job->errorString();
  ItemFetchJob *fetchJob = qobject_cast<ItemFetchJob*>(job);
  Q_UNUSED( fetchJob )
  //kDebug() << fetchJob->items().size();
  --mPendingJobs;
  if ( mPendingJobs == 0 && lowPrioQueue.isEmpty() ) { //Fetch jobs finished but there are were no items in the collection
    mCurrentCollection = Collection();
    emit idle( i18n( "Indexing completed." ) );
    //kDebug() << "Indexing completed.";

    processNextCollection();
    return;
  }
}

void FeederQueue::addItem( const Akonadi::Item &item )
{
  //kDebug() << item.id();
  if ( item.hasPayload() ) {
    highPrioQueue.addItem( item );
    mProcessItemQueueTimer.start();
  } else {
    if ( mItemFetchScope.fullPayload() || !mItemFetchScope.payloadParts().isEmpty() ) {
      ItemFetchJob *job = new ItemFetchJob( item );
      job->setFetchScope( mItemFetchScope );
      connect( job, SIGNAL( itemsReceived( Akonadi::Item::List ) ),
               SLOT( notificationItemsReceived( Akonadi::Item::List ) ) );
    }
  }
}

void FeederQueue::notificationItemsReceived(const Akonadi::Item::List& items)
{
  //kDebug() << items.size();
  foreach ( const Item &item, items ) {
    if ( !item.hasPayload() ) {
      continue;
    }
    highPrioQueue.addItem( item );
  }
  mProcessItemQueueTimer.start();
}


bool FeederQueue::isEmpty()
{
  return highPrioQueue.isEmpty() && lowPrioQueue.isEmpty() && mCollectionQueue.isEmpty();
}

void FeederQueue::continueIndexing()
{
  //kDebug();
  mProcessItemQueueTimer.start();
}

void FeederQueue::processItemQueue()
{
  //kDebug();
  ++mProcessedAmount;
  if ( (mProcessedAmount % 100) == 0 && mTotalAmount > 0 && mProcessedAmount <= mTotalAmount )
    emit progress( (mProcessedAmount * 100) / mTotalAmount );
  
  if ( !mOnline )
    return;
  
  if ( !highPrioQueue.isEmpty() ) {
    //kDebug() << "high";
    if (!highPrioQueue.processItem()) {
      return;
    }
  } else if ( !lowPrioQueue.isEmpty() ){ 
    //kDebug() << "low";
    if (!lowPrioQueue.processItem()) {
      return;
    }
  } else {
    //kDebug() << "idle";
    emit idle( i18n( "Ready to index data." ) );
  }

  if ( !highPrioQueue.isEmpty() || !lowPrioQueue.isEmpty() ) {
    //kDebug() << "continue";
    // go to eventloop before processing the next one, otherwise we miss the idle status change
    mProcessItemQueueTimer.start();
  }
}

void FeederQueue::prioQueueFinished()
{
  if (highPrioQueue.isEmpty() && lowPrioQueue.isEmpty() && (mPendingJobs == 0) && mCurrentCollection.isValid() ) {
    //kDebug() << "indexing completed";
    mCurrentCollection = Collection();
    emit idle( i18n( "Indexing completed." ) );
    processNextCollection();
  }
}

void FeederQueue::batchFinished()
{
  /*if ( sender() == &highPrioQueue )
    kDebug() << "high prio batch finished--------------------";
  if ( sender() == &lowPrioQueue )
    kDebug() << "low prio batch finished--------------------";*/
  if ( !highPrioQueue.isEmpty() || !lowPrioQueue.isEmpty() ) {
    //kDebug() << "continue";
    // go to eventloop before processing the next one, otherwise we miss the idle status change
    mProcessItemQueueTimer.start();
  }
}

void FeederQueue::jobResult(KJob* job)
{
  if ( job->error() )
    kWarning() << job->errorString();
}

const Akonadi::Collection& FeederQueue::currentCollection()
{
  return mCurrentCollection;
}

void FeederQueue::setItemFetchScope(ItemFetchScope scope)
{
  mItemFetchScope = scope;
}



ItemQueue::ItemQueue(int batchSize, QObject* parent)
: QObject(parent),
  mPendingRemoveDataJobs( 0 ),
  mBatchSize(batchSize), 
  block( false)
{

}

ItemQueue::~ItemQueue()
{

}


void ItemQueue::addItem(const Akonadi::Item &item)
{
  mItemPipeline.enqueue(item);
}


bool ItemQueue::processItem()
{
  if (block) {//wait until the old graph has been saved
    //kDebug() << "blocked";
    return false;
  }
  //kDebug();
  static bool processing = false; // guard against sub-eventloop reentrancy
  if ( processing )
    return false;
  processing = true;
  if ( !mItemPipeline.isEmpty() ) {
    const Akonadi::Item &item = mItemPipeline.dequeue();
    //kDebug() << item.id();
    Q_ASSERT(mBatch.size() == 0 ? mResourceGraph.isEmpty() : true); //otherwise we havent reached removeDataByApplication yet, and therfore mustn't overwrite mResourceGraph
    NepomukHelpers::addItemToGraph( item, mResourceGraph );
    mBatch.append(item.url());
  }
  processing = false;
  
  if ( mBatch.size() >= mBatchSize || mItemPipeline.isEmpty() ) {
    KJob *job = Nepomuk::removeDataByApplication( mBatch, Nepomuk::RemoveSubResoures, KGlobal::mainComponent() );
    connect( job, SIGNAL( finished( KJob* ) ), this, SLOT( removeDataResult( KJob* ) ) );
    mBatch.clear();
    //kDebug() << "store";
    block = true;
    return false;
  }
  return true;
}

void ItemQueue::removeDataResult(KJob* job)
{
  if ( job->error() )
    kWarning() << job->errorString();

  //All old items have been removed, so we can now store the new items
  //kDebug() << "Saving Graph";
  KJob *addGraphJob = NepomukHelpers::addGraphToNepomuk( mResourceGraph );
  connect( addGraphJob, SIGNAL( result( KJob* ) ), SLOT( jobResult( KJob* ) ) );

  mResourceGraph.clear();
  //trigger processing of next collection as everything of this one has been stored
  //kDebug() << "removing completed, saving complete, batch done==================";
}

void ItemQueue::jobResult(KJob* job)
{
  if ( job->error() )
    kWarning() << job->errorString();
  block = false;
  emit batchFinished();
  if ( mItemPipeline.isEmpty() ) {
    //kDebug() << "indexing completed";
    emit finished();
  }
}

bool ItemQueue::isEmpty()
{
  return mItemPipeline.isEmpty();
}


#include "feederqueue.moc"
