/*
 * Copyright (c) 2014 Christian Mollekopf <mollekopf@kolabsys.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef BATCHFETCHER_H
#define BATCHFETCHER_H

#include <KJob>
#include <KIMAP/ImapSet>
#include <KIMAP/FetchJob>
#include <KIMAP/SearchJob>

#include "messagehelper.h"

/**
 * A job that retrieves a set of messages in reverse-ordered batches.
 * After each batch fetchNextBatch() needs to be called (for throttling the download speed)
 */
class BatchFetcher : public KJob {
    Q_OBJECT
public:
    BatchFetcher(MessageHelper::Ptr messageHelper,
                 const KIMAP::ImapSet &set,
                 const KIMAP::FetchJob::FetchScope &scope,
                 int batchSize,
                 KIMAP::Session *session);
    virtual ~BatchFetcher();
    virtual void start();
    void fetchNextBatch();
    void setUidBased(bool);
    void setSearchUids(const KIMAP::ImapInterval &);
    void setGmailExtensionsEnabled(bool enable);

Q_SIGNALS:
    void itemsRetrieved(Akonadi::Item::List);

private Q_SLOTS:
    void onHeadersReceived(const QString &mailBox,
                           const QMap<qint64, qint64> &uids,
                           const QMap<qint64, qint64> &sizes,
                           const QMap<qint64, KIMAP::MessageAttribute> &attrs,
                           const QMap<qint64, KIMAP::MessageFlags> &flags,
                           const QMap<qint64, KIMAP::MessagePtr> &messages);
    void onHeadersFetchDone(KJob *job);
    void onUidSearchDone(KJob* job);

private:
    //Batch fetching
    KIMAP::ImapSet m_currentSet;
    KIMAP::FetchJob::FetchScope m_scope;
    KIMAP::Session *m_session;
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
