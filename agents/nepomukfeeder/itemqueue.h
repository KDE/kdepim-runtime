/*
 * Copyright (C) 2012  Christian Mollekopf <mollekopf@kolabsys.com>
 * Copyright (C) 2013  Vishesh Handa <me@vhanda.in>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef ITEMQUEUE_H
#define ITEMQUEUE_H

#include <QObject>
#include <Akonadi/Item>
#include <QQueue>
#include <QString>
#include <QTime>
#include <nepomuk2/simpleresourcegraph.h>
#include "propertycache.h"

class KJob;

/** 
 * The ItemQueue contains a list of Aknoadi items to be indexed
 *
 * This class internally consists of 2 queues - one contains a list
 * of akonadi items to fetch and the other contain the fetched akonadi items.
 *
 * During each iteration it will fill up the fetched item queue if required
 * and send one batch for indexing.
 */
class ItemQueue : public QObject {
  Q_OBJECT
public:
  /**
   * Create an ItemQueue
   *
   * \param batchSize describes the number of items that will be pushed
   *                  into Nepomuk in one go
   * \param fetchSize describes the number of items that will be fetched
   *                  from akonadi for pushing
   */
  explicit ItemQueue(int batchSize, int fetchSize, QObject* parent = 0);
  ~ItemQueue();

  /** add item to queue */
  void addItem(const Akonadi::Item &);
  void addItems(const Akonadi::Item::List &);

  /**
   * Index one batch of items.
   *
   * This sends one batch of items from its internal list to Nepomuk for indexing.
   *
   * If it does not contain the required number of item, it will fetch them
   * one fetchSize at a time. This entire operation is asynchronous and this
   * function returns almost instantly.
   *
   * \return \c true if succeeded in sending one batch for processing
   * \return \c false if the previous operation is still executing or there is
   *                  nothing to process
   * \sa batchFinished
   */
  bool processBatch();

  /** queue is empty */
  bool isEmpty() const;

  /**
   * Returns the total number of Akonadi Items in the queue
   */
  int size() const;

  /**
   * Sets the articifical delay that is introduced when processing one batch
   */
  void setProcessingDelay(int delay);

signals:
  /** all items processed */
  void finished();
  /** current batch has been processed and new Items can be added */
  void batchFinished();

private slots:
  void batchJobResult( KJob* job );
  void fetchJobResult( KJob* job );
  void removeDataResult( KJob* job );
  void slotEmitFinished();

private:
  /**
   * Index one batch of items
   *
   * \return \c false if nothing left to process
   * \return \c true sent one batch for processing
   */
  bool indexBatch();

  void addToQueue(Akonadi::Entity::Id);

  QQueue<Akonadi::Item::Id> mItemPipeline;
  Akonadi::Item::List mFetchedItemList;

  int mBatchSize; //Size of Nepomuk batch, number of items stored together in nepomuk
  int mFetchSize; //Maximum number of items fetched with full payload (defines ram usage of feeder), must be >= mBatchSize, ideally a multiple of it
  int mRunningJobs;
  int mDelay;

  PropertyCache mPropertyCache;

  QTime mTimer;
  double mAverageIndexingTime;
  qint64 mNumberOfIndexedItems;
};
#endif // ITEMQUEUE_H
