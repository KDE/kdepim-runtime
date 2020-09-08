/*
 * SPDX-FileCopyrightText: 2020 Shashwat Jolly <shashwat.jolly@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "entriesfetchjob.h"

#include <QtConcurrent>

#include "etesync_debug.h"
#include "settings.h"

using namespace EteSyncAPI;

EntriesFetchJob::EntriesFetchJob(const EteSync *client, const Akonadi::Collection &collection, QObject *parent)
    : KJob(parent)
    , mClient(client)
    , mCollection(collection)
{
}

void EntriesFetchJob::start()
{
    QFutureWatcher<void> *watcher = new QFutureWatcher<void>(this);
    connect(watcher, &QFutureWatcher<int>::finished, this, [this] {
        qCDebug(ETESYNC_LOG) << "emitResult from EntriesFetchJob";
        emitResult();
    });
    QFuture<void> fetchEntriesFuture = QtConcurrent::run(this, &EntriesFetchJob::fetchEntries);
    watcher->setFuture(fetchEntriesFuture);
}

void EntriesFetchJob::fetchEntries()
{
    const QString journalUid = mCollection.remoteId();
    mPrevUid = mLastUid = mCollection.remoteRevision();
    mEntryManager = etesync_entry_manager_new(mClient, journalUid);

    EntriesFetchJob::Status status = FETCH_OK;
    while (status != ERROR && status != ALL_ENTRIES_FETCHED) {
        status = fetchNextBatch();
    }

    if (status == ERROR) {
        qCDebug(ETESYNC_LOG) << "Returning error from entries fetch job";
        setError(etesync_get_error_code());
        CharPtr err(etesync_get_error_message());
        setErrorText(QStringFromCharPtr(err));
    }
    qCDebug(ETESYNC_LOG) << "Entries fetched";
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
