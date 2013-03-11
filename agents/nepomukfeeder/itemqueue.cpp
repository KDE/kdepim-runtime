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

#include "itemqueue.h"
#include "nepomukhelpers.h"
#include <QTimer>
#include <KDebug>
#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemFetchScope>
#include <nepomuk2/storeresourcesjob.h>
#include <KUrl>

#include <Nepomuk2/Vocabulary/NCO>
#include <Nepomuk2/Vocabulary/NMO>
#include <Soprano/Vocabulary/NAO>

using namespace Nepomuk2::Vocabulary;
using namespace Soprano::Vocabulary;

Q_DECLARE_METATYPE(Nepomuk2::SimpleResourceGraph)

ItemQueue::ItemQueue(int batchSize, int fetchSize, QObject* parent)
: QObject(parent),
  mBatchSize( batchSize ),
  mFetchSize( fetchSize ),
  mRunningJobs( 0 ),
  mProcessingDelay( 0 ),
  mAverageIndexingTime(0),
  mNumberOfIndexedItems(0)
{
  mPropertyCache.setCachedTypes(QList<QUrl>()
     << NCO::EmailAddress()
     << NCO::Contact()
     << NAO::FreeDesktopIcon()
     << NMO::MessageHeader()
  );
  if ( fetchSize < batchSize )  {
    kWarning() << "fetchSize must be >= batchsize";
    fetchSize = batchSize;
  }
}

ItemQueue::~ItemQueue()
{

}

void ItemQueue::addToQueue(Akonadi::Entity::Id id)
{
  mItemPipeline.enqueue( id );
}

void ItemQueue::addItem(const Akonadi::Item &item)
{
  //kDebug() << "pipline size: " << mItemPipeline.size();
  addToQueue(item.id());
}

void ItemQueue::addItems(const Akonadi::Item::List &list )
{
  foreach ( const Akonadi::Item &item, list ) {
    addToQueue( item.id() );
  }
}

