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

#include "journalsfetchjob.h"

#include <kio/job.h>

#include <QTimer>

using namespace EteSyncAPI;

JournalsFetchJob::JournalsFetchJob(EteSync *client, QObject *parent)
    : KJob(parent), mClient(client)
{
}

void JournalsFetchJob::start()
{
    QTimer::singleShot(0, this, &JournalsFetchJob::fetchJournals);
}

void JournalsFetchJob::fetchJournals()
{
    EteSyncJournalManager *journalManager = etesync_journal_manager_new(mClient);
    mJournals = etesync_journal_manager_list(journalManager);
    if (!mJournals) {
        setError(UserDefinedError);
        setErrorText(QStringLiteral("JournalsFetchJob failed to fetch journals"));
    }
    emitResult();
}
