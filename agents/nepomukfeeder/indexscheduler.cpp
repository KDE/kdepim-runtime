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
#include <QTimer>

#include "nepomukhelpers.h"
#include "nepomukfeeder-config.h"

#include <Nepomuk2/DataManagement>

using namespace Akonadi;

IndexScheduler::IndexScheduler( QObject* parent )
: QObject( parent ),
  mTotalAmount( 0 ),
  mTotalClearAmount( 0 ),
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
}

static inline QString emailMimetype()
{
  return QLatin1String("message/rfc822");
}

void IndexScheduler::addItem( const Akonadi::Item &item )
{
  kDebug() << item.id();
  if (item.mimeType() == emailMimetype()) {
    emailItemQueue.addItem( item );
  }
  else {
    highPrioQueue.addItem( item );
  }
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
    mTotalClearAmount++;
}

void IndexScheduler::removeItem(const Item& item)
{
    mItemsToRemove.append( item );
    mTotalClearAmount++;
}

void IndexScheduler::removeNepomukUris(const QList< QUrl > uriList)
{
    mUrisToRemove.append( uriList );
    mTotalClearAmount += uriList.size();
}


bool IndexScheduler::isEmpty()
{
  return highPrioQueue.isEmpty() && lowPrioQueue.isEmpty() &&
         emailItemQueue.isEmpty() && mCollectionQueue.isEmpty() &&
         mCollectionsToRemove.isEmpty() &&
         mItemsToRemove.isEmpty() && mUrisToRemove.isEmpty();
}

void IndexScheduler::continueIndexing()
{
  kDebug();
  mProcessItemQueueTimer.start();
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
  else if (mTotalClearAmount) {
    int size = mCollectionsToRemove.size() + mItemsToRemove.size() + mUrisToRemove.size();
    int percent = ( mTotalClearAmount - size ) * 100.0 / mTotalClearAmount;
    kDebug() << "Progress:" << percent << "%" << size << mTotalClearAmount;
    emit progress( percent );
  }

  if ( !mOnline ) {
    kDebug() << "not Online, stopping processing";
    return;
  }

  if ( !mCollectionQueue.isEmpty() ) {
    Akonadi::Collection collection = mCollectionQueue.takeFirst();

    KJob *job = NepomukHelpers::addCollectionToNepomuk( collection );
    connect( job, SIGNAL(result(KJob*)), this, SLOT(collectionSaveJobResult(KJob*)));

    emit running( i18n( "Indexing collection '%1'", collection.name() ) );
    return;
  }
  else if ( !highPrioQueue.isEmpty() ) {
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
      for( int i=0; i<10 && mCollectionsToRemove.count(); i++ )
          mNieUrlsToRemove << mCollectionsToRemove.takeFirst().url();

      QTimer::singleShot(lowPrioQueue.processingDelay(), this, SLOT(slotSendUrlsForClearing()));
      emit running( i18n("Clearing unused Emails") );
      return;
  }

  else if( !mItemsToRemove.isEmpty() ) {
      // Clear 10 items at a time
      for( int i=0; i<10 && mItemsToRemove.count(); i++ )
          mNieUrlsToRemove << mItemsToRemove.takeFirst().url();

      QTimer::singleShot(lowPrioQueue.processingDelay(), this, SLOT(slotSendUrlsForClearing()));
      emit running( i18n("Clearing unused Emails") );
      return;
  }

  else if( !mUrisToRemove.isEmpty() ) {
      // Clear 10 at a time
      for( int i=0; i<10 && mUrisToRemove.count(); i++ )
            mNieUrlsToRemove << mUrisToRemove.takeFirst();

      QTimer::singleShot(lowPrioQueue.processingDelay(), this, SLOT(slotSendUrlsForClearing()));
      emit running( i18n("Clearing unused Emails") );
      return;
  }

  kDebug() << "idle";
  emit idle( i18n( "Ready to index data." ) );
}

void IndexScheduler::slotSendUrlsForClearing()
{
    KJob *removeJob = Nepomuk2::removeResources( mNieUrlsToRemove, Nepomuk2::RemoveSubResoures );
    connect( removeJob, SIGNAL(finished(KJob*)), this, SLOT(batchFinished()) );

    mNieUrlsToRemove.clear();
}

void IndexScheduler::prioQueueFinished()
{
    if (highPrioQueue.isEmpty() && lowPrioQueue.isEmpty() && emailItemQueue.isEmpty() && mCollectionQueue.isEmpty()) {
        indexingComplete();
        mTotalAmount = 0;
    }
}

void IndexScheduler::batchFinished()
{
    if (!isEmpty()) {
        // go to eventloop before processing the next one, otherwise we miss the idle status change
        mProcessItemQueueTimer.start();
    }
    else if (mCollectionsToRemove.isEmpty() && mItemsToRemove.isEmpty() && mUrisToRemove.isEmpty()) {
        mTotalClearAmount = 0;
        indexingComplete();
    }
}

void IndexScheduler::collectionSaveJobResult(KJob* job)
{
  if ( job->error() )
    kWarning() << job->errorString();

  batchFinished();
}

int IndexScheduler::size()
{
  return lowPrioQueue.size() + highPrioQueue.size() + emailItemQueue.size();
}

void IndexScheduler::clear()
{
  mCollectionQueue.clear();

  lowPrioQueue.clear();
  highPrioQueue.clear();
  emailItemQueue.clear();

  mItemsToRemove.clear();
  mCollectionsToRemove.clear();
  mUrisToRemove.clear();
}


#include "indexscheduler.moc"
