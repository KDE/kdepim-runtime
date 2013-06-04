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


#ifndef INDEXSCHEDULER_H
#define INDEXSCHEDULER_H

#include <QObject>
#include <Akonadi/Item>
#include <Akonadi/Collection>
#include <QTimer>
#include "itemqueue.h"
#include "eventmonitor.h"

class FeederPluginloader;
class KJob;

/**
 * The scheduler takes collections and items and indexes them
 * 
 * There is basically: 
 *  -CollectionQueue: list of collections to index (initial indexing), items end up in the ItemQueue
 *  -ItemQueue: Queue for the actual indexing
 * 
 * 
 * The queue has two ItemQueues for indexing the items:
 *  -highPrioQueue for items with high prioritiy for indexing (a note which just changed)
 *  -lowPrioQueue for items with low priority (i.e. initial indexing of emails)
 * 
 * The idea is that items which have been changed by the user have higher priority than
 * the initial indexing of i.e. emails.
 * 
 * EntryPoints are:
 *  -addCollection: for initial indexing (low priority)
 *  -addItem: to index a single item (high priority)
 *
 */
class IndexScheduler: public QObject
{
  Q_OBJECT
public:
  explicit IndexScheduler( QObject* parent = 0 );
  virtual ~IndexScheduler();

  ///add the collection to the queue, all items of it will be fetched and indexed
  void addCollection(const Akonadi::Collection &);
  ///adds the item to the highPrioQueue or emailQueue
  void addItem(const Akonadi::Item &);
  void addLowPrioItem(const Akonadi::Item &);

  /**
   * Remove the collection \p collection from the Nepomuk index
   */
  void removeCollection(const Akonadi::Collection& collection);

  /**
   * Remove the item \p item from the Nepomuk index.
   */
  void removeItem(const Akonadi::Item& item);

  void removeNepomukUris(const QList<QUrl> uriList);

  /**
   * If enabled all items will be reindexed
   * The flag will be reset once all collections/items have been indexed
   */
  void setReindexing(bool);

  bool isEmpty();

  /** start/stop indexing */
  void setOnline(bool);

  int size();

  enum IndexingSpeed {
      /**
       * Index at full speed, i.e. do not use any artificial
       * delays.
       *
       * This is the mode used if the user is "away".
       */
      FullSpeed = 0,

      /**
       * Reduce the indexing speed mildly. This is the normal
       * mode used while the user works. The indexer uses small
       * delay between indexing two batches in order to keep the
       * load on CPU and IO down.
       */
      ReducedSpeed
  };

  void setIndexingSpeed( IndexingSpeed speed );

  /**
   * Clears all the Items in the internal queues which
   * are supposed to be indexed
   */
  void clear();

signals:
  void progress(int);
  void idle(QString);
  void running(QString);

private slots:
  void processNextCollection();
  void itemFetchResult( KJob* job );
  void processItemQueue();
  void prioQueueFinished();
  void batchFinished();
  void jobResult( KJob* job );

  void slotIdleStatusChanged(bool isIdle);
  void slotPowerManagementChanged(bool onBattery);

private:
  void itemHeadersReceived( const Akonadi::Item::List &items );
  void continueIndexing(); //start the indexing if work is to be done
  void collectionFullyIndexed();
  void indexingComplete();
  int mTotalAmount, mPendingJobs;

  Akonadi::Collection::List mCollectionQueue;
  Akonadi::Collection mCurrentCollection;
  bool mReIndex;
  bool mOnline;
  QTimer mProcessItemQueueTimer;

  ItemQueue lowPrioQueue;
  ItemQueue highPrioQueue;
  ItemQueue emailItemQueue;

  // To Clear
  Akonadi::Item::List mItemsToRemove;
  Akonadi::Collection::List mCollectionsToRemove;
  QList<QUrl> mUrisToRemove;

  Nepomuk2::EventMonitor* mEventMonitor;
};




#endif // INDEXSCHEDULER_H
