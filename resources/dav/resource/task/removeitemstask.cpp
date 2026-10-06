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
#include <Akonadi/ItemModifyJob>
#include <KDAV/DavItem>
#include <KDAV/DavItemDeleteJob>
#include <KDAV/DavItemModifyJob>
#include <klocalizedstring.h>

using namespace Qt::Literals;

RemoveItemsTask::RemoveItemsTask(ResourceStateInterface::Ptr resource, const Akonadi::Item::List &items, QObject *parent)
    : ResourceTask(std::move(resource), parent)
    , m_items(items)
{
}

void RemoveItemsTask::filterDeletableItemsAndExceptions(const Akonadi::Item::List &items)
{
    // Only delete what remains in the cache, helps when retrying
    auto remainingItems = Akonadi::Item::List();
    std::ranges::copy_if(items, std::back_inserter(remainingItems), [&](const auto &item) -> bool {
        return m_davItemCache->containsItem(item.remoteId());
    });

    // List all main items to delete from server
    std::ranges::copy_if(remainingItems, std::back_inserter(m_itemsToDelete), [](const auto &item) -> bool {
        return !item.remoteId().contains('#'_L1);
    });

    // List all exceptions that need to be removed from it's main item
    for (const auto &item : std::as_const(remainingItems)) {
        auto ridBase = item.remoteId();
        if (!ridBase.contains('#'_L1)) {
            continue;
        }
        ridBase.truncate(ridBase.indexOf('#'_L1));

        const auto isMainItemDeleted = std::ranges::any_of(m_itemsToDelete, [&](const auto &mainItem) {
            return mainItem.remoteId() == ridBase;
        });
        if (isMainItemDeleted) {
            continue;
        }

        if (m_exceptionsToDelete.contains(ridBase)) {
            m_exceptionsToDelete[ridBase].append(item);
        } else {
            m_exceptionsToDelete.insert(ridBase, {item});
        }
    }
}

void RemoveItemsTask::doStart()
{
    if (m_items.isEmpty()) {
        finished();
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

    filterDeletableItemsAndExceptions(m_items);

    if (m_itemsToDelete.isEmpty() && m_exceptionsToDelete.isEmpty()) {
        finished();
    } else if (!m_exceptionsToDelete.isEmpty()) {
        doExceptionItemsFetch();
    } else {
        doLocalItemsExceptionDeletion();
    }
}

void RemoveItemsTask::doExceptionItemsFetch()
{
    auto itemsToFetch = Akonadi::Item::List();
    for (auto it = m_exceptionsToDelete.constBegin(); it != m_exceptionsToDelete.constEnd(); ++it) {
        const auto &ridBase = it.key();

        // Get occurrences remoteIds without the one to delete
        const auto exceptionsUrls = m_davItemCache->exceptionUrls(ridBase);
        for (const auto &rid : std::as_const(exceptionsUrls)) {
            auto exceptionItem = Akonadi::Item();
            exceptionItem.setRemoteId(rid);
            itemsToFetch << exceptionItem;
        }
        auto exceptionItem = Akonadi::Item();
        exceptionItem.setRemoteId(ridBase);
        itemsToFetch << exceptionItem;
    }

    auto *job = new Akonadi::ItemFetchJob(itemsToFetch);
    job->setCollection(m_collection);
    job->fetchScope().fetchFullPayload();
    job->fetchScope().setAncestorRetrieval(Akonadi::ItemFetchScope::Parent);
    connect(job, &Akonadi::ItemFetchJob::result, this, &RemoveItemsTask::onExceptionItemsFetched);
}

void RemoveItemsTask::onExceptionItemsFetched(KJob *job)
{
    const auto *fetchJob = qobject_cast<Akonadi::ItemFetchJob *>(job);
    if (fetchJob->error()) {
        qCCritical(DAVRESOURCE_LOG) << "RemoveItemsTask: Error fetching items: " << fetchJob->errorString();
        onError(ErrorType::Unrecoverable);
        doLocalItemsExceptionDeletion(); // Jump to next step
        return;
    }

    auto itemsExceptions = QMap<QString, Akonadi::Item::List>();
    auto fetchedItems = fetchJob->items();
    for (const auto &item : fetchedItems) {
        auto ridBase = item.remoteId();
        const auto isMainItem = !ridBase.contains('#'_L1);
        if (!isMainItem) {
            ridBase.truncate(ridBase.indexOf('#'_L1));
        }

        // Main item will be put at the front of it's list
        if (itemsExceptions.contains(ridBase)) {
            if (isMainItem) {
                itemsExceptions[ridBase].push_front(item);
            } else {
                itemsExceptions[ridBase].append(item);
            }
        } else {
            itemsExceptions.insert(ridBase, {item});
        }
    }

    // Remove items without a main item, it has been deleted in Akonadi server and will execute an event of it's own
    itemsExceptions.removeIf([](const QMap<QString, Akonadi::Item::List>::iterator &element) {
        const auto &items = element.value();
        return items.isEmpty() || items.first().remoteId().contains('#'_L1);
    });

    doExceptionItemsUpdate(itemsExceptions);
}

void RemoveItemsTask::doExceptionItemsUpdate(const QMap<QString, Akonadi::Item::List> &itemsOccurrences)
{
    for (const auto &[ridBase, items] : itemsOccurrences.asKeyValueRange()) {
        if (items.isEmpty()) {
            continue; // Items missing in Akonadi server and will fire their own event, we ignore
        }

        const auto &mainItem = items.first();
        if (ridBase != mainItem.remoteId()) {
            continue; // Main item missing in Akonadi server and will fire it's own event, we ignore
        }

        const auto &dependentItems = items.sliced(1);
        KDAV::DavItem davItem = Utils::createDavItem(mainItem, mainItem.parentCollection(), dependentItems);
        if (davItem.data().isEmpty()) {
            qCCritical(DAVRESOURCE_LOG) << "Item " << mainItem.id() << " doesn't has a valid payload";
            onError(ErrorType::Unrecoverable);
            continue;
        }

        const KDAV::DavUrl davUrl = davUrlFromCollectionUrl(mainItem.parentCollection().remoteId(), mainItem.remoteId());
        davItem.setUrl(davUrl);
        davItem.setEtag(mainItem.remoteRevision());

        auto *modJob = new KDAV::DavItemModifyJob(davItem);
        connect(modJob, &KDAV::DavItemModifyJob::result, this, [this, mainItem, dependentItems](KJob *job) {
            onExceptionItemsUpdated(job, mainItem, dependentItems);
        });
        modJob->start();
        m_davItemChangeJobCounter += 1;
    }

    // If no jobs are started, jump to next step
    if (m_davItemChangeJobCounter == 0) {
        doLocalItemsExceptionDeletion();
    }
}

void RemoveItemsTask::onExceptionItemsUpdated(KJob *job, const Akonadi::Item &mainItem, const Akonadi::Item::List &dependentItems)
{
    m_davItemChangeJobCounter -= 1;

    const auto ridBase = mainItem.remoteId();
    const auto modifyJob = qobject_cast<KDAV::DavItemModifyJob *>(job);
    if (modifyJob->error()) {
        qCCritical(DAVRESOURCE_LOG) << "RemoveItemsTask: Error fetching item: " << modifyJob->errorString();
        onDavJobError(modifyJob);
    } else {
        for (const auto &occurrences : m_exceptionsToDelete[ridBase]) {
            m_davItemCache->removeException(occurrences.remoteId());
        }

        const auto davItem = modifyJob->item();
        auto updateItems = Akonadi::Item::List();
        updateItems.reserve(dependentItems.size() + 1);

        updateItems.append(mainItem);
        updateItems.back().setRemoteRevision(davItem.etag());
        m_davItemCache->setEtag(davItem.url().toDisplayString(), davItem.etag());

        for (const auto &exception : std::as_const(dependentItems)) {
            updateItems.append(exception);
            updateItems.back().setRemoteRevision(davItem.etag());
            m_davItemCache->setEtag(exception.remoteId(), davItem.etag());
        }

        auto *itemModifyJob = new Akonadi::ItemModifyJob(updateItems);
        itemModifyJob->setIgnorePayload(true);
        itemModifyJob->start();
    }

    // If all changed finished, jump to full item deletion
    if (m_davItemChangeJobCounter == 0) {
        doLocalItemsExceptionDeletion();
    }
}

void RemoveItemsTask::doLocalItemsExceptionDeletion()
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

    if (itemsExceptions.isEmpty()) {
        doItemsDeletion();
        return;
    }

    auto *deleteJob = new Akonadi::ItemDeleteJob(itemsExceptions, m_collection);
    connect(deleteJob, &Akonadi::ItemDeleteJob::result, this, [this](KJob *job) {
        if (job->error()) {
            qCWarning(DAVRESOURCE_LOG()) << "Error deleting items exceptions from Akonadi:" << job->errorText();
            onError(ErrorType::Unrecoverable);
        }

        doItemsDeletion();
    });
    deleteJob->start();
}

