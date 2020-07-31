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

#include <AkonadiCore/AttributeFactory>
#include <AkonadiCore/CollectionColorAttribute>
#include <AkonadiCore/CollectionModifyJob>
#include <KCalendarCore/ICalFormat>
#include <KCalendarCore/MemoryCalendar>
#include <QFile>

#include "entriesfetchjob.h"
#include "etesync_debug.h"
#include "etesyncresource.h"

using namespace Akonadi;
using namespace KCalendarCore;

BaseHandler::BaseHandler(EteSyncResource *resource)
    : mResource(resource), mClientState(resource->mClientState)
{
}

void BaseHandler::initialiseBaseDirectory()
{
    mResource->initialiseDirectory(baseDirectoryPath());
}

void BaseHandler::createEteSyncEntry(const EteSyncSyncEntry *syncEntry, const EteSyncCryptoManager *cryptoManager, const Collection &collection)
{
    EteSyncEntryPtr entry(etesync_entry_from_sync_entry(cryptoManager, syncEntry, collection.remoteRevision()));
    EteSyncEntryManagerPtr entryManager(etesync_entry_manager_new(mClientState->client(), collection.remoteId()));
    EteSyncEntry *entries[] = {entry.get(), NULL};
    etesync_entry_manager_create(entryManager.get(), entries, collection.remoteRevision());

    updateCollectionRevision(entry.get(), collection);
}

void BaseHandler::updateCollectionRevision(const EteSyncEntry *entry, const Collection &collection)
{
    const QString entryUid = QStringFromCharPtr(CharPtr(etesync_entry_get_uid(entry)));
    Collection col = collection;
    col.setRemoteRevision(entryUid);
    new CollectionModifyJob(col, this);
}