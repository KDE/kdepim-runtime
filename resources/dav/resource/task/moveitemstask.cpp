/*
 *  SPDX-FileCopyrightText: 2026 Dominique Michel <dominique.michel@enioka.com>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "moveitemstask.h"

#include "davgroupwareresource.h"
#include "davresource_debug.h"
#include "utils.h"

#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemFetchScope>
#include <Akonadi/ItemModifyJob>
#include <KDAV/DavItem>
#include <kstandardguiitem.h>
#if KDAV_VERSION >= QT_VERSION_CHECK(6, 30, 0)
#include <KDAV/DavItemMoveJob>
#else
#include <KDAV/DavItemCreateJob>
#include <KDAV/DavItemDeleteJob>
#endif
#include <KIO/DavJob>
#include <kcalendarcore/todo.h>
#include <klocalizedstring.h>

using namespace Qt::Literals;

using IncidencePtr = QSharedPointer<KCalendarCore::Incidence>;

MoveItemsTask::MoveItemsTask(ResourceStateInterface::Ptr resource,
                             const Akonadi::Item::List &items,
                             const Akonadi::Collection &collectionSrc,
                             const Akonadi::Collection &collectionDst,
                             QObject *parent)
    : ResourceTask(std::move(resource), parent)
    , m_items(items)
    , m_collectionSrc(collectionSrc)
    , m_collectionDst(collectionDst)
{
    Q_ASSERT(m_collectionSrc.isValid());
    Q_ASSERT(m_collectionDst.isValid());

    // Items given by Observer have old remoteId and new parentCollection, let's update them with new remoteId to be consistent
    for (auto &item : m_items) {
        if (!item.remoteId().isEmpty()) {
            if (item.remoteId().startsWith(m_collectionSrc.remoteId())) {
                item.setRemoteId(item.remoteId().replace(m_collectionSrc.remoteId(), m_collectionDst.remoteId()));
            } else {
                Q_ASSERT(item.remoteId().startsWith(m_collectionDst.remoteId()));
            }
        }
        Q_ASSERT(item.parentCollection() == m_collectionDst);
    }
}

void MoveItemsTask::doStart()
{
    m_davItemCacheSrc = resourceDavItemCache().value(m_collectionSrc.remoteId());
    m_davItemCacheDst = resourceDavItemCache().value(m_collectionDst.remoteId());
    if (!m_davItemCacheSrc || !m_davItemCacheDst) {
        qCDebug(DAVRESOURCE_LOG) << "Collection source and/or destination has disappeared during item move !";
        onError(ErrorType::Unrecoverable);
        finished();
        return;
    }

    for (auto &item : std::as_const(m_items)) {
        // Exceptions are moved with the main item, skip them
        const auto remoteId = item.remoteId();
        if (remoteId.isEmpty() || remoteId.contains('#'_L1)) {
            continue;
        }

        if (m_davItemCacheDst->containsItem(item.remoteId())) {
            // Item has already been moved, happens when this job is retrying
            continue;
        }

        auto oldRemoteId = remoteId;
        oldRemoteId.replace(m_collectionDst.remoteId(), m_collectionSrc.remoteId());

        // Without exceptions, item can be moved without exception fetching
        const auto exceptionsUrls = m_davItemCacheSrc->exceptionUrls(oldRemoteId);
        if (exceptionsUrls.isEmpty()) {
            m_itemsToMove.insert(remoteId, {item});
        } else {
            m_itemsToFetch << item;
        }
    }

    if (m_itemsToFetch.isEmpty() && m_itemsToMove.isEmpty()) {
        finished();
    } else if (!m_itemsToFetch.isEmpty()) {
        doExceptionItemsFetch();
    } else {
        doItemsMove();
    }
}

void MoveItemsTask::doExceptionItemsFetch()
{
    // TODO: when items are moved, their remoteId is deleted in the akonadi-server, but not in the provided item :
    // - we can't fetch dependentItems using the remoteId in davItemCache
    // - for now we will fetch all items and filter using event's UID
    // It's really not ideal, but I don't see another clean way, and should happen rarely enough

    auto uidsMap = QMap<QString, Akonadi::Item>();
    for (const auto &fetchItem : m_itemsToFetch) {
        if (fetchItem.hasPayload<IncidencePtr>()) {
            uidsMap.insert(fetchItem.payload<IncidencePtr>()->uid(), fetchItem);
        }
    }
    if (uidsMap.isEmpty()) {
        doItemsMove(); // Jump to next step
        return;
    }

    auto *fetchJob = new Akonadi::ItemFetchJob(m_collectionDst);
    fetchJob->fetchScope().fetchFullPayload();
    connect(fetchJob, &KJob::result, this, [this, uidsMap = std::move(uidsMap)](KJob *job) {
        onExceptionItemsFetched(job, uidsMap);
    });
    fetchJob->start();
}

void MoveItemsTask::onExceptionItemsFetched(KJob *job, const QMap<QString, Akonadi::Item> &uidsMap)
{
    const auto *fetchJob = static_cast<Akonadi::ItemFetchJob *>(job);
    if (job->error()) {
        qCWarning(DAVRESOURCE_LOG()) << "Unable to fetch items during items move:" << job->errorString();
        onError(ErrorType::Unrecoverable);
        doItemsMove(); // Jump to next step
        return;
    }

    auto fetchItems = fetchJob->items();
    for (auto &fetchItem : fetchItems) {
        if (!fetchItem.hasPayload<IncidencePtr>()) {
            continue;
        }
        const auto incidence = fetchItem.payload<IncidencePtr>();
        const auto mainItemIt = uidsMap.find(incidence->uid());
        if (mainItemIt == uidsMap.end()) {
            continue;
        }

        // Restore remoteId (it's the destination remoteId)
        const auto ridBase = mainItemIt->remoteId();
        if (incidence->hasRecurrenceId()) {
            fetchItem.setRemoteId(ridBase + "#"_L1 + incidence->instanceIdentifier());
        } else {
            fetchItem.setRemoteId(ridBase);
        }

        // Insert items to be moved
        if (auto itemsToMove = m_itemsToMove.find(ridBase); itemsToMove != m_itemsToMove.end()) {
            if (!incidence->hasRecurrenceId()) {
                itemsToMove.value().push_front(fetchItem);
            } else {
                itemsToMove.value().append(fetchItem);
            }
        } else {
            m_itemsToMove.insert(ridBase, {fetchItem});
        }
    }

    doItemsMove();
}

void MoveItemsTask::doItemsMove()
{
    for (const auto &[ridBase, newItems] : m_itemsToMove.asKeyValueRange()) {
        if (newItems.isEmpty()) {
            continue;
        }
        if (newItems.first().remoteId().contains('#'_L1)) {
            qCWarning(DAVRESOURCE_LOG()) << "Item to move is missing main item:" << newItems.first().remoteId() << "skipping...";
            continue;
        }
        auto newMainItem = newItems.first();
        auto newDependentItems = newItems.sliced(1);

        // Old items are same as new item but located in source collection, meaning old remoteId and parentCollection
        auto oldItems = newItems;
        for (auto &oldItem : oldItems) {
            oldItem.setRemoteId(oldItem.remoteId().replace(m_collectionDst.remoteId(), m_collectionSrc.remoteId()));
            oldItem.setParentCollection(m_collectionSrc);
        }

        auto newDavItem = Utils::createDavItem(newMainItem, m_collectionDst, newDependentItems);
#if KDAV_VERSION >= QT_VERSION_CHECK(6, 30, 0)
        // We must send the move job to our item located at it's old location
        const auto oldDavUrl = davUrlFromCollectionUrl(m_collectionSrc.remoteId(), oldItems.first().remoteId());
        newDavItem.setUrl(oldDavUrl);

        // We must not pass an authenticated url as destination, only the destination path
        auto *job = new KDAV::DavItemMoveJob(newDavItem, QUrl::fromUserInput(newMainItem.remoteId()));
        connect(job, &KDAV::DavItemMoveJob::result, this, [this, oldItems, newItems](KJob *job) mutable {
            onItemsMoved(job, oldItems, newItems);
        });
        job->start();
        m_moveJobCounter += 1;
#else
        const auto newDavUrl = resourceSettings()->davUrlFromCollectionUrl(m_collectionDst.remoteId(), newDavItem.url().toDisplayString());
        newDavItem.setUrl(newDavUrl);
        auto *createJob = new KDAV::DavItemCreateJob(newDavItem);
        connect(createJob, &KJob::result, this, [this, newItems, oldItems, newDavItem](KJob *job) mutable {
            onItemCreated(job, oldItems, newItems, newDavItem);
        });
        createJob->start();
        m_moveJobCounter += 1;

#endif
    }

    // If no jobs are started, jump to next step
    if (m_moveJobCounter == 0) {
        finished();
    }
}

#if KDAV_VERSION >= QT_VERSION_CHECK(6, 30, 0)
void MoveItemsTask::onItemsMoved(KJob *job, const Akonadi::Item::List &oldItems, const Akonadi::Item::List &newItems)
{
    const auto *moveJob = qobject_cast<KDAV::DavItemMoveJob *>(job);
    if (moveJob->error()) {
        qCCritical(DAVRESOURCE_LOG) << "MoveItemsTask: Error moving item: " << moveJob->errorString();
        onDavJobError(moveJob);
        onMoveJobDone();
        return;
    }

    for (const auto &oldItem : oldItems) {
        m_davItemCacheSrc->removeEtag(oldItem.remoteId());
    }
    for (const auto &newItem : newItems) {
        m_davItemCacheDst->setEtag(newItem.remoteId(), newItem.remoteRevision());
    }

    auto *itemModifyJob = new Akonadi::ItemModifyJob(newItems);
    itemModifyJob->setIgnorePayload(true);
    itemModifyJob->disableRevisionCheck();
    connect(itemModifyJob, &KJob::result, this, [this](KJob *job) {
        if (job->error()) {
            qCWarning(DAVRESOURCE_LOG) << "MoveItemsTask: Error modifying item with new remoteId: " << job->errorString();
        }
        onMoveJobDone();
    });
    itemModifyJob->start();
}
#else
void MoveItemsTask::onItemCreated(KJob *job, const Akonadi::Item::List &oldItems, Akonadi::Item::List newItems, const KDAV::DavItem &newDavItem)
{
    const auto *createJob = qobject_cast<KDAV::DavItemCreateJob *>(job);
    if (job->error()) {
        onDavJobError(createJob);
        onMoveJobDone();
        return;
    }

    const auto createdDavItem = createJob->item();
    const auto newRemoteId = createdDavItem.url().toDisplayString();
    const auto newEtag = createdDavItem.etag();

    // Update newItems remoteIds and remoteRevision
    for (auto &newItem : newItems) {
        const auto separatorIndex = newItem.remoteId().indexOf(u'#');
        if (separatorIndex != -1) {
            newItem.setRemoteId(newRemoteId + newItem.remoteId().mid(separatorIndex));
        } else {
            newItem.setRemoteId(newRemoteId);
        }
        newItem.setRemoteRevision(newEtag);
    }

    // Update cache
    for (const auto &newItem : newItems) {
        m_davItemCacheDst->setEtag(newItem.remoteId(), newItem.remoteRevision());
    }

    // Update remoteId's in Akonadi server
    auto *itemModifyJob = new Akonadi::ItemModifyJob(newItems);
    itemModifyJob->setIgnorePayload(true);
    itemModifyJob->disableRevisionCheck();
    itemModifyJob->start();

    // Delete the dav item located at the old location
    const auto oldDavUrl = davUrlFromCollectionUrl(m_collectionSrc.remoteId(), oldItems.first().remoteId());
    auto deleteDavItem = newDavItem;
    deleteDavItem.setUrl(oldDavUrl);
    auto *deleteJob = new KDAV::DavItemDeleteJob(deleteDavItem);
    connect(deleteJob, &KDAV::DavItemDeleteJob::result, this, [this, oldItems](KJob *deleteJob) {
        if (deleteJob->error()) {
            qCWarning(DAVRESOURCE_LOG()) << "Unable to delete item during move:" << deleteJob->errorString();
            onError(ErrorType::Unrecoverable);
        } else {
            // Update cache
            for (const auto &oldItem : oldItems) {
                m_davItemCacheSrc->removeEtag(oldItem.remoteId());
            }
        }

        onMoveJobDone();
    });
    deleteJob->start();
}
#endif

void MoveItemsTask::onMoveJobDone()
{
    m_moveJobCounter -= 1;
    if (m_moveJobCounter == 0) {
        finished();
    }
}

void MoveItemsTask::finished()
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
        synchronizeCollection(m_collectionSrc);
        synchronizeCollection(m_collectionDst);
        break;
    }
}

#include "moc_moveitemstask.cpp"
