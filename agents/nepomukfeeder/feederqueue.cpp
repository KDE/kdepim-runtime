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
#include "dms-copy/simpleresource.h"
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
  mOnline( true ),
  lowPrioQueue(1, 100, this),
  highPrioQueue(1, 100, this)
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
  //kDebug() << online;
  mOnline = online;
  if (online)
    continueIndexing();
}

void FeederQueue::addCollection( const Akonadi::Collection &collection )
{
  //kDebug() << collection.id();
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
  kDebug() << "Indexing collection" << mCurrentCollection.name();
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
  kDebug() << items.count();
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
    lowPrioQueue.addItems(itemsToUpdate);
    mTotalAmount += itemsToUpdate.size();
    mProcessItemQueueTimer.start();
  }
}

void FeederQueue::itemFetchResult(KJob* job)
{
  if ( job->error() )
    kWarning() << job->errorString();

  --mPendingJobs;
  if ( mPendingJobs == 0 && lowPrioQueue.isEmpty() ) { //Fetch jobs finished but there were no items in the collection
    mCurrentCollection = Collection();
    emit idle( i18n( "Indexing completed." ) );
    //kDebug() << "Indexing completed.";

    processNextCollection();
    return;
  }
}

void FeederQueue::addItem( const Akonadi::Item &item )
{
  kDebug() << item.id();
  highPrioQueue.addItem( item );
  mProcessItemQueueTimer.start();
}

bool FeederQueue::isEmpty()
{
  return highPrioQueue.isEmpty() && lowPrioQueue.isEmpty() && mCollectionQueue.isEmpty();
}

void FeederQueue::continueIndexing()
{
  kDebug();
  mProcessItemQueueTimer.start();
}

void FeederQueue::processItemQueue()
{
  //kDebug();
  ++mProcessedAmount;
  if ( (mProcessedAmount % 100) == 0 && mTotalAmount > 0 && mProcessedAmount <= mTotalAmount )
    emit progress( (mProcessedAmount * 100) / mTotalAmount );
  
  if ( !highPrioQueue.isEmpty() ) {
    //kDebug() << "high";
    if (!highPrioQueue.processItem()) {
      return;
    }
  } else if ( !mOnline ) {
    kDebug() << "not Online, stopping processing";
    return;
  } else if ( !lowPrioQueue.isEmpty() ){ 
    //kDebug() << "low";
    if (!lowPrioQueue.processItem()) {
      return;
    }
  } else {
    //kDebug() << "idle";
    emit idle( i18n( "Ready to index data." ) );
  }

  if ( !highPrioQueue.isEmpty() || ( !lowPrioQueue.isEmpty() && mOnline ) ) {
    //kDebug() << "continue";
    // go to eventloop before processing the next one, otherwise we miss the idle status change
    mProcessItemQueueTimer.start();
  }
}

