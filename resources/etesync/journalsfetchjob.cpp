/*
 * SPDX-FileCopyrightText: 2020 Shashwat Jolly <shashwat.jolly@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "journalsfetchjob.h"

#include <QtConcurrent>

#include "etesync_debug.h"

using namespace EteSyncAPI;

JournalsFetchJob::JournalsFetchJob(EteSync *client, QObject *parent)
    : KJob(parent)
    , mClient(client)
{
}

void JournalsFetchJob::start()
{
    QFutureWatcher<void> *watcher = new QFutureWatcher<void>(this);
    connect(watcher, &QFutureWatcher<void>::finished, this, [this] {
        qCDebug(ETESYNC_LOG) << "emitResult from JournalsFetchJob";
        emitResult();
    });
    QFuture<void> fetchJournalsFuture = QtConcurrent::run(this, &JournalsFetchJob::fetchJournals);
    watcher->setFuture(fetchJournalsFuture);
}

void JournalsFetchJob::fetchJournals()
{
    EteSyncJournalManager *journalManager = etesync_journal_manager_new(mClient);
    mJournals = etesync_journal_manager_list(journalManager);
    if (!mJournals) {
        setError(int(etesync_get_error_code()));
        CharPtr err(etesync_get_error_message());
        setErrorText(QStringFromCharPtr(err));
    }
    qCDebug(ETESYNC_LOG) << "Journals fetched";
}
