/*
    Copyright (c) 2006 - 2007 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_STRIGI_FEEDER_H
#define AKONADI_STRIGI_FEEDER_H

#include <akonadi/agentbase.h>
#include <akonadi/collection.h>
#include <akonadi/item.h>
#include <strigi/qtdbus/strigiclient.h>

class KJob;
namespace Akonadi_Strigifeeder_Agent {
class Settings;
}

namespace Akonadi {

class ItemFetchScope;

/**
  Full text search provider using strigi.
*/
class StrigiFeeder : public AgentBase, public AgentBase::ObserverV2
{
  Q_OBJECT

  public:
    StrigiFeeder( const QString &id );
    ~StrigiFeeder();

    void configure( WId windowId );

    /**
     * Set the index compatibility level. If the current level is below this, a full re-indexing is performed.
     */
    void setIndexCompatibilityLevel( int level );

  public Q_SLOTS:
    /** Trigger a complete update of all items. */
    void updateAll();

  Q_SIGNALS:
    void fullyIndexed();

  protected:
    void itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection );
    void itemChanged( const Akonadi::Item &item, const QSet<QByteArray> &partIdentifiers );
    void itemRemoved( const Akonadi::Item &item );
    void itemMoved( const Akonadi::Item &item, const Akonadi::Collection &collectionSource,
                    const Akonadi::Collection &collectionDestination );

    void doSetOnline( bool online );

  private Q_SLOTS:
    void collectionsReceived( const Akonadi::Collection::List &collections );
    void itemHeadersReceived( const Akonadi::Item::List &items );
    void itemsReceived( const Akonadi::Item::List &items );
    void notificationItemsReceived( const Akonadi::Item::List &items );
    void itemFetchResult( KJob *job );

    void selfTest();
    void slotFullyIndexed();
    void systemIdle();
    void systemResumed();

  private:
    void processNextCollection();
    void checkOnline();
    bool needsReIndexing() const;
    Akonadi::ItemFetchScope fetchScopeForCollection( const Akonadi::Collection &collection );

    void indexItem( const Item &item );

    StrigiClient mStrigi;

    Akonadi::Collection::List mCollectionQueue;
    Akonadi::Collection mCurrentCollection;
    int mTotalAmount, mProcessedAmount, mPendingJobs;
    QTimer mStrigiDaemonStartupTimeout;
    int mIndexCompatLevel;
    bool mStrigiDaemonStartupAttempted;
    bool mInitialUpdateDone;
    bool mSelfTestPassed;
    bool mSystemIsIdle;
    Akonadi_Strigifeeder_Agent::Settings *mSettings;
};

}

#endif
