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
    QString journalUid = mCollection.remoteId();
    QString prevUid = mCollection.remoteRevision();
    EteSyncEntryManagerPtr entryManager(etesync_entry_manager_new(mClient, journalUid));
    mEntries = etesync_entry_manager_list(entryManager.get(), prevUid, 0);

    if (!mEntries) {
        setError(UserDefinedError);
        CharPtr err(etesync_get_error_message());
        setErrorText(QStringFromCharPtr(err));
    }

    emitResult();
}
