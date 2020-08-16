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

#include "entriesfetchjob.h"

#include <kio/job.h>

#include <QTimer>

#include "etesync_debug.h"
#include "settings.h"

using namespace EteSyncAPI;

EntriesFetchJob::EntriesFetchJob(const EteSync *client, const Akonadi::Collection &collection, QObject *parent)
    : KJob(parent), mClient(client), mCollection(collection)
{
}

void EntriesFetchJob::start()
{
    QTimer::singleShot(0, this, &EntriesFetchJob::fetchEntries);
}

void EntriesFetchJob::fetchEntries()
{
    const QString journalUid = mCollection.remoteId();
    mPrevUid = mLastUid = mCollection.remoteRevision();
    mEntryManager = etesync_entry_manager_new(mClient, journalUid);

    EntriesFetchJob::Status status;
    do {
        status = fetchNextBatch();
    } while (status != ERROR && status != ALL_ENTRIES_FETCHED);

    if (status == ERROR) {
        setError(UserDefinedError);
        CharPtr err(etesync_get_error_message());
        setErrorText(QStringFromCharPtr(err));
    }
    emitResult();
}

EntriesFetchJob::Status EntriesFetchJob::fetchNextBatch()
{
    std::pair<std::vector<EteSyncEntryPtr>, bool> entries = etesync_entry_manager_list(mEntryManager.get(), mLastUid, 50);
    if (entries.second) {
        return ERROR;
    }
    if (entries.first.empty()) {
        return ALL_ENTRIES_FETCHED;
    }
    mEntries.insert(mEntries.end(), std::make_move_iterator(entries.first.begin()), std::make_move_iterator(entries.first.end()));
    mLastUid = QStringFromCharPtr(CharPtr(etesync_entry_get_uid(mEntries[mEntries.size() - 1].get())));
    return FETCH_OK;
}
