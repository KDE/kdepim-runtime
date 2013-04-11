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

#include <nie.h>

#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemFetchScope>
#include <Akonadi/Collection>

#include <KLocalizedString>
#include <KUrl>
#include <KJob>
#include <KIcon>
#include <KNotification>
#include <KIconLoader>

#include <QDateTime>

#include "nepomukhelpers.h"
#include "nepomukfeeder-config.h"

using namespace Akonadi;

FeederQueue::FeederQueue( QObject* parent )
: QObject( parent ),
  mTotalAmount( 0 ),
  mPendingJobs( 0 ),
  mReIndex( false ),
  mOnline( true ),
  lowPrioQueue(1, 100, this),
  highPrioQueue(1, 100, this),
  emailItemQueue(1, 100, this)
{
  mProcessItemQueueTimer.setInterval( 0 );
  mProcessItemQueueTimer.setSingleShot( true );
  connect( &mProcessItemQueueTimer, SIGNAL(timeout()), SLOT(processItemQueue()) );

  connect( &lowPrioQueue, SIGNAL(finished()), SLOT(prioQueueFinished()));
  connect( &highPrioQueue, SIGNAL(finished()), SLOT(prioQueueFinished()));
  connect( &emailItemQueue, SIGNAL(finished()), SLOT(prioQueueFinished()));
  connect( &lowPrioQueue, SIGNAL(batchFinished()), SLOT(batchFinished()));
  connect( &highPrioQueue, SIGNAL(batchFinished()), SLOT(batchFinished()));
  connect( &emailItemQueue, SIGNAL(batchFinished()), SLOT(batchFinished()));
}

FeederQueue::~FeederQueue()
{

}

void FeederQueue::setReindexing( bool reindex )
{
  // FIXME: This doesn't seem like it will do anything?
  //        Shouldn't it call some kind of signal?
  mReIndex = reindex;
}

void FeederQueue::setOnline( bool online )
{
  //kDebug() << online;
  mOnline = online;
  if ( online )
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
        lowPrioQueue.setProcessingDelay( 0 );
        highPrioQueue.setProcessingDelay( 0 );
        emailItemQueue.setProcessingDelay( 0 );
    } else {
        lowPrioQueue.setProcessingDelay( s_snailPaceDelay );
        highPrioQueue.setProcessingDelay( s_reducedSpeedDelay );
        emailItemQueue.setProcessingDelay( s_snailPaceDelay );
    }
}

void FeederQueue::addCollection( const Akonadi::Collection &collection )
{
  //kDebug() << collection.id();

  // If the collection contains mail, append it, otherwise prepend.
  // This ensures the smaller, fewer collections with things like
  // contacts or events in them are processed first. They tend to
  // be more important than the full text index of mail.
  // Bit of a hack, yes, would probably be better to have priorties
  // in the plugins and then keep a priority queue properly.
  if(mCollectionQueue.contains(collection)) {
    return;
  }
  if ( collection.contentMimeTypes().contains( QLatin1String( "message/rfc822" ) ) )
    mCollectionQueue.append( collection );
  else
    mCollectionQueue.prepend( collection );

  if ( mPendingJobs == 0 ) {
    processNextCollection();
  }
}

void FeederQueue::processNextCollection()
{
  //kDebug();
  if ( mCurrentCollection.isValid() )
    return;
  if ( mCollectionQueue.isEmpty() ) {
    indexingComplete();
    return;
  }
  mCurrentCollection = mCollectionQueue.takeFirst();
  emit running( i18n( "Indexing collection '%1'...", mCurrentCollection.name() ) );
  kDebug() << "Indexing collection " << mCurrentCollection.name() << mCurrentCollection.id();

  KJob *job = NepomukHelpers::addCollectionToNepomuk( mCurrentCollection );
  connect( job, SIGNAL(result(KJob*)), this, SLOT(jobResult(KJob*)));

  ItemFetchJob *itemFetch = new ItemFetchJob( mCurrentCollection, this );
  itemFetch->fetchScope().setCacheOnly( true );
  itemFetch->fetchScope().setIgnoreRetrievalErrors( true );
  connect( itemFetch, SIGNAL(finished(KJob*)), SLOT(itemFetchResult(KJob*)) );
  ++mPendingJobs;
}

void FeederQueue::itemHeadersReceived( const Akonadi::Item::List& items )
{
  kDebug() << items.count();
  Akonadi::Item::List itemsToUpdate;
  foreach ( const Item &item, items ) {
    if ( item.storageCollectionId() != mCurrentCollection.id() )
      continue; // stay away from links

    // update item if it does not exist or does not have a proper id
    if ( mReIndex || !NepomukHelpers::isIndexed(item)) {
      itemsToUpdate.append( item );
    }
  }

  if ( !itemsToUpdate.isEmpty() ) {
    lowPrioQueue.addItems( itemsToUpdate );
    mTotalAmount += itemsToUpdate.size();
    mProcessItemQueueTimer.start();
  }
}

