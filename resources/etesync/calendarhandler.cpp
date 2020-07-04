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

#include "calendarhandler.h"

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

CalendarHandler::CalendarHandler(EteSyncResource *resource)
    : mResource(resource), mClientState(resource->mClientState)
{
    mResource->initialiseDirectory(baseDirectoryPath());
    AttributeFactory::registerAttribute<CollectionColorAttribute>();
}

void CalendarHandler::setupItems(EteSyncEntry **entries, Akonadi::Collection &collection)
{
    qCDebug(ETESYNC_LOG) << "CalendarHandler: Setting up items";
    QString prevUid = collection.remoteRevision();
    QString journalUid = collection.remoteId();

    bool isIncremental = false;

    if (!prevUid.isEmpty() && !prevUid.isNull()) {
        isIncremental = true;
    }

    EteSyncJournalPtr journal(etesync_journal_manager_fetch(mClientState->journalManager(), journalUid));
    EteSyncCryptoManagerPtr cryptoManager(etesync_journal_get_crypto_manager(journal.get(), mClientState->derived(), mClientState->keypair()));

    QMap<QString, KCalendarCore::Incidence::Ptr> incidences;

    Item::List changedItems;
    Item::List removedItems;

    for (EteSyncEntry **iter = entries; *iter; iter++) {
        EteSyncEntryPtr entry(*iter);
        EteSyncSyncEntryPtr syncEntry(etesync_entry_get_sync_entry(entry.get(), cryptoManager.get(), prevUid));

        CharPtr contentStr(etesync_sync_entry_get_content(syncEntry.get()));

        KCalendarCore::ICalFormat format;
        KCalendarCore::Incidence::Ptr incidence = format.fromString(QStringFromCharPtr(contentStr));

        const QString action = QStringFromCharPtr(CharPtr(etesync_sync_entry_get_action(syncEntry.get())));
        qCDebug(ETESYNC_LOG) << action;
        qCDebug(ETESYNC_LOG) << incidence->created();
        if (action == QStringLiteral(ETESYNC_SYNC_ENTRY_ACTION_ADD) || action == QStringLiteral(ETESYNC_SYNC_ENTRY_ACTION_CHANGE)) {
            incidences[incidence->uid()] = incidence;
        } else if (action == QStringLiteral(ETESYNC_SYNC_ENTRY_ACTION_DELETE)) {
            if (incidences.contains(incidence->uid())) {
                incidences.remove(incidence->uid());
            } else {
                Item item;
                item.setMimeType(KCalendarCore::Event::eventMimeType());
                item.setParentCollection(collection);
                item.setRemoteId(incidence->uid());
                item.setPayload<KCalendarCore::Incidence::Ptr>(incidence);
                removedItems << item;

                deleteLocalCalendar(incidence);
            }
        }

        prevUid = QStringFromCharPtr(CharPtr(etesync_entry_get_uid(entry.get())));
    }

    free(entries);

    collection.setRemoteRevision(prevUid);
    new CollectionModifyJob(collection, this);

    for (auto it = incidences.constBegin(); it != incidences.constEnd(); it++) {
        Item item;
        item.setMimeType(KCalendarCore::Event::eventMimeType());
        item.setParentCollection(collection);
        item.setRemoteId(it.key());
        item.setPayload<KCalendarCore::Incidence::Ptr>(it.value());
        changedItems << item;

        updateLocalCalendar(it.value());
    }

    if (isIncremental) {
        mResource->itemsRetrievedIncremental(changedItems, removedItems);
    } else {
        mResource->itemsRetrieved(changedItems);
    }
}

QString CalendarHandler::baseDirectoryPath() const
{
    return mResource->baseDirectoryPath() + QStringLiteral("/Calendar");
}

QString CalendarHandler::getLocalCalendar(const QString &incidenceUid) const
{
    const QString path = baseDirectoryPath() + QLatin1Char('/') + incidenceUid + QLatin1String(".ical");

    QFile file(path);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        qCDebug(ETESYNC_LOG) << "Unable to read " << path << file.errorString();
        return QString();
    }
    QTextStream in(&file);
    return in.readAll();
}

