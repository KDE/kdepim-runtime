/*
    Copyright (c) 2007 Tobias Koenig <tokoe@kde.org>
                  2008 Sebastian Trueg <trueg@kde.org>
                  2009 Volker Krause <vkrause@kde.org>
                  2011 Christian Mollekopf <chrigi_1@fastmail.fm>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#ifndef NEPOMUKFEEDERAGENT_H
#define NEPOMUKFEEDERAGENT_H

#include <akonadi/agentbase.h>
#include <akonadi/collection.h>
#include <akonadi/item.h>

#include <QtCore/QTimer>
#include "feederqueue.h"
#include "indexerconfig.h"

class FeederPluginloader;
class FindUnindexedItemsJob;
class KJob;

namespace Akonadi
{
  class Item;
  class ItemFetchScope;
  class CollectionFetchJob;


/**
 * The main feeder class which listens for changes and sends them
 * to the scheduler.
 *
 * Reindexing:
 * Increasing the mIndexCompatLevel, issues a reindexing.
 */
class NepomukFeederAgent : public Akonadi::AgentBase, public Akonadi::AgentBase::ObserverV2
{
  Q_OBJECT

  public:
    explicit NepomukFeederAgent(const QString& id);
    ~NepomukFeederAgent();

    /**
     * Sets whether the 'Only feed when system is idle' functionality shall be used.
     */
    void disableIdleDetection( bool value );

    bool isDisableIdleDetection() const;

    void forceReindexCollection(const qlonglong id);

    void forceReindexItem(const qlonglong id);

    bool queueIsEmpty();

    QString currentCollectionName();

    QStringList listOfCollection() const;
    qlonglong totalitems() const;
    qlonglong indexeditems() const;
    bool isIndexing() const;
  public slots:
    /** Trigger a complete update of all items. */
    void updateAll();

  signals:
    void fullyIndexed();

  protected:
    void itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection );
    void itemChanged( const Akonadi::Item &item, const QSet<QByteArray> &partIdentifiers );
    void itemRemoved(const Akonadi::Item &item);

    void collectionAdded(const Akonadi::Collection& collection, const Akonadi::Collection& parent);
    void collectionChanged(const Akonadi::Collection& collection, const QSet< QByteArray >& partIdentifiers);
    using AgentBase::ObserverV2::collectionChanged;
    void collectionRemoved(const Akonadi::Collection& collection);

    void doSetOnline(bool online);

  private:
    void setRunning( bool running );
    void processNextNotification();
    void findUnindexed();

  private slots:
    void selfTest();
    void checkMigration();
    void slotFullyIndexed();
    void systemIdle();
    void systemResumed();
    void collectionsReceived( const Akonadi::Collection::List &collections );
    void collectionListReceived( KJob* );
    void configure( WId windowId );
    void foundUnindexedItems(KJob *job);

    void emitIdle(const QString&);
    void emitRunning(const QString&);
  private:
    bool mInitialUpdateDone;
    bool mIdleDetectionDisabled;
    bool mInitialIndexingDisabled;

    FeederQueue mQueue;

    qlonglong mTotalItems;
    qlonglong mIndexedItems;

    IndexerConfig* m_indexerConfig;

    Akonadi::CollectionFetchJob* m_collectionFetchJob;
    FindUnindexedItemsJob* m_findUnindexedItemsJob;
};

}

#endif
