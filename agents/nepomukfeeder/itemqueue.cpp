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
#include <KUrl>

#include <Nepomuk2/DataManagement>
#include <Nepomuk2/StoreResourcesJob>
#include <Nepomuk2/ResourceManager>

#include <Soprano/Model>
#include <Soprano/QueryResultIterator>

#include <Nepomuk2/Vocabulary/NCO>
#include <Nepomuk2/Vocabulary/NMO>
#include <Nepomuk2/Vocabulary/NIE>
#include <Soprano/Vocabulary/NAO>

using namespace Nepomuk2::Vocabulary;
using namespace Soprano::Vocabulary;

Q_DECLARE_METATYPE(Nepomuk2::SimpleResourceGraph)

ItemQueue::ItemQueue(int batchSize, int fetchSize, QObject* parent)
: QObject(parent),
  mBatchSize( batchSize ),
  mFetchSize( fetchSize ),
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

int ItemQueue::processingDelay()
{
    return mDelay;
}


void ItemQueue::addItem(const Akonadi::Item &item)
{
    QDateTime modificationTime = item.modificationTime();
    if (modificationTime.isNull()) {
        // If the DateTime is not provided, it means highest priority
        modificationTime = QDateTime::currentDateTime();
    }

    mItemPipeline.insert(modificationTime, item.id());
}

void ItemQueue::addItems(const Akonadi::Item::List &list)
{
    foreach (const Akonadi::Item &item, list) {
        addItem( item );
    }
}

Akonadi::Item::List ItemQueue::fetchHighestPriorityItems(int numItems)
{
    Akonadi::Item::List list;
    list.reserve(numItems);

    int i = 0;
    QMutableMapIterator<QDateTime, Akonadi::Item::Id> iter(mItemPipeline);
    iter.toBack();

    while (iter.hasPrevious() && i < numItems) {
        iter.previous();
        i++;

        list << Akonadi::Item(iter.value());
        iter.remove();
    }

    return list;
}

bool ItemQueue::processBatch()
{
  kDebug() << "pipeline size: " << mItemPipeline.size() << mFetchedItemList.size();
  if ( runningJobCount() > 0 ) {//wait until the old graph has been saved
    kDebug() << "blocked: " << runningJobCount();
    return false;
  }
  Q_ASSERT( runningJobCount() == 0 );

  if ( mItemPipeline.isEmpty() && mFetchedItemList.isEmpty() ) {
    return false;
  }

  if ( mFetchedItemList.size() <= mBatchSize && !mItemPipeline.isEmpty() ) {
    // Get the list of items to fetch
    Akonadi::Item::List itemFetchList = fetchHighestPriorityItems( mFetchSize );
    kDebug() << "Fetching" << itemFetchList.size() << "items";

    Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob( itemFetchList, this );
    job->fetchScope().fetchFullPayload();
    job->fetchScope().setAncestorRetrieval( Akonadi::ItemFetchScope::Parent );
    job->fetchScope().setCacheOnly( true );
    job->fetchScope().setIgnoreRetrievalErrors( true );
    job->setProperty( "numberOfItems", itemFetchList.size() );

    connect( job, SIGNAL(result(KJob*)), SLOT(fetchJobResult(KJob*)) );
    addJob( job );
    return true;
  }

  return indexBatch();
}

void ItemQueue::fetchJobResult(KJob* job)
{
  removeJob( job );
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
    kDebug() << "Indexing" << item.id() << item.modificationTime();

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
    addJob( job );
    return true;
  }

  return false;
}