bool CalendarHandler::updateLocalCalendar(const KCalendarCore::Incidence::Ptr &incidence)
{
    const QString path = baseDirectoryPath() + QLatin1Char('/') + incidence->uid() + QLatin1String(".ical");

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        qCDebug(ETESYNC_LOG) << "Unable to open " << path << file.errorString();
        return false;
    }
    KCalendarCore::Calendar::Ptr calendar(new MemoryCalendar(QTimeZone::utc()));
    calendar->addIncidence(incidence);
    KCalendarCore::ICalFormat format;
    file.write(charArrFromQString(format.toString(calendar)));
    return true;
}

void CalendarHandler::deleteLocalCalendar(const KCalendarCore::Incidence::Ptr &incidence)
{
    const QString path = baseDirectoryPath() + QLatin1Char('/') + incidence->uid() + QLatin1String(".ical");
    QFile file(path);
    if (!file.remove()) {
        qCDebug(ETESYNC_LOG) << "Unable to remove " << path << file.errorString();
        return;
    }
}

void CalendarHandler::itemAdded(const Akonadi::Item &item,
                                const Akonadi::Collection &collection)
{
    KCalendarCore::Calendar::Ptr calendar(new MemoryCalendar(QTimeZone::utc()));
    calendar->addIncidence(item.payload<Incidence::Ptr>());
    KCalendarCore::ICalFormat format;
    qCDebug(ETESYNC_LOG) << "Calendar item added " << format.toString(calendar);

    EteSyncJournalPtr journal(etesync_journal_manager_fetch(mClientState->journalManager(), collection.remoteId()));
    EteSyncCryptoManagerPtr cryptoManager(etesync_journal_get_crypto_manager(journal.get(), mClientState->derived(), mClientState->keypair()));

    EteSyncSyncEntryPtr syncEntry(etesync_sync_entry_new(QStringLiteral(ETESYNC_SYNC_ENTRY_ACTION_ADD), format.toString(calendar)));
    EteSyncEntryPtr entry(etesync_entry_from_sync_entry(cryptoManager.get(), syncEntry.get(), collection.remoteRevision()));

    EteSyncEntryManagerPtr entryManager(etesync_entry_manager_new(mClientState->client(), collection.remoteId()));

    EteSyncEntry *entries[] = {entry.get(), NULL};

    etesync_entry_manager_create(entryManager.get(), entries, collection.remoteRevision());

    mResource->changeCommitted(item);

    updateLocalCalendar(item.payload<Incidence::Ptr>());

    // Update last UID in collection remoteRevision
    const QString entryUid = QStringFromCharPtr(CharPtr(etesync_entry_get_uid(entry.get())));
    Collection col = collection;
    col.setRemoteRevision(entryUid);
    new CollectionModifyJob(col, this);
}

void CalendarHandler::itemChanged(const Akonadi::Item &item,
                                  const QSet<QByteArray> &parts)
{
    Collection collection = item.parentCollection();

    KCalendarCore::Calendar::Ptr calendar(new MemoryCalendar(QTimeZone::utc()));
    calendar->addIncidence(item.payload<Incidence::Ptr>());
    KCalendarCore::ICalFormat format;
    qCDebug(ETESYNC_LOG) << "Calendar item added " << format.toString(calendar);

    EteSyncJournalPtr journal(etesync_journal_manager_fetch(mClientState->journalManager(), collection.remoteId()));
    EteSyncCryptoManagerPtr cryptoManager(etesync_journal_get_crypto_manager(journal.get(), mClientState->derived(), mClientState->keypair()));

    EteSyncSyncEntryPtr syncEntry(etesync_sync_entry_new(QStringLiteral(ETESYNC_SYNC_ENTRY_ACTION_CHANGE), format.toString(calendar)));
    EteSyncEntryPtr entry(etesync_entry_from_sync_entry(cryptoManager.get(), syncEntry.get(), collection.remoteRevision()));

    EteSyncEntryManagerPtr entryManager(etesync_entry_manager_new(mClientState->client(), collection.remoteId()));

    EteSyncEntry *entries[] = {entry.get(), NULL};

    etesync_entry_manager_create(entryManager.get(), entries, collection.remoteRevision());

    mResource->changeCommitted(item);

    updateLocalCalendar(item.payload<Incidence::Ptr>());

    // Update last UID in collection remoteRevision
    const QString entryUid = QStringFromCharPtr(CharPtr(etesync_entry_get_uid(entry.get())));
    Collection col = collection;
    col.setRemoteRevision(entryUid);
    new CollectionModifyJob(col, this);
}

