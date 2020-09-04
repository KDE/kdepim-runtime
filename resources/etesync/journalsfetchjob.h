/*
 * SPDX-FileCopyrightText: 2020 Shashwat Jolly <shashwat.jolly@gmail.com>
 * 
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef ETESYNCJOURNALSFETCHJOB_H
#define ETESYNCJOURNALSFETCHJOB_H

#include <KJob>

#include "etesyncadapter.h"

namespace EteSyncAPI {
    class JournalsFetchJob : public KJob
    {
        Q_OBJECT

    public:
        explicit JournalsFetchJob(EteSync *client, QObject *parent = nullptr);

        void start() override;

        EteSyncJournal **journals() const
        {
            return mJournals;
        }

    protected:
        void fetchJournals();

    private:
        EteSync *mClient = nullptr;
        EteSyncJournal **mJournals = nullptr;
    };
}  // namespace EteSyncAPI

#endif
