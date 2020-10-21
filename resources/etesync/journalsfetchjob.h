/*
 * SPDX-FileCopyrightText: 2020 Shashwat Jolly <shashwat.jolly@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef ETESYNCJOURNALSFETCHJOB_H
#define ETESYNCJOURNALSFETCHJOB_H

#include <KJob>

#include "etebaseadapter.h"
#include "etesyncclientstate.h"

#include <AkonadiCore/Collection>

namespace EteSyncAPI {
class JournalsFetchJob : public KJob
{
    Q_OBJECT

public:
    explicit JournalsFetchJob(const EteSyncClientState *mClientState, const Akonadi::Collection &resourceCollection, QObject *parent = nullptr);

    void start() override;

    Akonadi::Collection::List collections() const
    {
        return mCollections;
    }

    Akonadi::Collection::List removedCollections() const
    {
        return mRemovedCollections;
    }

    QString syncToken() const
    {
        return mSyncToken;
    }

protected:
    void fetchJournals();
    void setupCollection(const EtebaseCollection *etesyncCollection);
private:
    const EteSyncClientState *mClientState = nullptr;
    Akonadi::Collection::List mCollections;
    Akonadi::Collection::List mRemovedCollections;
    const Akonadi::Collection mResourceCollection;
    QString mSyncToken;
};
} // namespace EteSyncAPI

#endif
