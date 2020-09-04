/*
 * SPDX-FileCopyrightText: 2020 Shashwat Jolly <shashwat.jolly@gmail.com>
 * 
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "journalsfetchjob.h"

#include <QtConcurrent>

using namespace EteSyncAPI;

JournalsFetchJob::JournalsFetchJob(EteSync *client, QObject *parent)
    : KJob(parent), mClient(client)
{
}

void JournalsFetchJob::start()
{
    QtConcurrent::run(this, &JournalsFetchJob::fetchJournals);
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
    emitResult();
}
