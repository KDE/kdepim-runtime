/*
 * SPDX-FileCopyrightText: 2020 Shashwat Jolly <shashwat.jolly@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "basehandler.h"

#include <AkonadiCore/CollectionModifyJob>
#include <ItemSync>

#include "entriesfetchjob.h"
#include "etesync_debug.h"
#include "etesyncresource.h"

using namespace Akonadi;
using namespace EteSyncAPI;

BaseHandler::BaseHandler(EteSyncResource *resource)
    : mResource(resource)
    , mClientState((resource->mClientState).get())
{
}

void BaseHandler::initialiseBaseDirectory()
{
    mResource->initialiseDirectory(baseDirectoryPath());
}

void BaseHandler::setupItems(std::vector<EteSyncEntryPtr> &entries, Akonadi::Collection &collection, QString &prevUid)
{
    qCDebug(ETESYNC_LOG) << "BaseHandler: Setting up items";

    const QString journalUid = collection.remoteId();

    const bool isIncremental = (prevUid.isEmpty() || prevUid.isNull()) ? false : true;

    Item::List changedItems;
    Item::List removedItems;

    getItemListFromEntries(entries, changedItems, removedItems, collection, journalUid, prevUid);

    if (isIncremental) {
        mResource->itemsRetrievedIncremental(changedItems, removedItems);
    } else {
        mResource->itemsRetrieved(changedItems);
    }

    collection.setRemoteRevision(prevUid);
    new CollectionModifyJob(collection, this);
}

bool BaseHandler::createEteSyncEntry(const EteSyncSyncEntry *syncEntry, const EteSyncCryptoManager *cryptoManager, const Collection &collection)
{
    EteSyncEntryPtr entry = etesync_entry_from_sync_entry(cryptoManager, syncEntry, collection.remoteRevision());
    if (!entry) {
        qCDebug(ETESYNC_LOG) << "Could not create entry from sync entry";
        qCDebug(ETESYNC_LOG) << "EteSync error" << QStringFromCharPtr(CharPtr(etesync_get_error_message()));
        return false;
    }
    EteSyncEntryManagerPtr entryManager = etesync_entry_manager_new(mClientState->client(), collection.remoteId());
    EteSyncEntry *entries[] = {entry.get(), nullptr};
    if (etesync_entry_manager_create(entryManager.get(), entries, collection.remoteRevision())) {
        handleConflictError(collection);
        mResource->handleError();
        return false;
    }
    updateCollectionRevision(entry.get(), collection);
    return true;
}

void BaseHandler::updateCollectionRevision(const EteSyncEntry *entry, const Collection &collection)
{
    const QString entryUid = QStringFromCharPtr(CharPtr(etesync_entry_get_uid(entry)));
    Collection col = collection;
    col.setRemoteRevision(entryUid);
    new CollectionModifyJob(col, this);
}

void BaseHandler::syncCollection(const QVariant &collectionVariant)
{
    const Collection collection = collectionVariant.value<Collection>();

    qCDebug(ETESYNC_LOG) << "Syncing journal" << collection.remoteId();

    auto job = new EntriesFetchJob(mClientState->client(), collection, this);
    connect(job, &EntriesFetchJob::finished, this, &BaseHandler::slotItemsRetrieved);
    job->start();
}

void BaseHandler::slotItemsRetrieved(KJob *job)
{
    if (job->error()) {
        qCWarning(ETESYNC_LOG) << job->errorText();
        return;
    }

    std::vector<EteSyncEntryPtr> entries = qobject_cast<EntriesFetchJob *>(job)->getEntries();

    Collection collection = qobject_cast<EntriesFetchJob *>(job)->collection();

    qCDebug(ETESYNC_LOG) << "BaseHandler: Syncing items";
    QString prevUid = collection.remoteRevision();
    const QString journalUid = collection.remoteId();

    const bool isIncremental = (prevUid.isEmpty() || prevUid.isNull()) ? false : true;

    Item::List changedItems;
    Item::List removedItems;

    getItemListFromEntries(entries, changedItems, removedItems, collection, journalUid, prevUid);

    collection.setRemoteRevision(prevUid);
    new CollectionModifyJob(collection, this);

    ItemSync *syncer = new ItemSync(collection);

    if (isIncremental) {
        syncer->setIncrementalSyncItems(changedItems, removedItems);
    } else {
        syncer->setFullSyncItems(changedItems);
    }
    connect(syncer, SIGNAL(result(KJob*)), this, SLOT(taskDone()));
}

bool BaseHandler::handleConflictError(const Collection &collection)
{
    if (etesync_get_error_code() == EteSyncErrorCode::ETESYNC_ERROR_CODE_CONFLICT) {
        qCDebug(ETESYNC_LOG) << "Conflict error";
        mResource->deferTask();
        mResource->scheduleCustomTask(this, "syncCollection", QVariant::fromValue(collection), EteSyncResource::Prepend);
        return true;
    }
    return false;
}

void BaseHandler::taskDone()
{
    mResource->taskDone();
}
