/*
 * Copyright (C) 2020 by Shashwat Jolly <shashwat.jolly@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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

        QString getPrevUid()
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
