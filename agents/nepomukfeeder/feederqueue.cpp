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
#include <Soprano/QueryResultIterator>
#include <nie.h>

#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemFetchScope>

#include <KLocalizedString>
#include <KUrl>
#include <KJob>

#include <QDateTime>

#include <aneo.h>

#include "nepomukhelpers.h"
#include "nepomukfeeder-config.h"

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

void FeederQueue::setIndexingSpeed(FeederQueue::IndexingSpeed speed)
{
    const int s_reducedSpeedDelay = 500; // ms
    const int s_snailPaceDelay = 3000;   // ms

    kDebug() << speed;

    //
    // The low prio queue is always throttled a little more than the high prio one
    //
    if ( speed == FullSpeed ) {
        lowPrioQueue.setProcessingDelay(0);
        highPrioQueue.setProcessingDelay(0);
    }
    else {
        lowPrioQueue.setProcessingDelay(s_snailPaceDelay);
        highPrioQueue.setProcessingDelay(s_reducedSpeedDelay);
    }
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
  kDebug() << "Indexing collection " << mCurrentCollection.name() << mCurrentCollection.id();

  // process the collection only if it has not already been indexed
  // we check if the collection already has been indexed with the following values
  // - nie:url needs to be set
  // - aneo:akonadiIndexCompatLevel needs to match the indexer's level
  if ( !mReIndex &&
        Nepomuk::ResourceManager::instance()->mainModel()->executeQuery(QString::fromLatin1("ask where { ?r %1 %2 ; %3 %4 . }")
                                                                        .arg(Soprano::Node::resourceToN3(Vocabulary::NIE::url()),
                                                                            Soprano::Node::resourceToN3(mCurrentCollection.url()),
                                                                            Soprano::Node::resourceToN3(Vocabulary::ANEO::akonadiIndexCompatLevel()),
                                                                            Soprano::Node::literalToN3(NEPOMUK_FEEDER_INDEX_COMPAT_LEVEL)),
                                                                            Soprano::Query::QueryLanguageSparql).boolValue() ) {
    kDebug() << "already indexed collection: " << mCurrentCollection.id() << " skipping";
    QTimer::singleShot(0, this, SLOT(processNextCollection()));
    return;
  }
  KJob *job = NepomukHelpers::addCollectionToNepomuk(mCurrentCollection);
  connect( job, SIGNAL(result(KJob*)), this, SLOT(jobResult(KJob*)));

  ItemFetchJob *itemFetch = new ItemFetchJob( mCurrentCollection, this );
  itemFetch->fetchScope().setCacheOnly( true );
  connect( itemFetch, SIGNAL(finished(KJob*)), SLOT(itemFetchResult(KJob*)) );
  ++mPendingJobs;
}

