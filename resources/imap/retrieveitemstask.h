/*
    Copyright (c) 2010 Klarälvdalens Datakonsult AB,
                       a KDAB Group company <info@kdab.com>
    Author: Kevin Ottens <kevin@kdab.com>
    Copyright (c) 2014 Christian Mollekopf <mollekopf@kolabsys.com>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#ifndef RETRIEVEITEMSTASK_H
#define RETRIEVEITEMSTASK_H

#include <kimap/fetchjob.h>

#include "resourcetask.h"
#include <QElapsedTimer>
class BatchFetcher;
namespace Akonadi {
class Session;
}

class RetrieveItemsTask : public ResourceTask
{
    Q_OBJECT

public:
    explicit RetrieveItemsTask(const ResourceStateInterface::Ptr &resource, QObject *parent = nullptr);
    ~RetrieveItemsTask() override;
    void setFetchMissingItemBodies(bool enabled);

public Q_SLOTS:
    void onFetchItemsWithoutBodiesDone(const QVector<qint64> &items);
    void onReadyForNextBatch(int size);

private Q_SLOTS:
    void fetchItemsWithoutBodiesDone(KJob *job);
    void onPreExpungeSelectDone(KJob *job);
    void onExpungeDone(KJob *job);
    void onFinalSelectDone(KJob *job);
    void onStatusDone(KJob *job);
    void onItemsRetrieved(const Akonadi::Item::List &addedItems);
    void onRetrievalDone(KJob *job);
    void onFlagsFetchDone(KJob *job);

protected:
    void doStart(KIMAP::Session *session) override;

    virtual BatchFetcher *createBatchFetcher(MessageHelper::Ptr messageHelper, const KIMAP::ImapSet &set, const KIMAP::FetchJob::FetchScope &scope, int batchSize, KIMAP::Session *session);

private:
    void prepareRetrieval();
    void startRetrievalTasks();
    void triggerPreExpungeSelect(const QString &mailBox);
    void triggerExpunge(const QString &mailBox);
    void triggerFinalSelect(const QString &mailBox);
    void retrieveItems(const KIMAP::ImapSet &set, const KIMAP::FetchJob::FetchScope &scope, bool incremental = false, bool uidBased = false);
    void listFlagsForImapSet(const KIMAP::ImapSet &set);
    void taskComplete();

    KIMAP::Session *m_session = nullptr;
    QVector<qint64> m_messageUidsMissingBody;
    int m_fetchedMissingBodies;
    bool m_fetchMissingBodies;
    bool m_incremental;
    qint64 m_localHighestModSeq;
    BatchFetcher *m_batchFetcher = nullptr;
    Akonadi::Collection m_modifiedCollection;
    bool m_uidBasedFetch;
    bool m_flagsChanged;
    QElapsedTimer m_time;

    // Results of SELECT
    QString m_mailBox;
    int m_messageCount;
    int m_uidValidity;
    qint64 m_nextUid;
    qint64 m_highestModSeq;
    QList<QByteArray> m_flags;
};

#endif
