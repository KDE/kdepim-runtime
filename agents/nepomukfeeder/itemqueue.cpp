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
#include <QFile>
#include <KDebug>
#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemFetchScope>
#include <nepomuk2/storeresourcesjob.h>

Q_DECLARE_METATYPE(Nepomuk2::SimpleResourceGraph);

ItemQueue::ItemQueue(int batchSize, int fetchSize, QObject* parent)
: QObject(parent),
  mBatchSize( batchSize ),
  mFetchSize( fetchSize ),
  mRunningJobs( 0 ),
  mProcessingDelay( 0 ),
  mItemsAreNotIndexed( false )
{
  mPropertyCache.setCachedTypes(QList<QUrl>()
     << QUrl("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#EmailAddress")
     << QUrl("http://www.semanticdesktop.org/ontologies/2007/03/22/nco#Contact")
     << QUrl("http://www.semanticdesktop.org/ontologies/2007/08/15/nao#FreeDesktopIcon")
     << QUrl("http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#MessageHeader")
  );
  if ( fetchSize < batchSize )  {
    kWarning() << "fetchSize must be >= batchsize";
    fetchSize = batchSize;
  }
}

ItemQueue::~ItemQueue()
{

}

void ItemQueue::setSaveFile(const QString& saveFile)
{
  mSaveFile = saveFile;
  loadState();
}

void ItemQueue::saveState()
{
  if (mSaveFile.isEmpty())
    return;
  QFile file( mSaveFile );
  if ( !file.open( QIODevice::WriteOnly ) ) {
    qWarning() << "could not save item pipeline to file " << file.fileName();
    return;
  }
  
  QDataStream stream( &file );
  stream.setVersion( QDataStream::Qt_4_7 );

  stream << (qulonglong)(mItemPipelineBackup.size());
  for ( int i = 0; i < mItemPipelineBackup.size(); ++i ) {
    stream << mItemPipelineBackup.at(i);
  }
}

void ItemQueue::loadState()
{
  QFile file( mSaveFile );
  if ( !file.open( QIODevice::ReadOnly ) )
    return;
  QDataStream stream( &file );
  stream.setVersion( QDataStream::Qt_4_7 );

  qulonglong size;
  Akonadi::Item::Id id;

  stream >> size;
  for ( qulonglong i = 0; i < size && !stream.atEnd(); ++i ) {
    stream >> id;
    mItemPipelineBackup.enqueue(id);
  }
  mItemPipeline = mItemPipelineBackup;
}

void ItemQueue::setItemsAreNotIndexed(bool enable)
{
  mItemsAreNotIndexed = enable;
}

void ItemQueue::addItem(const Akonadi::Item &item)
{
  //kDebug() << "pipline size: " << mItemPipeline.size();
  mItemPipeline.enqueue( item.id() ); //TODO if payload is available add directly to
  mItemPipelineBackup.enqueue( item.id() );
  saveState();
}

void ItemQueue::addItems(const Akonadi::Item::List &list )
{
  foreach ( const Akonadi::Item &item, list ) {
    addItem( item );
  }
}

bool ItemQueue::processItem()
{
  //kDebug() << "pipline size: " << mItemPipeline.size() << mItemFetchList.size() << mFetchedItemList.size();
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
    foreach(Akonadi::Item::Id id, mTempFetchList) {
      mItemPipelineBackup.removeOne(id);
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
      mItemPipelineBackup.removeOne(item.id());
      continue;
    }
    Q_ASSERT( item.hasPayload() );
    Q_ASSERT( mBatch.size() == 0 ? mResourceGraph.isEmpty() : true ); //otherwise we havent reached addGraphToNepomuk yet, and therefore mustn't overwrite mResourceGraph
    NepomukHelpers::addItemToGraph( item, mResourceGraph );
    mBatch.append( item.id() );
  }
  if ( mBatch.size() && ( mBatch.size() >= mBatchSize || mItemPipeline.isEmpty() ) ) {
    //kDebug() << "process batch of " << mBatch.size() << "      left: " << mFetchedItemList.size();
    KJob *addGraphJob = NepomukHelpers::addGraphToNepomuk( mPropertyCache.applyCache( mResourceGraph ), mItemsAreNotIndexed );
    addGraphJob->setProperty("graph", QVariant::fromValue(mResourceGraph));
    connect( addGraphJob, SIGNAL(result(KJob*)), SLOT(batchJobResult(KJob*)) );
    mRunningJobs++;
    foreach (Akonadi::Item::Id id, mBatch) {
      mItemPipelineBackup.removeOne(id);
    }
    saveState();
    mBatch.clear();
    mResourceGraph.clear();
    return false;
  }
  return true;
}

void ItemQueue::batchJobResult(KJob* job)
{
  mRunningJobs--;
  //kDebug() << "------------------------------------------";
  //kDebug() << "pipline size: " << mItemPipeline.size();
  //kDebug() << "fetchedItemList : " << mFetchedItemList.size();
  Q_ASSERT( mBatch.isEmpty() );
  if ( job->error() ) {
    /*foreach( const Nepomuk::SimpleResource &res, m_debugGraph.toList() ) {
        kWarning() << res;
    }*/
    kWarning() << job->errorString();
  } else {
    Nepomuk2::StoreResourcesJob *storeResourcesJob = static_cast<Nepomuk2::StoreResourcesJob*>(job);
    Q_ASSERT(storeResourcesJob);
    Nepomuk2::SimpleResourceGraph graph = storeResourcesJob->property("graph").value<Nepomuk2::SimpleResourceGraph>();
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
    if (!mItemPipelineBackup.isEmpty()) {
      kWarning() << mItemPipelineBackup;
    }
    Q_ASSERT(mItemPipelineBackup.isEmpty());
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