void CalendarHandler::itemRemoved(const Akonadi::Item &item)
{
    Collection collection = item.parentCollection();

    EteSyncJournalPtr journal(etesync_journal_manager_fetch(mClientState->journalManager(), collection.remoteId()));
    EteSyncCryptoManagerPtr cryptoManager(etesync_journal_get_crypto_manager(journal.get(), mClientState->derived(), mClientState->keypair()));

    QString calendar = getLocalCalendar(item.remoteId());

    EteSyncSyncEntryPtr syncEntry(etesync_sync_entry_new(ETESYNC_SYNC_ENTRY_ACTION_DELETE, charArrFromQString(calendar)));
    EteSyncEntryPtr entry(etesync_entry_from_sync_entry(cryptoManager.get(), syncEntry.get(), collection.remoteRevision()));

    EteSyncEntryManagerPtr entryManager(etesync_entry_manager_new(mClientState->client(), collection.remoteId()));

    EteSyncEntry *entries[] = {entry.get(), NULL};

    etesync_entry_manager_create(entryManager.get(), entries, collection.remoteRevision());

    mResource->changeCommitted(item);

    // Update last UID in collection remoteRevision
    const QString entryUid = QStringFromCharPtr(CharPtr(etesync_entry_get_uid(entry.get())));
    collection.setRemoteRevision(entryUid);
    new CollectionModifyJob(collection, this);
}

void CalendarHandler::collectionAdded(const Akonadi::Collection &collection, const Akonadi::Collection &parent)
{
    QString journalUid = QStringFromCharPtr(CharPtr(etesync_gen_uid()));
    EteSyncJournalPtr journal(etesync_journal_new(journalUid, ETESYNC_CURRENT_VERSION));

    /// TODO: Description?
    EteSyncCollectionInfoPtr info(etesync_collection_info_new(QStringLiteral(ETESYNC_COLLECTION_TYPE_CALENDAR), collection.displayName(), QString(), EteSyncDEFAULT_COLOR));

    EteSyncCryptoManagerPtr cryptoManager(etesync_journal_get_crypto_manager(journal.get(), mClientState->derived(), mClientState->keypair()));

    etesync_journal_set_info(journal.get(), cryptoManager.get(), info.get());

    etesync_journal_manager_create(mClientState->journalManager(), journal.get());

    Collection newCollection(collection);
    mResource->setupCollection(newCollection, journal.get());
    mResource->changeCommitted(newCollection);
}

void CalendarHandler::collectionChanged(const Akonadi::Collection &collection)
{
    QString journalUid = collection.remoteId();
    EteSyncJournalPtr journal(etesync_journal_manager_fetch(mClientState->journalManager(), journalUid));

    auto journalColor = EteSyncDEFAULT_COLOR;
    if (collection.hasAttribute<CollectionColorAttribute>()) {
        qCDebug(ETESYNC_LOG) << "Has attr";
        const CollectionColorAttribute *colorAttr = collection.attribute<CollectionColorAttribute>();
        if (colorAttr) {
            qCDebug(ETESYNC_LOG) << "Is not null";
            journalColor = colorAttr->color().rgb();
        }
    }

    qCDebug(ETESYNC_LOG) << "New Color " << journalColor;

    EteSyncCollectionInfoPtr info(etesync_collection_info_new(QStringLiteral(ETESYNC_COLLECTION_TYPE_CALENDAR), collection.displayName(), QString(), journalColor));

    EteSyncCryptoManagerPtr cryptoManager(etesync_journal_get_crypto_manager(journal.get(), mClientState->derived(), mClientState->keypair()));

    etesync_journal_set_info(journal.get(), cryptoManager.get(), info.get());

    etesync_journal_manager_update(mClientState->journalManager(), journal.get());

    mResource->changeCommitted(collection);
}

void CalendarHandler::collectionRemoved(const Akonadi::Collection &collection)
{
    QString journalUid = collection.remoteId();
    EteSyncJournalPtr journal(etesync_journal_manager_fetch(mClientState->journalManager(), journalUid));

    etesync_journal_manager_delete(mClientState->journalManager(), journal.get());
}
