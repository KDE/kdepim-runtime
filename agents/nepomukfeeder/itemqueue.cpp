/*
    Copyright (C) 2011  Christian Mollekopf <chrigi_1@fastmail.fm>
    Copyright (C) 2013  Vishesh Handa <me@vhanda.in>

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
  mDelay( 0 ),
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

void ItemQueue::setProcessingDelay(int delay)
{
  mDelay = delay;
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

bool ItemQueue::processBatch()
{
  kDebug() << "pipline size: " << mItemPipeline.size() << mFetchedItemList.size();
  if ( mRunningJobs > 0 ) {//wait until the old graph has been saved
    kDebug() << "blocked: " << mRunningJobs;
    return false;
  }
  Q_ASSERT( mRunningJobs == 0 );
  mRunningJobs = 0;

  if ( mItemPipeline.isEmpty() && mFetchedItemList.isEmpty() ) {
    return false;
  }

  if ( mFetchedItemList.size() <= mBatchSize && !mItemPipeline.isEmpty() ) {
    // Get the list of items to fetch
    Akonadi::Item::List itemFetchList;
    itemFetchList.reserve( mFetchSize );
    for ( int i=0; i<mFetchSize && !mItemPipeline.isEmpty(); i++ ) {
      itemFetchList << Akonadi::Item( mItemPipeline.dequeue() );
    }
    kDebug() << "Fetching" << itemFetchList.size() << "items";

    Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob( itemFetchList, this );
    job->fetchScope().fetchFullPayload();
    job->fetchScope().setAncestorRetrieval( Akonadi::ItemFetchScope::Parent );
    job->fetchScope().setCacheOnly( true );
    job->fetchScope().setIgnoreRetrievalErrors( true );
    job->setProperty( "numberOfItems", itemFetchList.size() );

    connect( job, SIGNAL(result(KJob*)), SLOT(fetchJobResult(KJob*)) );
    mRunningJobs++;
    return true;
  }

  return indexBatch();
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

  mFetchedItemList.append( fetchJob->items() );
  int numberOfItems = fetchJob->property( "numberOfItems" ).toInt();
  if ( fetchJob->items().size() != numberOfItems ) {
    kWarning() << "Not all items were fetched: " << fetchJob->items().size() << numberOfItems;
  }
  
  if ( !indexBatch() ) { //Can happen if only items without payload were fetched
    QTimer::singleShot( mDelay, this, SLOT(slotEmitFinished()) );
  }
}

bool ItemQueue::indexBatch()
{
  Nepomuk2::SimpleResourceGraph resourceGraph;
  QList<Akonadi::Item::Id> batch;

  while ( batch.size() < mBatchSize && !mFetchedItemList.isEmpty() ) {
    const Akonadi::Item &item = mFetchedItemList.takeFirst();
    //kDebug() << item.id();
    if ( !item.hasPayload() ) { //can happen due to deserialization error or with items that actually don't have a local payload
      kWarning() << "failed to fetch item or item without payload: " << item.id();
      continue;
    }
    Q_ASSERT( item.hasPayload() );

    NepomukHelpers::addItemToGraph( item, resourceGraph );
    batch.append( item.id() );
  }

  if ( batch.size() ) {
    //kDebug() << "process batch of " << batch.size() << "  left: " << mFetchedItemList.size();
    mTimer.start();

    QList<QUrl> akondiUrls;
    foreach (Akonadi::Item::Id id, batch) {
        akondiUrls << Akonadi::Item(id).url();
    }

    KJob *job = Nepomuk2::removeDataByApplication( akondiUrls, Nepomuk2::RemoveSubResoures, KGlobal::mainComponent() );
    job->setProperty("graph", QVariant::fromValue(resourceGraph));
    connect( job, SIGNAL(finished(KJob*)), this, SLOT(removeDataResult(KJob*)) );
    mRunningJobs++;
    return true;
  }

  return false;
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
  // FIXME: Only compute all of this if DEBUG messages have been enabled
  kDebug() << "------------------------------------------";
  kDebug() << "pipline size: " << mItemPipeline.size();
  kDebug() << "fetchedItemList : " << mFetchedItemList.size();

  kDebug() << "Indexing took(ms): " << mTimer.elapsed();
  mNumberOfIndexedItems++;
  mAverageIndexingTime += ((double)mTimer.elapsed()-mAverageIndexingTime)/(double)mNumberOfIndexedItems;
  kDebug() << "Average (ms): " << mAverageIndexingTime;
  const Nepomuk2::SimpleResourceGraph graph = job->property("graph").value<Nepomuk2::SimpleResourceGraph>();
  //FIXME: Better error handling - Store this in some error file?
  if ( job->error() ) {
    kWarning() << "Error while storing graph: " << job->errorString();
    foreach( const Nepomuk2::SimpleResource &res, graph.toList() ) {
        kDebug() << res;
    }
  } else {
    Nepomuk2::StoreResourcesJob *storeResourcesJob = static_cast<Nepomuk2::StoreResourcesJob*>(job);
    Q_ASSERT(storeResourcesJob);
    mPropertyCache.fillCache(graph, storeResourcesJob->mappings());
  }

  QTimer::singleShot( mDelay, this, SLOT(slotEmitFinished()) );
}

bool ItemQueue::isEmpty() const
{
  return mItemPipeline.isEmpty() && mFetchedItemList.isEmpty();
}

int ItemQueue::size() const
{
  return mItemPipeline.size() + mFetchedItemList.size();
}

void ItemQueue::slotEmitFinished()
{
  emit batchFinished();
  if ( isEmpty() )
      emit finished();
}


#include "itemqueue.moc"
