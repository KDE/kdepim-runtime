/*
 * SPDX-FileCopyrightText: 2020 Shashwat Jolly <shashwat.jolly@gmail.com>
 * 
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef ETESYNCENTRIESFETCHJOB_H
#define ETESYNCENTRIESFETCHJOB_H

#include <AkonadiCore/Collection>
#include <KJob>

#include "etesyncadapter.h"

namespace EteSyncAPI {
    class EntriesFetchJob : public KJob
    {
        Q_OBJECT

    public:
        explicit EntriesFetchJob(const EteSync *client, const Akonadi::Collection &collection, QObject *parent = nullptr);

        void start() override;

        std::vector<EteSyncEntryPtr> getEntries()
        {
            return std::move(mEntries);
        }

        QString getPrevUid() const
        {
            return mPrevUid;
        }

        Akonadi::Collection collection() const
        {
            return mCollection;
        }

    protected:
        enum Status
        {
            FETCH_OK,
            ERROR,
            ALL_ENTRIES_FETCHED
        };
        void fetchEntries();
        Status fetchNextBatch();

    private:
        const EteSync *mClient = nullptr;
        QString mPrevUid;
        QString mLastUid;
        EteSyncEntryManagerPtr mEntryManager;
        std::vector<EteSyncEntryPtr> mEntries;
        Akonadi::Collection mCollection;
    };
}  // namespace EteSyncAPI

#endif
