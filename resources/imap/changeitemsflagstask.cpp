/*
    SPDX-FileCopyrightText: 2013 Daniel Vrátil <dvratil@redhat.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "changeitemsflagstask.h"

#include "highestmodseqattribute.h"
#include "imapresource_debug.h"
#include <KIMAP/SelectJob>
#include <KIMAP/Session>
#include <KIMAP/StoreJob>

ChangeItemsFlagsTask::ChangeItemsFlagsTask(const ResourceStateInterface::Ptr &resource, QObject *parent)
    : ResourceTask(ResourceTask::DeferIfNoSession, resource, parent)
{
}

ChangeItemsFlagsTask::~ChangeItemsFlagsTask() = default;

void ChangeItemsFlagsTask::doStart(KIMAP::Session *session)
{
    auto collection = items().at(0).parentCollection();
    const QString mailBox = mailBoxForCollection(collection);
    qCDebug(IMAPRESOURCE_LOG) << mailBox;
    if (serverSupportsCondstore() && collection.hasAttribute<HighestModSeqAttribute>()) {
        m_condstoreStore = true;
        m_highestModSeq = collection.attribute<HighestModSeqAttribute>()->highestModSequence();

        // We might need to fetch items and update the backend accordingly, set item merging mode in that case
        auto cts = collection.contentMimeTypes();
        cts.removeOne(Akonadi::Collection::mimeType());
        cts.removeOne(KMime::Message::mimeType());
        if (!cts.isEmpty()) {
            setItemMergingMode(Akonadi::ItemSync::GIDMerge);
        } else {
            setItemMergingMode(Akonadi::ItemSync::RIDMerge);
        }
    }

    if (session->selectedMailBox() != mailBox) {
        auto select = new KIMAP::SelectJob(session);
        select->setMailBox(mailBox);

        connect(select, &KJob::result, this, &ChangeItemsFlagsTask::onSelectDone);

        select->start();
    } else {
        if (!addedFlags().isEmpty()) {
            triggerAppendFlagsJob(session);
        } else if (!removedFlags().isEmpty()) {
            triggerRemoveFlagsJob(session);
        } else {
            qCDebug(IMAPRESOURCE_LOG) << "nothing to do";
            changeProcessed();
        }
    }
}

void ChangeItemsFlagsTask::onSelectDone(KJob *job)
{
    if (job->error()) {
        qCWarning(IMAPRESOURCE_LOG) << "Select failed: " << job->errorString();
        cancelTask(job->errorString());
    } else {
        auto select = static_cast<KIMAP::SelectJob *>(job);
        qCDebug(IMAPRESOURCE_LOG) << addedFlags();
        if (!addedFlags().isEmpty()) {
            triggerAppendFlagsJob(select->session());
        } else if (!removedFlags().isEmpty()) {
            triggerRemoveFlagsJob(select->session());
        } else {
            qCDebug(IMAPRESOURCE_LOG) << "nothing to do";
            changeProcessed();
        }
    }
}

KIMAP::StoreJob *ChangeItemsFlagsTask::prepareJob(KIMAP::Session *session)
{
    KIMAP::ImapSet set;
    const Akonadi::Item::List &allItems = items();
    // Split the request to multiple smaller requests of 2000 UIDs each - various IMAP
    // servers have various limits on maximum size of a request
    // 2000 is a random number that sounds like a good compromise between performance
    // and functionality (i.e. 2000 UIDs should be supported by any server out there)
    for (int i = 0, count = qMin(2000, allItems.count() - m_processedItems); i < count; ++i) {
        set.add(allItems[m_processedItems].remoteId().toLong());
        ++m_processedItems;
    }

    auto store = new KIMAP::StoreJob(session);
    store->setUidBased(true);
    store->setSequenceSet(set);
    if (m_condstoreStore) {
        store->setLastModSeq(m_highestModSeq);
    }

    return store;
}

void ChangeItemsFlagsTask::triggerAppendFlagsJob(KIMAP::Session *session)
{
    const auto supportedFlags = fromAkonadiToSupportedImapFlags(addedFlags().values(), items().at(0).parentCollection());
    if (supportedFlags.isEmpty()) {
        if (!removedFlags().isEmpty()) {
            m_processedItems = 0;
            triggerRemoveFlagsJob(session);
        } else if (!m_unchangedMessages.isEmpty()) {
            triggerUnchangedItemsFetch(session);
        } else {
            changeProcessed();
        }
    } else {
        KIMAP::StoreJob *store = prepareJob(session);
        store->setFlags(supportedFlags);
        store->setMode(KIMAP::StoreJob::AppendFlags);
        connect(store, &KIMAP::StoreJob::result, this, &ChangeItemsFlagsTask::onAppendFlagsDone);
        store->start();
    }
}

void ChangeItemsFlagsTask::triggerRemoveFlagsJob(KIMAP::Session *session)
{
    const auto supportedFlags = fromAkonadiToSupportedImapFlags(removedFlags().values(), items().at(0).parentCollection());
    if (supportedFlags.isEmpty()) {
        if (!m_unchangedMessages.isEmpty()) {
            triggerUnchangedItemsFetch(session);
        } else {
            changeProcessed();
        }
    } else {
        KIMAP::StoreJob *store = prepareJob(session);
        store->setFlags(supportedFlags);
        store->setMode(KIMAP::StoreJob::RemoveFlags);
        connect(store, &KIMAP::StoreJob::result, this, &ChangeItemsFlagsTask::onRemoveFlagsDone);
        store->start();
    }
}

void ChangeItemsFlagsTask::onAppendFlagsDone(KJob *job)
{
    if (job->error()) {
        qCWarning(IMAPRESOURCE_LOG) << "Flag append failed: " << job->errorString();
        cancelTask(job->errorString());
    } else {
        KIMAP::StoreJob *storeJob = qobject_cast<KIMAP::StoreJob *>(job);
        KIMAP::Session *session = storeJob->session();
        if (!storeJob->unchangedMessages().isEmpty()) {
            for (const auto &interval : storeJob->unchangedMessages().intervals()) {
                m_unchangedMessages.add(interval);
            }
        }
        if (m_processedItems < items().count()) {
            triggerAppendFlagsJob(session);
        } else if (removedFlags().isEmpty()) {
            if (!m_unchangedMessages.isEmpty()) {
                triggerUnchangedItemsFetch(session);
            } else {
                changeProcessed();
            }
        } else {
            qCDebug(IMAPRESOURCE_LOG) << removedFlags();
            m_processedItems = 0;
            triggerRemoveFlagsJob(session);
        }
    }
}

void ChangeItemsFlagsTask::onRemoveFlagsDone(KJob *job)
{
    if (job->error()) {
        qCWarning(IMAPRESOURCE_LOG) << "Flag remove failed: " << job->errorString();
        cancelTask(job->errorString());
    } else {
        KIMAP::StoreJob *storeJob = qobject_cast<KIMAP::StoreJob *>(job);
        if (!storeJob->unchangedMessages().isEmpty()) {
            for (const auto &interval : storeJob->unchangedMessages().intervals()) {
                m_unchangedMessages.add(interval);
            }
        }
        if (m_processedItems < items().count()) {
            triggerRemoveFlagsJob(qobject_cast<KIMAP::Job *>(job)->session());
        } else {
            if (!m_unchangedMessages.isEmpty()) {
                triggerUnchangedItemsFetch(storeJob->session());
            } else {
                changeProcessed();
            }
        }
    }
}

BatchFetcher *ChangeItemsFlagsTask::createBatchFetcher(MessageHelper::Ptr messageHelper,
                                                       const KIMAP::ImapSet &set,
                                                       const KIMAP::FetchJob::FetchScope &scope,
                                                       int batchSize,
                                                       KIMAP::Session *session)
{
    return new BatchFetcher(messageHelper, set, scope, batchSize, session);
}

void ChangeItemsFlagsTask::triggerUnchangedItemsFetch(KIMAP::Session *session)
{
    KIMAP::FetchJob::FetchScope scope;
    scope.parts.clear();
    scope.mode = KIMAP::FetchJob::FetchScope::Flags;
    m_batchFetcher = createBatchFetcher(resourceState()->messageHelper(), m_unchangedMessages, scope, 10 * batchSize(), session);
    m_batchFetcher->setUidBased(true);
    connect(m_batchFetcher, &BatchFetcher::itemsRetrieved, this, &ChangeItemsFlagsTask::onItemsRetrieved);
    connect(m_batchFetcher, &KJob::result, this, &ChangeItemsFlagsTask::onUnchangedItemsFetchDone);
    m_batchFetcher->start();
}

void ChangeItemsFlagsTask::onReadyForNextBatch(int size)
{
    Q_UNUSED(size)
    if (m_batchFetcher) {
        m_batchFetcher->fetchNextBatch();
    }
}

void ChangeItemsFlagsTask::onItemsRetrieved(const Akonadi::Item::List &addedItems)
{
    for (const auto &item : addedItems) {
        m_updatedMessages << item;
    }
}

void ChangeItemsFlagsTask::onUnchangedItemsFetchDone(KJob *job)
{
    m_batchFetcher = nullptr;
    if (job->error()) {
        qCWarning(IMAPRESOURCE_LOG) << job->errorString();
        cancelTask(job->errorString());
        return;
    }
    changesCommitted(m_updatedMessages);
}

#include "moc_changeitemsflagstask.cpp"
