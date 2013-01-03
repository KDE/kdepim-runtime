/*
 * Copyright (C) 2012  Christian Mollekopf <mollekopf@kolabsys.com>
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
 * adds items to the internal queue, until the limit is passed, then the items are stored
 */
class ItemQueue : public QObject {
  Q_OBJECT
public:
    explicit ItemQueue(int batchSize, int fetchSize, QObject* parent = 0);
    ~ItemQueue();

  /** add item to queue */
  void addItem(const Akonadi::Item &);
  void addItems(const Akonadi::Item::List &);
  /** process one item @return returns false if currently blocked */
  bool processItem();
  /** queue is empty */
  bool isEmpty() const;

  /** the delay between two batches */
  void setProcessingDelay(int ms);

  void setSaveFile(const QString &saveFile);

  /** Enable optimizations if we know all items of this queue are not yet indexed (all checks are skipped) */
  void setItemsAreNotIndexed(bool);

signals:
  /** all items processed */
  void finished();
  /** current batch has been processed and new Items can be added */
  void batchFinished();

private slots:
  void batchJobResult( KJob* job );
  void fetchJobResult( KJob* job );
  void continueProcessing();

private:
  bool processBatch();
  void saveState();
  void loadState();
  void addToQueue(Akonadi::Entity::Id);

  QString mSaveFile;
  QQueue<Akonadi::Item::Id> mItemPipelineBackup;
  QQueue<Akonadi::Item::Id> mItemPipeline;
  Nepomuk2::SimpleResourceGraph mResourceGraph;
  QList<Akonadi::Item::Id> mBatch;
  QList<Akonadi::Item::Id> mTempFetchList;
  Akonadi::Item::List mItemFetchList;
  Akonadi::Item::List mFetchedItemList;

  int mBatchSize; //Size of Nepomuk batch, number of items stored together in nepomuk
  int mFetchSize; //Maximum number of items fetched with full payload (defines ram usage of feeder), must be >= mBatchSize, ideally a multiple of it
  int mRunningJobs;

  int mProcessingDelay;
  PropertyCache mPropertyCache;
  bool mItemsAreNotIndexed;

  QTime mTimer;
  double mAverageIndexingTime;
  qint64 mNumberOfIndexedItems;
};
#endif // ITEMQUEUE_H