void RemoveItemsTask::doItemsDeletion()
{
    for (const auto &item : std::as_const(m_itemsToDelete)) {
        const auto davUrl = davUrlFromCollectionUrl(item.parentCollection().remoteId(), item.remoteId());
        KDAV::DavItem davItem;
        davItem.setUrl(davUrl);
        davItem.setEtag(item.remoteRevision());

        auto *job = new KDAV::DavItemDeleteJob(davItem);
        connect(job, &KDAV::DavItemDeleteJob::result, this, [this, item](KJob *job) {
            onItemDeleted(job, item);
        });
        job->start();
        m_davItemDeleteJobCounter += 1;
    }

    // If no jobs are started, jump to next step
    if (m_davItemDeleteJobCounter == 0) {
        finished();
    }
}

void RemoveItemsTask::onItemDeleted(KJob *job, const Akonadi::Item &item)
{
    m_davItemDeleteJobCounter -= 1;

    const auto *deleteJob = qobject_cast<KDAV::DavItemDeleteJob *>(job);
    if (deleteJob->error()) {
        qCCritical(DAVRESOURCE_LOG) << "RemoveItemsTask: Error deleting item: " << deleteJob->errorString();
        onDavJobError(deleteJob);
    } else {
        const auto remoteId = item.remoteId();
        for (const auto &exceptionUrl : m_davItemCache->exceptionUrls(remoteId)) {
            m_davItemCache->removeException(exceptionUrl);
        }
        m_davItemCache->removeEtag(remoteId);
    }

    // If all changed finished, jump to full item deletion
    if (m_davItemDeleteJobCounter == 0) {
        finished();
    }
}

void RemoveItemsTask::finished()
{
    switch (m_error) {
    case ErrorType::NoError:
        changeProcessed();
        break;
    case ErrorType::Retryable:
        retryAfterFailure(m_errorMessage);
        break;
    case ErrorType::Unrecoverable:
        // We might be in an inconsistent state and un/committing the transaction won't match the new server state where
        // some update/deletion succeeded and failed.
        // To restore consistency, we synchronize the collection and show an error message.
        cancelTask(i18n("Some deletion failed"));
        synchronizeCollection(m_collection);
        break;
    }
}

#include "moc_removeitemstask.cpp"
