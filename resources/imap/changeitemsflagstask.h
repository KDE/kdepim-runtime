/*
    SPDX-FileCopyrightText: 2013 Daniel Vrátil <dvratil@redhat.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "batchfetcher.h"
#include "resourcetask.h"

namespace KIMAP
{
class StoreJob;
}

class ChangeItemsFlagsTask : public ResourceTask
{
    Q_OBJECT

public:
    explicit ChangeItemsFlagsTask(const ResourceStateInterface::Ptr &resource, QObject *parent = nullptr);
    ~ChangeItemsFlagsTask() override;

protected Q_SLOTS:
    void onSelectDone(KJob *job);
    void onAppendFlagsDone(KJob *job);
    void onRemoveFlagsDone(KJob *job);

    void onUnchangedItemsFetchDone(KJob *job);
    void onItemsRetrieved(const Akonadi::Item::List &addedItems);
    void onReadyForNextBatch(int size);

protected:
    KIMAP::StoreJob *prepareJob(KIMAP::Session *session);

    void doStart(KIMAP::Session *session) override;

    virtual void triggerAppendFlagsJob(KIMAP::Session *session);
    virtual void triggerRemoveFlagsJob(KIMAP::Session *session);

    /*
    Unfortunately, in the case of a CONDSTORE-enabled STORE operation that fails for some message, the server does not necessarily send the infos on the items
    that did not change
    So we have to fetch them manually to correctly update their flags
    */
    virtual void triggerUnchangedItemsFetch(KIMAP::Session *session);

    virtual BatchFetcher *createBatchFetcher(MessageHelper::Ptr messageHelper,
                                             const KIMAP::ImapSet &set,
                                             const KIMAP::FetchJob::FetchScope &scope,
                                             int batchSize,
                                             KIMAP::Session *session);

protected:
    int m_processedItems = 0;

    // CONDTSORE-related fields

    bool m_condstoreStore = false;
    qint64 m_highestModSeq = 0;

    KIMAP::ImapSet m_unchangedMessages;
    Akonadi::Item::List m_updatedMessages;
    BatchFetcher *m_batchFetcher = nullptr;
};
