/*
    SPDX-FileCopyrightText: 2020  Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef RETRIEVEITEMSTASK_QRESYNC_H
#define RETRIEVEITEMSTASK_QRESYNC_H

#include <kimap/fetchjob.h>

#include "resourcetask.h"

#include <QElapsedTimer>
#include <QMap>

#include <memory>

class BatchFetcher;
class RetrieveItemsTaskQResync : public ResourceTask
{
    Q_OBJECT

public:
    explicit RetrieveItemsTaskQResync(const ResourceStateInterface::Ptr &resource, QObject *parent = nullptr);
    ~RetrieveItemsTaskQResync() override;

public Q_SLOTS:
    void onReadyForNextBatch(int size);

protected:
    void doStart(KIMAP::Session *session) override;

    void selectMailbox(KIMAP::Session *session);
    void expungeMessages(KIMAP::Session *session);
    void fetchNewMessages(KIMAP::Session *session);

    bool shouldExpunge(const Akonadi::Collection &collection, bool readOnly) const;

    void finishSync();

protected Q_SLOTS:
    void removeLocalMessages(const KIMAP::ImapSet &messages);
    void updateLocalMessages(const QMap<qint64, KIMAP::Message> &messages);
    void updateLocalCollection();

private:
    struct MailBoxState {
        qint64 uidValidity = -1;
        qint64 nextUid = -1;
        quint64 highestModSeq = 0;
        QList<QByteArray> flags;
        int messageCount = 0;
        int recentCount = 0;
        int firstUnseen = 0;
    };

    MailBoxState m_serverState;
    MailBoxState m_localState;

    enum class SyncMode {
        Full,
        Incremental
    };
    SyncMode m_syncMode = SyncMode::Incremental;


    struct Stats {
        quint64 removed = 0;
        quint64 updated = 0;
        quint64 appended = 0;
        QElapsedTimer timer;
    } m_stats;

    std::unique_ptr<BatchFetcher> m_batchFetcher;
};

#endif
