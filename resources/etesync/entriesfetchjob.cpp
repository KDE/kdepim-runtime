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

    while (fetchNextBatch()) {
    }

    if (etesync_get_error_code() != EteSyncErrorCode::ETESYNC_ERROR_CODE_NO_ERROR) {
        setError(UserDefinedError);
        CharPtr err(etesync_get_error_message());
        setErrorText(QStringFromCharPtr(err));
        emitResult();
        return;
    }
    emitResult();
}

bool EntriesFetchJob::fetchNextBatch()
{
    std::vector<EteSyncEntryPtr> entries = etesync_entry_manager_list(mEntryManager.get(), mLastUid, 50);
    if (entries.empty()) {
        return false;
    }
    mEntries.insert(mEntries.end(), std::make_move_iterator(entries.begin()), std::make_move_iterator(entries.end()));
    mLastUid = QStringFromCharPtr(CharPtr(etesync_entry_get_uid(mEntries[mEntries.size() - 1].get())));
    return true;
}
