/*
 * SPDX-FileCopyrightText: 2020 Shashwat Jolly <shashwat.jolly@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef ETESYNCENTRIESFETCHJOB_H
#define ETESYNCENTRIESFETCHJOB_H

#include <AkonadiCore/Collection>
#include <AkonadiCore/Item>
#include <KJob>

#include "etebaseadapter.h"
#include "etesyncclientstate.h"

namespace EteSyncAPI
{
class EntriesFetchJob : public KJob
{
    Q_OBJECT

public:
    explicit EntriesFetchJob(const EteSyncClientState *mClientState,
                             const Akonadi::Collection &collection,
                             EtebaseCollectionPtr etesyncCollection,
                             QObject *parent = nullptr);

    void start() override;

    Akonadi::Item::List items() const
    {
        return mItems;
    }

    Akonadi::Item::List removedItems() const
    {
        return mRemovedItems;
    }

    Akonadi::Collection collection() const
    {
        return mCollection;
    }

protected:
    void fetchEntries();
    void setupItem(const EtebaseItem *etesyncItem, const QString &type);

private:
    const EteSyncClientState *mClientState = nullptr;
    Akonadi::Collection mCollection;
    const EtebaseCollectionPtr mEtesyncCollection;
    Akonadi::Item::List mItems;
    Akonadi::Item::List mRemovedItems;
};
} // namespace EteSyncAPI

#endif