void ItemQueue::removeDataResult(KJob* job)
{
  removeJob( job );
  if ( job->error() )
    kWarning() << job->errorString();

  //All old items have been removed, so we can now store the new items
  const Nepomuk2::SimpleResourceGraph graph = job->property("graph").value<Nepomuk2::SimpleResourceGraph>();
  Nepomuk2::SimpleResourceGraph identifiedGraph = mPropertyCache.applyCache( graph );

  foreach(const QUrl& uri, identifiedGraph.allResourceUris()) {
      Nepomuk2::SimpleResource res = identifiedGraph[uri];
      if( res.contains(NMO::plainTextMessageContent()) ) {
          mPlainTextContent.insert( uri, res.property(NMO::plainTextMessageContent()).first().toString() );
          identifiedGraph[uri].remove(NMO::plainTextMessageContent());
      }
  }

  KJob *addGraphJob = NepomukHelpers::addGraphToNepomuk( mPropertyCache.applyCache( graph ) );
  addGraphJob->setProperty("graph", QVariant::fromValue( graph ));
  connect( addGraphJob, SIGNAL(result(KJob*)), SLOT(batchJobResult(KJob*)) );
  addJob( addGraphJob );
}

namespace {
    void setNmoPlainTextContent(const QUrl& uri, const QString& plainText_) {
        if( uri.isEmpty() || plainText_.isEmpty() )
            return;

        QString plainText( plainText_ );
        // This number has been experimentally chosen. Virtuoso cannot handle more than this
        static const int maxSize = 2490000;
        if( plainText.size() > maxSize )  {
            kWarning() << "Trimming plain text content from " << plainText.size() << " to " << maxSize;
            plainText.resize( maxSize );
        }

        QString uriN3 = Soprano::Node::resourceToN3( uri );

        QString query = QString::fromLatin1("select ?g where { graph ?g { %1 a aneo:AkonadiDataObject . } }")
        .arg ( uriN3 );
        Soprano::Model* model = Nepomuk2::ResourceManager::instance()->mainModel();
        Soprano::QueryResultIterator it = model->executeQuery( query, Soprano::Query::QueryLanguageSparqlNoInference );

        QUrl graph;
        if( it.next() ) {
            graph = it[0].uri();
            it.close();
        }

        if( !graph.isEmpty() ) {
            QString graphN3 = Soprano::Node::resourceToN3( graph );
            QString insertCommand = QString::fromLatin1("sparql insert { graph %1 { %2 nmo:plainTextMessageContent %3 . } }")
                                    .arg( graphN3, uriN3, Soprano::Node::literalToN3(plainText) );

            model->executeQuery( insertCommand, Soprano::Query::QueryLanguageUser, QLatin1String("sql") );
        }
    }

}
void ItemQueue::batchJobResult(KJob* job)
{
  removeJob( job );
  // FIXME: Only compute all of this if DEBUG messages have been enabled
  kDebug() << "------------------------------------------";
  kDebug() << "pipeline size: " << mItemPipeline.size();
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

    // Store the plain text
    const QHash<QUrl, QUrl> mappings = storeResourcesJob->mappings();
    QList<QUrl> keys = mappings.uniqueKeys();
    foreach(const QUrl& key, keys) {
        const QUrl uri = mappings.value(key);
        setNmoPlainTextContent( uri, mPlainTextContent.value(key) );
    }
    mPlainTextContent.clear();
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

void ItemQueue::clear()
{
  if ( runningJobCount() > 0 ) {
    kWarning() << "called with " << runningJobCount() << " jobs outstanding!";
    killAllJobs();
  }
  mItemPipeline.clear();
  mFetchedItemList.clear();
}

int ItemQueue::runningJobCount() const
{
  return mJobs.count();
}

void ItemQueue::addJob(KJob *job)
{
  if ( mJobs.contains(job) )
    kWarning() << "Job Already exists!";
  else
    mJobs.insert(job);
}

void ItemQueue::removeJob(KJob *job)
{
  if ( !mJobs.contains(job) )
    kWarning() << "Job does not exist!";
  else
    mJobs.remove(job);
}

void ItemQueue::killAllJobs()
{
  QSetIterator<KJob *> i(mJobs);
  while (i.hasNext()) {
     KJob *job = i.next();
     job->kill(KJob::Quietly);
  }
  mJobs.clear();
}

