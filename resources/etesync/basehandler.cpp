/*
 * Copyright (C) 2020 by Shashwat Jolly <shashwat.jolly@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
    : mResource(resource), mClientState(resource->mClientState)
{
}

void BaseHandler::initialiseBaseDirectory()
{
    mResource->initialiseDirectory(baseDirectoryPath());
}

void BaseHandler::setupItems(EteSyncEntry **entries, Akonadi::Collection &collection)
{
    qCDebug(ETESYNC_LOG) << "BaseHandler: Setting up items";
    QString prevUid = collection.remoteRevision();
    const QString journalUid = collection.remoteId();

    const bool isIncremental = (prevUid.isEmpty() || prevUid.isNull()) ? false : true;

    Item::List changedItems;
    Item::List removedItems;

    getItemListFromEntries(entries, changedItems, removedItems, collection, journalUid, prevUid);

    collection.setRemoteRevision(prevUid);
    new CollectionModifyJob(collection, this);

    if (isIncremental) {
        mResource->itemsRetrievedIncremental(changedItems, removedItems);
    } else {
        mResource->itemsRetrieved(changedItems);
    }
}

bool BaseHandler::createEteSyncEntry(const EteSyncSyncEntry *syncEntry, const EteSyncCryptoManager *cryptoManager, const Collection &collection)
{
    EteSyncEntryPtr entry(etesync_entry_from_sync_entry(cryptoManager, syncEntry, collection.remoteRevision()));
    EteSyncEntryManagerPtr entryManager(etesync_entry_manager_new(mClientState->client(), collection.remoteId()));
    EteSyncEntry *entries[] = {entry.get(), NULL};
    const auto result = etesync_entry_manager_create(entryManager.get(), entries, collection.remoteRevision());
    if (result) {
        handleConflictError(collection);
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

    EteSyncEntry **entries = qobject_cast<EntriesFetchJob *>(job)->entries();

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
    connect(syncer, SIGNAL(result(KJob *)), this, SLOT(taskDone()));
}

bool BaseHandler::handleConflictError(const Collection &collection)
{
    if (etesync_get_error_code() == EteSyncErrorCode::ETESYNC_ERROR_CODE_CONFLICT) {
        mResource->deferTask();
        mResource->scheduleCustomTask(this, "syncCollection", QVariant::fromValue(collection), ResourceBase::Prepend);
        return false;
    }
    return true;
}

void BaseHandler::taskDone()
{
    mResource->taskDone();
}