void FeederQueue::prioQueueFinished()
{
  if (highPrioQueue.isEmpty() && lowPrioQueue.isEmpty() && (mPendingJobs == 0) && mCurrentCollection.isValid() ) {
    //kDebug() << "indexing of collection " << mCurrentCollection.id() << " completed";
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



ItemQueue::ItemQueue(int batchSize, int fetchSize, QObject* parent)
: QObject(parent),
  mPendingRemoveDataJobs( 0 ),
  mBatchSize( batchSize ),
  mFetchSize( fetchSize ),
  block( false )
{
  if ( fetchSize < batchSize )  {
    kWarning() << "fetchSize must be >= batchsize";
    fetchSize = batchSize;
  }
}

ItemQueue::~ItemQueue()
{

}


void ItemQueue::addItem(const Akonadi::Item &item)
{
  //kDebug() << "pipline size: " << mItemPipeline.size();
  mItemPipeline.enqueue(item.id()); //TODO if payload is available add directly to 
}

void ItemQueue::addItems(const Akonadi::Item::List &list )
{
  foreach (const Akonadi::Item &item, list) {
    addItem(item);
  }
}


bool ItemQueue::processItem()
{
  if (block) {//wait until the old graph has been saved
    //kDebug() << "blocked";
    return false;
  }
  //kDebug() << "------------------------procItem";
  static bool processing = false; // guard against sub-eventloop reentrancy
  if ( processing )
    return false;
  processing = true;
  if ( !mItemPipeline.isEmpty() ) {
    mItemFetchList.append( Akonadi::Item( mItemPipeline.dequeue() ) );
  }
  processing = false;
  
  if (mItemFetchList.size() >= mFetchSize || mItemPipeline.isEmpty() ) {
    //kDebug() << QString("Fetching %1 items").arg(mItemFetchList.size());
    Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob( mItemFetchList, 0 );
    job->fetchScope().fetchFullPayload();
    job->fetchScope().setCacheOnly( true );
    job->setProperty("numberOfItems", mItemFetchList.size());
    connect( job, SIGNAL( itemsReceived( Akonadi::Item::List ) ),
            SLOT( itemsReceived( Akonadi::Item::List ) ) );
    connect( job, SIGNAL(result(KJob*)), SLOT(fetchJobResult(KJob*)) );
    mItemFetchList.clear();
    block = true;
    return false;
  } else { //In case there is nothing in the itemFetchList, but still in the fetchedItemList
    return processBatch();
  }
  return true;
}

void ItemQueue::itemsReceived(const Akonadi::Item::List& items)
{
    Akonadi::ItemFetchJob *job = qobject_cast<Akonadi::ItemFetchJob*>(sender());
    int numberOfItems = job->property("numberOfItems").toInt();
    //kDebug() << items.size() << numberOfItems;
    mFetchedItemList.append(items);
    if ( mFetchedItemList.size() >= numberOfItems ) { //Sometimes we get a partial delivery only, wait for the rest
        processBatch();
    }
}

void ItemQueue::fetchJobResult(KJob* job)
{
  if ( job->error() ) {
    kWarning() << job->errorString();
    block = false;
    emit batchFinished();
  }
}

bool ItemQueue::processBatch()
{
  int size = mFetchedItemList.size();
  //kDebug() << size;
  for ( int i = 0; i < size && i < mBatchSize; i++ ) {
    const Akonadi::Item &item = mFetchedItemList.takeFirst();
    //kDebug() << item.id();
    Q_ASSERT(item.hasPayload());
    Q_ASSERT(mBatch.size() == 0 ? mResourceGraph.isEmpty() : true); //otherwise we havent reached removeDataByApplication yet, and therfore mustn't overwrite mResourceGraph
    NepomukHelpers::addItemToGraph( item, mResourceGraph );
    mBatch.append(item.url());
  }
  if ( mBatch.size() && ( mBatch.size() >= mBatchSize || mItemPipeline.isEmpty() ) ) {
    //kDebug() << "process batch of " << mBatch.size() << "      left: " << mFetchedItemList.size();
    KJob *job = Nepomuk::removeDataByApplication( mBatch, Nepomuk::RemoveSubResoures, KGlobal::mainComponent() );
    connect( job, SIGNAL( finished( KJob* ) ), this, SLOT( removeDataResult( KJob* ) ) );
    mBatch.clear();
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
  connect( addGraphJob, SIGNAL( result( KJob* ) ), SLOT( batchJobResult( KJob* ) ) );
  //m_debugGraph = mResourceGraph;
  mResourceGraph.clear();
  //trigger processing of next collection as everything of this one has been stored
  //kDebug() << "removing completed, saving complete, batch done==================";
}

void ItemQueue::batchJobResult(KJob* job)
{
  //kDebug() << "------------------------------------------";
  //kDebug() << "pipline size: " << mItemPipeline.size();
  //kDebug() << "fetchedItemList : " << mFetchedItemList.size();
  Q_ASSERT(mBatch.isEmpty());
  int timeout = 0;
  if ( job->error() ) {
    /*foreach( const Nepomuk::SimpleResource &res, m_debugGraph.toList() ) {
        kWarning() << res;
    }*/
    kWarning() << job->errorString();
    timeout = 30000; //Nepomuk is probably still working. Lets wait a bit and hope it has finished until the next batch arrives.
  }
  QTimer::singleShot(timeout, this, SLOT(continueProcessing()));
}

void ItemQueue::continueProcessing()
{
  if (processBatch()) { //Go back for more
    //kDebug() << "batch finished";
    block = false;
    emit batchFinished();
  } else {
      //kDebug() << "there was more...";
      return;
  }
  if ( mItemPipeline.isEmpty() && mFetchedItemList.isEmpty() ) {
    kDebug() << "indexing completed";
    emit finished();
  }
}


bool ItemQueue::isEmpty()
{
  return mItemPipeline.isEmpty() && mFetchedItemList.isEmpty();
}


#include "feederqueue.moc"
