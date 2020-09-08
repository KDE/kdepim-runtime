/*
 * SPDX-FileCopyrightText: 2020 Shashwat Jolly <shashwat.jolly@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef BASEHANDLER_H
#define BASEHANDLER_H

#include <AkonadiCore/Collection>
#include <AkonadiCore/Item>
#include <KCalendarCore/Incidence>

#include "etesyncadapter.h"
#include "etesyncclientstate.h"

using namespace Akonadi;

class EteSyncResource;

class BaseHandler : public QObject
{
    Q_OBJECT
public:
    typedef std::unique_ptr<BaseHandler> Ptr;
    explicit BaseHandler(EteSyncResource *resource);

    virtual const QString mimeType() = 0;

    virtual void setupItems(std::vector<EteSyncEntryPtr> &entries, Akonadi::Collection &collection, QString &prevUid);

    virtual void itemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection) = 0;
    virtual void itemChanged(const Akonadi::Item &item, const QSet<QByteArray> &parts) = 0;
    virtual void itemRemoved(const Akonadi::Item &item) = 0;

    virtual void collectionAdded(const Akonadi::Collection &collection, const Akonadi::Collection &parent) = 0;
    virtual void collectionChanged(const Akonadi::Collection &collection) = 0;
    virtual void collectionRemoved(const Akonadi::Collection &collection) = 0;

protected Q_SLOTS:

    void slotItemsRetrieved(KJob *job);
    virtual void syncCollection(const QVariant &collectionVariant);
    void taskDone();

protected:
    void initialiseBaseDirectory();
    bool createEteSyncEntry(const EteSyncSyncEntry *syncEntry, const EteSyncCryptoManager *cryptoManager, const Collection &collection);
    void updateCollectionRevision(const EteSyncEntry *entry, const Collection &collection);

    virtual QString baseDirectoryPath() const = 0;
    virtual const QString etesyncCollectionType() = 0;
    virtual void getItemListFromEntries(std::vector<EteSyncEntryPtr> &entries, Item::List &changedItems, Item::List &removedItems, Collection &collection, const QString &journalUid, QString &prevUid) = 0;

    bool handleConflictError(const Collection &collection);

    EteSyncResource *mResource = nullptr;
    EteSyncClientState *mClientState = nullptr;
};

#endif
