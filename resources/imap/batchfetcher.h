/*
 * SPDX-FileCopyrightText: 2014 Christian Mollekopf <mollekopf@kolabsys.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#ifndef BATCHFETCHER_H
#define BATCHFETCHER_H

#include <KJob>
#include <KIMAP/FetchJob>
#include <KIMAP/SearchJob>

#include "messagehelper.h"

/**
 * A job that retrieves a set of messages in reverse-ordered batches.
 * After each batch fetchNextBatch() needs to be called (for throttling the download speed)
 */
class BatchFetcher : public KJob
{
    Q_OBJECT
public:
    BatchFetcher(MessageHelper::Ptr messageHelper, const KIMAP::ImapSet &set, const KIMAP::FetchJob::FetchScope &scope, int batchSize, KIMAP::Session *session);
    ~BatchFetcher() override;
    void start() override;
    void fetchNextBatch();
    void setUidBased(bool);
    void setSearchUids(const KIMAP::ImapInterval &interval);
    void setGmailExtensionsEnabled(bool enable);

Q_SIGNALS:
    void itemsRetrieved(const Akonadi::Item::List &);

private Q_SLOTS:
    void onMessagesAvailable(const QMap<qint64, KIMAP::Message> &messages);
    void onHeadersFetchDone(KJob *job);
    void onUidSearchDone(KJob *job);

private:
    //Batch fetching
    KIMAP::ImapSet m_currentSet;
    KIMAP::FetchJob::FetchScope m_scope;
    KIMAP::Session *m_session = nullptr;
    int m_batchSize;
    bool m_uidBased;
    int m_fetchedItemsInCurrentBatch;
    const MessageHelper::Ptr m_messageHelper;
    bool m_fetchInProgress;
    bool m_continuationRequested;
    KIMAP::ImapInterval m_searchUidInterval;
    bool m_gmailEnabled;
    bool m_searchInChunks;
};

#endif // BATCHFETCHER_H