void FeederQueue::itemFetchResult(KJob* job)
{
  if ( job->error() )
    kWarning() << job->errorString();

  Akonadi::ItemFetchJob *fetchJob = qobject_cast<Akonadi::ItemFetchJob *>( job );
  Q_ASSERT( fetchJob );
  itemHeadersReceived( fetchJob->items() );

  --mPendingJobs;
  if ( mPendingJobs == 0 && lowPrioQueue.isEmpty() ) { //Fetch jobs finished but there were no items in the collection
    collectionFullyIndexed();
    return;
  }
}

static inline QString emailMimetype()
{
  return QLatin1String("message/rfc822");
}

void FeederQueue::addItem( const Akonadi::Item &item )
{
  kDebug() << item.id();
  highPrioQueue.addItem( item );
  mTotalAmount++;
  mProcessItemQueueTimer.start();
}

void FeederQueue::addLowPrioItem( const Akonadi::Item &item )
{
  kDebug() << item.id();
  if (item.mimeType() == emailMimetype()) {
    emailItemQueue.addItem( item );
  } else {
    lowPrioQueue.addItem( item );
  }
  mTotalAmount++;
  mProcessItemQueueTimer.start();
}

bool FeederQueue::isEmpty()
{
  return allQueuesEmpty() && mCollectionQueue.isEmpty();
}

void FeederQueue::continueIndexing()
{
  kDebug();
  mProcessItemQueueTimer.start();
}

void FeederQueue::collectionFullyIndexed()
{
    NepomukHelpers::markCollectionAsIndexed( mCurrentCollection );
    const QString summary = i18n( "Indexing collection '%1' completed.", mCurrentCollection.name() );
    mCurrentCollection = Collection();
    const QPixmap pixmap = KIcon( "nepomuk" ).pixmap( KIconLoader::SizeSmall, KIconLoader::SizeSmall );
    KNotification::event( QLatin1String("indexingcollectioncompleted"),
                            summary,
                            pixmap,
                            0,
                            KNotification::CloseOnTimeout,
                            KGlobal::mainComponent());


    //kDebug() << "indexing of collection " << mCurrentCollection.id() << " completed";
    processNextCollection();
}

void FeederQueue::indexingComplete()
{
  //kDebug() << "fully indexed";
  mReIndex = false;
  emit progress( 100 );
  emit idle( i18n( "Indexing completed." ) );
  emit fullyIndexed();
}

void FeederQueue::processItemQueue()
{
  if ( mTotalAmount ) {
    int percent = ( mTotalAmount - size() ) * 100.0 / mTotalAmount;
    kDebug() << "Progress:" << percent << "%" << size() << mTotalAmount;
    emit progress( percent );
  }

  if ( !mOnline ) {
    kDebug() << "not Online, stopping processing";
    return;
  }

  if ( !highPrioQueue.isEmpty() ) {
    kDebug() << "high";
    if ( highPrioQueue.processBatch() ) {
        emit running( i18n( "Indexing" ) );
        return;
    }
  }
  else if ( !lowPrioQueue.isEmpty() ) {
    kDebug() << "low";
    if ( lowPrioQueue.processBatch() ) {
        emit running( i18n( "Indexing" ) );
        return;
    }
  }
  else if ( !emailItemQueue.isEmpty() ) {
    kDebug() << "email";
    if ( emailItemQueue.processBatch() ) {
        emit running( i18n( "Indexing Email" ) );
        return;
    }
  }

  kDebug() << "idle";
  emit idle( i18n( "Ready to index data." ) );
}

void FeederQueue::prioQueueFinished()
{
  if ( allQueuesEmpty() && ( mPendingJobs == 0 )) {
    if (mCurrentCollection.isValid()) {
      collectionFullyIndexed();
    } else {
      indexingComplete();
    }
    mTotalAmount = 0;
  }
}

bool FeederQueue::allQueuesEmpty() const
{
  if ( highPrioQueue.isEmpty() && lowPrioQueue.isEmpty() && emailItemQueue.isEmpty() ) {
    return true;
  }
  return false;
}


void FeederQueue::batchFinished()
{
  /*if ( sender() == &highPrioQueue )
    kDebug() << "high prio batch finished--------------------";
  if ( sender() == &lowPrioQueue )
    kDebug() << "low prio batch finished--------------------";*/
  if ( !allQueuesEmpty() ) {
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

Akonadi::Collection::List FeederQueue::listOfCollection() const
{
  return mCollectionQueue;
}

int FeederQueue::size()
{
  return lowPrioQueue.size() + highPrioQueue.size() + emailItemQueue.size();
}


#include "feederqueue.moc"