bool ItemQueue::processItem()
{
  kDebug() << "pipline size: " << mItemPipeline.size() << mItemFetchList.size() << mFetchedItemList.size();
  if ( mRunningJobs > 0 ) {//wait until the old graph has been saved
    //kDebug() << "blocked: " << mRunningJobs;
    return false;
  }
  Q_ASSERT( mRunningJobs == 0 );
  mRunningJobs = 0;
  //kDebug() << "------------------------procItem";
  if ( !mItemPipeline.isEmpty() ) {
    mItemFetchList.append( Akonadi::Item( mItemPipeline.dequeue() ) );
  }

  if ( mItemFetchList.size() >= mFetchSize || mItemPipeline.isEmpty() ) {
    //kDebug() << QString( "Fetching %1 items" ).arg( mItemFetchList.size() );
    Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob( mItemFetchList, this );
    job->fetchScope().fetchFullPayload();
    job->fetchScope().setAncestorRetrieval( Akonadi::ItemFetchScope::Parent );
    job->fetchScope().setCacheOnly( true );
    job->fetchScope().setIgnoreRetrievalErrors( true );
    foreach(const Akonadi::Item &it, mItemFetchList) {
      mTempFetchList.append(it.id());
    }
    job->setProperty( "numberOfItems", mItemFetchList.size() );
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
  Akonadi::ItemFetchJob *fetchJob = qobject_cast<Akonadi::ItemFetchJob*>( job );
  Q_ASSERT( fetchJob );
  int numberOfItems = fetchJob->property( "numberOfItems" ).toInt();
  mFetchedItemList.append( fetchJob->items() );
  if ( fetchJob->items().size() != numberOfItems ) {
    kWarning() << "Not all items were fetched: " << fetchJob->items().size() << numberOfItems;
    foreach(const Akonadi::Item &it, mItemFetchList) {
      mTempFetchList.removeOne(it.id());
    }
  }
  mTempFetchList.clear();
  
  if ( processBatch() && mBatch.isEmpty() ) { //Can happen if only items without payload were fetched
    emit batchFinished();
  }
}

bool ItemQueue::processBatch()
{
  //kDebug() << size;
  for ( int i = 0; i < mFetchedItemList.size() && i < mBatchSize; i++ ) {
    const Akonadi::Item &item = mFetchedItemList.takeFirst();
    //kDebug() << item.id();
    if ( !item.hasPayload() ) { //can happen due to deserialization error or with items that actually don't have a local payload
      kWarning() << "failed to fetch item or item without payload: " << item.id();
      continue;
    }
    Q_ASSERT( item.hasPayload() );
    Q_ASSERT( mBatch.size() == 0 ? mResourceGraph.isEmpty() : true ); //otherwise we havent reached addGraphToNepomuk yet, and therefore mustn't overwrite mResourceGraph
    NepomukHelpers::addItemToGraph( item, mResourceGraph );
    mBatch.append( item.id() );
  }
  if ( mBatch.size() && ( mBatch.size() >= mBatchSize || mItemPipeline.isEmpty() ) ) {
    //kDebug() << "process batch of " << mBatch.size() << "      left: " << mFetchedItemList.size();
    mTimer.start();
    
    QList<QUrl> batch;
    foreach (Akonadi::Item::Id id, mBatch) {
        batch << Akonadi::Item(id).url();
    }

    KJob *job = Nepomuk2::removeDataByApplication( batch, Nepomuk2::RemoveSubResoures, KGlobal::mainComponent() );
    job->setProperty("graph", QVariant::fromValue(mResourceGraph));
    connect( job, SIGNAL(finished(KJob*)), this, SLOT(removeDataResult(KJob*)) );
    mRunningJobs++;
    mBatch.clear();
    mResourceGraph.clear();
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
  const Nepomuk2::SimpleResourceGraph graph = job->property("graph").value<Nepomuk2::SimpleResourceGraph>();
  KJob *addGraphJob = NepomukHelpers::addGraphToNepomuk( mPropertyCache.applyCache( graph ) );
  addGraphJob->setProperty("graph", QVariant::fromValue( graph ));
  connect( addGraphJob, SIGNAL(result(KJob*)), SLOT(batchJobResult(KJob*)) );
  mRunningJobs++;
}

void ItemQueue::batchJobResult(KJob* job)
{
  mRunningJobs--;
  kDebug() << "------------------------------------------";
  kDebug() << "pipline size: " << mItemPipeline.size();
  kDebug() << "fetchedItemList : " << mFetchedItemList.size();

  kDebug() << "Indexing took(ms): " << mTimer.elapsed();
  mNumberOfIndexedItems++;
  mAverageIndexingTime += ((double)mTimer.elapsed()-mAverageIndexingTime)/(double)mNumberOfIndexedItems;
  kDebug() << "Average (ms): " << mAverageIndexingTime;
  const Nepomuk2::SimpleResourceGraph graph = job->property("graph").value<Nepomuk2::SimpleResourceGraph>();
  Q_ASSERT( mBatch.isEmpty() );
  if ( job->error() ) {
    kWarning() << "Error while storing graph";
    foreach( const Nepomuk2::SimpleResource &res, graph.toList() ) {
        kWarning() << res;
    }
    kWarning() << job->errorString();
  } else {
    Nepomuk2::StoreResourcesJob *storeResourcesJob = static_cast<Nepomuk2::StoreResourcesJob*>(job);
    Q_ASSERT(storeResourcesJob);
    mPropertyCache.fillCache(graph, storeResourcesJob->mappings());
  }
  QTimer::singleShot(mProcessingDelay, this, SLOT(continueProcessing()));
  mRunningJobs++;
}

void ItemQueue::continueProcessing()
{
  mRunningJobs--;
  if ( processBatch() ) { //Go back for more
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

bool ItemQueue::isEmpty() const
{
    return mItemPipeline.isEmpty() && mFetchedItemList.isEmpty();
}

void ItemQueue::setProcessingDelay(int ms)
{
    mProcessingDelay = ms;
}

#include "itemqueue.moc"
