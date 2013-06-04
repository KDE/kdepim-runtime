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


#include "indexscheduler.h"

#include <nie.h>

#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemFetchScope>
#include <Akonadi/Collection>

#include <KLocalizedString>
#include <KUrl>
#include <KJob>
#include <KIconLoader>

#include <QDateTime>

#include "nepomukhelpers.h"
#include "nepomukfeeder-config.h"

#include <Nepomuk2/DataManagement>

using namespace Akonadi;

IndexScheduler::IndexScheduler( QObject* parent )
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

  mEventMonitor = new Nepomuk2::EventMonitor( this );
  connect( mEventMonitor, SIGNAL(idleStatusChanged(bool)), this, SLOT(slotIdleStatusChanged(bool)) );
  connect( mEventMonitor, SIGNAL(powerManagementStatusChanged(bool)), this, SLOT(slotPowerManagementChanged(bool)) );
}

IndexScheduler::~IndexScheduler()
{

}

void IndexScheduler::setReindexing( bool reindex )
{
  // FIXME: This doesn't seem like it will do anything?
  //        Shouldn't it call some kind of signal?
  mReIndex = reindex;
}

void IndexScheduler::setOnline( bool online )
{
  //kDebug() << online;
  if ( online && !mEventMonitor->isOnBattery() ) {
      mOnline = online;
      slotIdleStatusChanged( mEventMonitor->isIdle() );
      continueIndexing();
  }
  else {
      mOnline = false;
  }
}

void IndexScheduler::slotIdleStatusChanged(bool isIdle)
{
    if ( mOnline ) {
        setIndexingSpeed( isIdle ? FullSpeed : ReducedSpeed );
    }
}

void IndexScheduler::slotPowerManagementChanged(bool onBattery)
{
    setOnline( !onBattery );
    // FIXME: Need some kind of better status message!
}


void IndexScheduler::setIndexingSpeed(IndexScheduler::IndexingSpeed speed)
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

void IndexScheduler::addCollection( const Akonadi::Collection &collection )
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

void IndexScheduler::processNextCollection()
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

void IndexScheduler::itemHeadersReceived( const Akonadi::Item::List& items )
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

void IndexScheduler::itemFetchResult(KJob* job)
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

void IndexScheduler::addItem( const Akonadi::Item &item )
{
  kDebug() << item.id();
  highPrioQueue.addItem( item );
  mTotalAmount++;
  mProcessItemQueueTimer.start();
}

void IndexScheduler::addLowPrioItem( const Akonadi::Item &item )
{
//  kDebug() << item.id();
  if (item.mimeType() == emailMimetype()) {
    emailItemQueue.addItem( item );
  } else {
    lowPrioQueue.addItem( item );
  }
  mTotalAmount++;
  mProcessItemQueueTimer.start();
}

void IndexScheduler::removeCollection(const Collection& collection)
{
    mCollectionsToRemove.append( collection );
}

void IndexScheduler::removeItem(const Item& item)
{
    mItemsToRemove.append( item );
}

void IndexScheduler::removeNepomukUris(const QList< QUrl > uriList)
{
    mUrisToRemove.append( uriList );
}


bool IndexScheduler::isEmpty()
{
  return highPrioQueue.isEmpty() && lowPrioQueue.isEmpty() &&
         emailItemQueue.isEmpty() && mCollectionQueue.isEmpty() &&
         mCollectionsToRemove.isEmpty() &&
         mItemsToRemove.isEmpty();
}

void IndexScheduler::continueIndexing()
{
  kDebug();
  mProcessItemQueueTimer.start();
}

void IndexScheduler::collectionFullyIndexed()
{
    NepomukHelpers::markCollectionAsIndexed( mCurrentCollection );
    mCurrentCollection = Collection();

    //kDebug() << "indexing of collection " << mCurrentCollection.id() << " completed";
    processNextCollection();
}

void IndexScheduler::indexingComplete()
{
  //kDebug() << "fully indexed";
  mReIndex = false;
  emit progress( 100 );
  emit idle( i18n( "Indexing completed." ) );
}

void IndexScheduler::processItemQueue()
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

  // Cleaning
  else if( !mCollectionsToRemove.isEmpty() ) {
      kDebug() << "Clear collection";
      // FIXME: We should probably be clearing the contents of the collection as well

      // Clear 10 collections at a time
      QList<QUrl> nieUrls;
      for( int i=0; i<10 && mCollectionsToRemove.count(); i++ )
          nieUrls << mCollectionsToRemove.takeFirst().url();

      KJob *removeJob = Nepomuk2::removeResources( nieUrls, Nepomuk2::RemoveSubResoures );
      connect( removeJob, SIGNAL(finished(KJob*)), this, SLOT(batchFinished()) );

      emit running( i18n("Clearing unused Emails") );
      return;
  }

  else if( !mItemsToRemove.isEmpty() ) {
      // Clear 10 items at a time
      QList<QUrl> nieUrls;
      for( int i=0; i<10 && mItemsToRemove.count(); i++ )
          nieUrls << mItemsToRemove.takeFirst().url();

      KJob *removeJob = Nepomuk2::removeResources( nieUrls, Nepomuk2::RemoveSubResoures );
      connect( removeJob, SIGNAL(finished(KJob*)), this, SLOT(batchFinished()) );

      emit running( i18n("Clearing unused Emails") );
      return;
  }

  else if( !mUrisToRemove.isEmpty() ) {
      // Clear 10 at a time
      QList<QUrl> uris;
      for( int i=0; i<10 && mUrisToRemove.count(); i++ )
            uris << mUrisToRemove.takeFirst();

      KJob *removeJob = Nepomuk2::removeResources( uris, Nepomuk2::RemoveSubResoures );
      connect( removeJob, SIGNAL(finished(KJob*)), this, SLOT(batchFinished()) );

      emit running( i18n("Clearing unused Emails") );
      return;
  }

  kDebug() << "idle";
  emit idle( i18n( "Ready to index data." ) );
}

void IndexScheduler::prioQueueFinished()
{
  if ( isEmpty() && ( mPendingJobs == 0 )) {
    if (mCurrentCollection.isValid()) {
      collectionFullyIndexed();
    } else {
      indexingComplete();
    }
    mTotalAmount = 0;
  }
}

void IndexScheduler::batchFinished()
{
  /*if ( sender() == &highPrioQueue )
    kDebug() << "high prio batch finished--------------------";
  if ( sender() == &lowPrioQueue )
    kDebug() << "low prio batch finished--------------------";*/
  if ( !isEmpty() ) {
    //kDebug() << "continue";
    // go to eventloop before processing the next one, otherwise we miss the idle status change
    mProcessItemQueueTimer.start();
  }
}

void IndexScheduler::jobResult(KJob* job)
{
  if ( job->error() )
    kWarning() << job->errorString();
}

int IndexScheduler::size()
{
  return lowPrioQueue.size() + highPrioQueue.size() + emailItemQueue.size();
}

void IndexScheduler::clear()
{
  mCollectionQueue.clear();
  mCurrentCollection = Collection();

  lowPrioQueue.clear();
  highPrioQueue.clear();
  emailItemQueue.clear();

  mItemsToRemove.clear();
  mCollectionsToRemove.clear();
  mUrisToRemove.clear();
}


#include "indexscheduler.moc"
