/*
 *  SPDX-FileCopyrightText: 2026 Dominique Michel <dominique.michel@enioka.com>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "removeitemstask.h"

#include "davgroupwareresource.h"
#include "davresource_debug.h"
#include "utils.h"

#include <Akonadi/ItemDeleteJob>
#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemFetchScope>
#include <KDAV/DavItem>
#include <KDAV/DavItemDeleteJob>
#include <KDAV/DavItemModifyJob>

using namespace Qt::Literals;

RemoveItemsTask::RemoveItemsTask(DavGroupwareResource *resource, Akonadi::Item::List items, QObject *parent)
    : ResourceTask(resource, parent)
    , m_items(items)
{
}

void RemoveItemsTask::doStart()
{
    if (m_items.isEmpty()) {
        changeProcessed();
        return;
    }

    m_collection = m_items.first().parentCollection();
    m_davItemCache = resourceDavItemCache().value(m_collection.remoteId());
    if (!m_davItemCache) {
        qCDebug(DAVRESOURCE_LOG) << "Collection has disappeared during RemoveItemsTask !";
        cancelTask();
        return;
    }

    Q_ASSERT(std::ranges::all_of(m_items, [&](const auto &item) {
        return item.parentCollection() == m_collection;
    }));

    // List all items to delete from server
    std::ranges::copy_if(m_items, std::back_inserter(m_itemsToDelete), [](const auto &item) -> bool {
        return !item.remoteId().contains('#'_L1);
    });

    // List all exceptions to remove from item
    for (const auto &item : m_items) {
        auto ridBase = item.remoteId();
        if (!ridBase.contains('#'_L1)) {
            continue;
        }
        ridBase.truncate(ridBase.indexOf('#'_L1));

        const auto isMainItemDeleted = std::ranges::any_of(m_itemsToDelete, [&](const auto &item) {
            return item.remoteId() == ridBase;
        });
        if (isMainItemDeleted) {
            continue;
        }

        if (m_occurrencesToDelete.contains(ridBase)) {
            m_occurrencesToDelete[ridBase].append(item);
        } else {
            m_occurrencesToDelete.insert(ridBase, {item});
        }
    }

    Q_ASSERT(!m_itemsToDelete.isEmpty() || !m_occurrencesToDelete.isEmpty());
    if (!m_occurrencesToDelete.isEmpty()) {
        // Fetch items without their deleted occurrences
        doItemsFetch();
    } else {
        // If no occurrences to delete, jump to full item deletion
        doItemsExceptionDeletion();
    }
}

void RemoveItemsTask::doItemsFetch()
{
    auto itemsToFetch = Akonadi::Item::List();
    for (auto it = m_occurrencesToDelete.constBegin(); it != m_occurrencesToDelete.constEnd(); ++it) {
        const auto &ridBase = it.key();
        const auto &occurrencesToDelete = it.value();

        // Get occurrences remoteIds without the one to delete
        auto exceptionsUrls = m_davItemCache->exceptionUrls(ridBase);
        exceptionsUrls.removeIf([&](const QString &exceptionUrl) {
            return std::ranges::any_of(occurrencesToDelete, [&](const auto &item) {
                return item.remoteId() == exceptionUrl;
            });
        });

        for (const QString &rid : exceptionsUrls) {
            auto exceptionItem = Akonadi::Item();
            exceptionItem.setRemoteId(rid);
            itemsToFetch << exceptionItem;
        }
        auto exceptionItem = Akonadi::Item();
        exceptionItem.setRemoteId(ridBase);
        itemsToFetch << exceptionItem;
    }

    auto job = new Akonadi::ItemFetchJob(itemsToFetch);
    job->setCollection(m_collection);
    job->fetchScope().fetchFullPayload();
    job->fetchScope().setAncestorRetrieval(Akonadi::ItemFetchScope::Parent);
    connect(job, &Akonadi::ItemFetchJob::result, this, &RemoveItemsTask::onItemsFetched);
}

void RemoveItemsTask::onItemsFetched(KJob *job)
{
    const auto fetchJob = qobject_cast<Akonadi::ItemFetchJob *>(job);
    if (fetchJob->error()) {
        qCCritical(DAVRESOURCE_LOG) << "RemoveItemsTask: Error fetching items: " << fetchJob->errorString();
        cancelTask();
        return;
    }

    // Main item will be at front of it's list
    auto itemsOccurrences = QMap<QString, Akonadi::Item::List>();
    auto fetchedItems = fetchJob->items();
    for (const auto &item : fetchedItems) {
        auto ridBase = item.remoteId();
        const auto isMainItem = !ridBase.contains('#'_L1);
        if (!isMainItem) {
            ridBase.truncate(ridBase.indexOf('#'_L1));
        }

        if (itemsOccurrences.contains(ridBase)) {
            if (isMainItem) {
                itemsOccurrences[ridBase].push_front(item);
            } else {
                itemsOccurrences[ridBase].append(item);
            }
        } else {
            itemsOccurrences.insert(ridBase, {item});
        }
    }

    doItemsChange(itemsOccurrences);
}

void RemoveItemsTask::doItemsChange(const QMap<QString, Akonadi::Item::List> &itemsOccurrences)
{
    // Start modify jobs
    for (auto it = itemsOccurrences.constBegin(); it != itemsOccurrences.constEnd(); ++it) {
        const auto &ridBase = it.key();
        const auto &items = it.value();

        const auto &mainItem = items.first();
        Q_ASSERT(ridBase == mainItem.remoteId());
        const auto &dependentItems = items.sliced(1);

        KDAV::DavItem davItem = Utils::createDavItem(mainItem, mainItem.parentCollection(), dependentItems);
        if (davItem.data().isEmpty()) {
            qCCritical(DAVRESOURCE_LOG) << "Item " << mainItem.id() << " doesn't has a valid payload";
            // TODO:FIXME: save error but ignore current item
        }

        const KDAV::DavUrl davUrl = resourceSettings()->davUrlFromCollectionUrl(mainItem.parentCollection().remoteId(), mainItem.remoteId());
        davItem.setUrl(davUrl);
        davItem.setEtag(mainItem.remoteRevision());

        auto modJob = new KDAV::DavItemModifyJob(davItem);
        connect(modJob, &KDAV::DavItemModifyJob::result, this, [this, ridBase](KJob *job) {
            onItemChanged(job, ridBase);
        });
        modJob->start();
        m_davItemChangeJobCounter += 1;
    }
}

void RemoveItemsTask::onItemChanged(KJob *job, const QString &ridBase)
{
    m_davItemChangeJobCounter -= 1;

    const auto modifyJob = qobject_cast<KDAV::DavItemModifyJob *>(job);
    if (modifyJob->error()) {
        qCCritical(DAVRESOURCE_LOG) << "RemoveItemsTask: Error fetching item: " << modifyJob->errorString();
        // TODO:FIXME: save error but ignore current item
        // TODO:FIXME: what do we do with cache ?
    } else {
        for (const auto &occurrences : m_occurrencesToDelete[ridBase]) {
            m_davItemCache->removeException(occurrences.remoteId());
        }
    }

    // If all changed finished, jump to full item deletion
    if (m_davItemChangeJobCounter == 0) {
        doItemsExceptionDeletion();
    }
}

void RemoveItemsTask::doItemsExceptionDeletion()
{
    auto itemsExceptions = Akonadi::Item::List();
    for (const auto &itemToDelete : m_itemsToDelete) {
        const auto exceptionUrls = m_davItemCache->exceptionUrls(itemToDelete.remoteId());
        for (const auto &exceptionUrl : exceptionUrls) {
            auto item = Akonadi::Item();
            item.setRemoteId(exceptionUrl);
            itemsExceptions.append(item);
        }
    }

    // Skip if empty
    if (itemsExceptions.isEmpty()) {
        doItemDeletion();
        return;
    }

    auto deleteJob = new Akonadi::ItemDeleteJob(itemsExceptions, m_collection);
    connect(deleteJob, &Akonadi::ItemDeleteJob::result, this, [this](KJob *job) {
        if (job->error()) {
            qCWarning(DAVRESOURCE_LOG()) << "Error deleting items exceptions from akonadi:" << job->errorText();
            // TODO:FIXME: save error but ignore current item
            // TODO:FIXME: what do we do with cache ?
        }

        doItemDeletion();
    });
    deleteJob->start();
}

void RemoveItemsTask::doItemDeletion()
{
    if (m_itemsToDelete.isEmpty()) {
        finished();
        return;
    }

    for (const auto &item : m_itemsToDelete) {
        const KDAV::DavUrl davUrl = resourceSettings()->davUrlFromCollectionUrl(item.parentCollection().remoteId(), item.remoteId());

        KDAV::DavItem davItem;
        davItem.setUrl(davUrl);
        davItem.setEtag(item.remoteRevision());

        auto job = new KDAV::DavItemDeleteJob(davItem);
        connect(job, &KDAV::DavItemDeleteJob::result, this, [this, item](KJob *job) {
            onItemDeleted(job, item);
        });
        job->start();
        m_davItemDeleteJobCounter += 1;
    }
}

void RemoveItemsTask::onItemDeleted(KJob *job, const Akonadi::Item &item)
{
    m_davItemDeleteJobCounter -= 1;

    const auto deleteJob = qobject_cast<KDAV::DavItemDeleteJob *>(job);
    if (deleteJob->error()) {
        qCCritical(DAVRESOURCE_LOG) << "RemoveItemsTask: Error deleting item: " << deleteJob->errorString();
        // TODO:FIXME: save error but ignore current item
        // TODO:FIXME: what do we do with cache ?
    } else {
        m_davItemCache->removeEtag(item.remoteId());
    }

    // If all changed finished, jump to full item deletion
    if (m_davItemDeleteJobCounter == 0) {
        finished();
    }
}

void RemoveItemsTask::finished()
{
    // TODO:FIXME: store errors along the way and notify here
    changeProcessed();
}

#include "moc_removeitemstask.cpp"