void FeederQueue::itemHeadersReceived( const Akonadi::Item::List& items )
{
  kDebug() << items.count();
  Akonadi::Item::List itemsToUpdate;
  foreach( const Item &item, items ) {
    if ( item.storageCollectionId() != mCurrentCollection.id() )
      continue; // stay away from links

    // update item if it does not exist or does not have a proper id
    // we check if the item already exists with the following values:
    // - nie:url needs to be set
    // - aneo:akonadiItemId needs to be set
    // - nie:lastModified needs to match the item's modification time
    // - aneo:akonadiIndexCompatLevel needs to match the indexer's level
    if ( mReIndex ||
         !Nepomuk::ResourceManager::instance()->mainModel()->executeQuery(QString::fromLatin1("ask where { ?r %1 %2 ; %3 %4 ; %5 %6 ; %7 %8 . }")
                                                                          .arg(Soprano::Node::resourceToN3(Vocabulary::NIE::url()),
                                                                               Soprano::Node::resourceToN3(item.url()),
                                                                               Soprano::Node::resourceToN3(Vocabulary::ANEO::akonadiItemId()),
                                                                               Soprano::Node::literalToN3(QString::number(item.id())),
                                                                               Soprano::Node::resourceToN3(Vocabulary::NIE::lastModified()),
                                                                               Soprano::Node::literalToN3(item.modificationTime()),
                                                                               Soprano::Node::resourceToN3(Vocabulary::ANEO::akonadiIndexCompatLevel()),
                                                                               Soprano::Node::literalToN3(NEPOMUK_FEEDER_INDEX_COMPAT_LEVEL)),
                                                                          Soprano::Query::QueryLanguageSparql).boolValue() ) {
      itemsToUpdate.append( item );
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

  Akonadi::ItemFetchJob *fetchJob = qobject_cast<Akonadi::ItemFetchJob *>(job);
  Q_ASSERT(fetchJob);
  itemHeadersReceived(fetchJob->items());

  --mPendingJobs;
  if ( mPendingJobs == 0 && lowPrioQueue.isEmpty() ) { //Fetch jobs finished but there were no items in the collection
    collectionFullyIndexed();
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

void FeederQueue::collectionFullyIndexed()
{
    NepomukHelpers::markCollectionAsIndexed(mCurrentCollection);
    mCurrentCollection = Collection();
    emit idle( i18n( "Indexing completed." ) );
    //kDebug() << "indexing of collection " << mCurrentCollection.id() << " completed";
    processNextCollection();
}

void FeederQueue::processItemQueue()
{
  //kDebug();
  ++mProcessedAmount;
  if ( (mProcessedAmount % 100) == 0 && mTotalAmount > 0 && mProcessedAmount <= mTotalAmount )
    emit progress( (mProcessedAmount * 100) / mTotalAmount );
  
  if ( !mOnline ) {
    kDebug() << "not Online, stopping processing";
    return;
  } else if ( !highPrioQueue.isEmpty() ) {
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

  if ( !highPrioQueue.isEmpty() || ( !lowPrioQueue.isEmpty() && mOnline ) ) {
    //kDebug() << "continue";
    // go to eventloop before processing the next one, otherwise we miss the idle status change
    mProcessItemQueueTimer.start();
  }
}

void FeederQueue::prioQueueFinished()
{
  if (highPrioQueue.isEmpty() && lowPrioQueue.isEmpty() && (mPendingJobs == 0) && mCurrentCollection.isValid() ) {
    collectionFullyIndexed();
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

ItemQueue::ItemQueue(int batchSize, int fetchSize, QObject* parent)
: QObject(parent),
  mPendingRemoveDataJobs( 0 ),
  mBatchSize( batchSize ),
  mFetchSize( fetchSize ),
  mRunningJobs( 0 ),
  mProcessingDelay( 0 )
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
  //kDebug() << "pipline size: " << mItemPipeline.size() << mItemFetchList.size() << mFetchedItemList.size();
  if (mRunningJobs > 0) {//wait until the old graph has been saved
    //kDebug() << "blocked: " << mRunningJobs;
    return false;
  }
  Q_ASSERT(mRunningJobs == 0);
  mRunningJobs = 0;
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
    job->fetchScope().setAncestorRetrieval( ItemFetchScope::Parent );
    job->fetchScope().setCacheOnly( true );
    job->setProperty("numberOfItems", mItemFetchList.size());
    connect( job, SIGNAL(result(KJob*)), SLOT(fetchJobResult(KJob*)) );
    mRunningJobs++;
    mItemFetchList.clear();
    return false;
  } else { //In case there is nothing in the itemFetchList, but still in the fetchedItemList
    return processBatch();
  }
  return true;
}

void ItemQueue::fetchJobResult(KJob* job)
{
  mRunningJobs--;
  if ( job->error() ) {
    kWarning() << job->errorString();
    emit batchFinished();
  }
  Akonadi::ItemFetchJob *fetchJob = qobject_cast<Akonadi::ItemFetchJob*>(job);
  Q_ASSERT(fetchJob);
  int numberOfItems = fetchJob->property("numberOfItems").toInt();
  mFetchedItemList.append(fetchJob->items());
  if(fetchJob->items().size() != numberOfItems) {
    kWarning() << "Not all items were fetched: " << fetchJob->items().size() << numberOfItems;
  }
  processBatch();
}

bool ItemQueue::processBatch()
{
  //kDebug() << size;
  for ( int i = 0; i < mFetchedItemList.size() && i < mBatchSize; i++ ) {
    const Akonadi::Item &item = mFetchedItemList.takeFirst();
    //kDebug() << item.id();
    if (!item.hasPayload()) { //can happen due to deserialization error
      kWarning() << "failed to fetch item: " << item.id();
      continue;
    }
    Q_ASSERT(item.hasPayload());
    Q_ASSERT(mBatch.size() == 0 ? mResourceGraph.isEmpty() : true); //otherwise we havent reached removeDataByApplication yet, and therefore mustn't overwrite mResourceGraph
    NepomukHelpers::addItemToGraph( item, mResourceGraph );
    mBatch.append(item.url());
  }
  if ( mBatch.size() && ( mBatch.size() >= mBatchSize || mItemPipeline.isEmpty() ) ) {
    //kDebug() << "process batch of " << mBatch.size() << "      left: " << mFetchedItemList.size();
    KJob *job = Nepomuk::removeDataByApplication( mBatch, Nepomuk::RemoveSubResoures, KGlobal::mainComponent() );
    connect( job, SIGNAL( finished( KJob* ) ), this, SLOT( removeDataResult( KJob* ) ) );
    mRunningJobs++;
    mBatch.clear();
    return false;
  }
  return true;
}

void ItemQueue::removeDataResult(KJob* job)
{
  mRunningJobs--;
  if ( job->error() )
    kWarning() << job->errorString();

  //All old items have been removed, so we can now store the new items
  //kDebug() << "Saving Graph";
  KJob *addGraphJob = NepomukHelpers::addGraphToNepomuk( mResourceGraph );
  connect( addGraphJob, SIGNAL( result( KJob* ) ), SLOT( batchJobResult( KJob* ) ) );
  mRunningJobs++;
  //m_debugGraph = mResourceGraph;
  mResourceGraph.clear();
  //trigger processing of next collection as everything of this one has been stored
  //kDebug() << "removing completed, saving complete, batch done==================";
}

void ItemQueue::batchJobResult(KJob* job)
{
  mRunningJobs--;
  //kDebug() << "------------------------------------------";
  //kDebug() << "pipline size: " << mItemPipeline.size();
  //kDebug() << "fetchedItemList : " << mFetchedItemList.size();
  Q_ASSERT(mBatch.isEmpty());
  if ( job->error() ) {
    /*foreach( const Nepomuk::SimpleResource &res, m_debugGraph.toList() ) {
        kWarning() << res;
    }*/
    kWarning() << job->errorString();
  }
  QTimer::singleShot(mProcessingDelay, this, SLOT(continueProcessing()));
  mRunningJobs++;
}

void ItemQueue::continueProcessing()
{
  mRunningJobs--;
  if (processBatch()) { //Go back for more
    //kDebug() << "batch finished";
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

void ItemQueue::setProcessingDelay(int ms)
{
    mProcessingDelay = ms;
}


#include "feederqueue.moc"
