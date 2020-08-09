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

#include "calendartaskbasehandler.h"

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

CalendarTaskBaseHandler::CalendarTaskBaseHandler(EteSyncResource *resource) : BaseHandler(resource)
{
    initialiseBaseDirectory();
}

void CalendarTaskBaseHandler::getItemListFromEntries(EteSyncEntry **entries, Item::List &changedItems, Item::List &removedItems, Collection &collection, const QString &journalUid, QString &prevUid)
{
    const EteSyncJournalPtr &journal = mResource->getJournal(journalUid);
    EteSyncCryptoManagerPtr cryptoManager(etesync_journal_get_crypto_manager(journal.get(), mClientState->derived(), mClientState->keypair()));

    QMap<QString, KCalendarCore::Incidence::Ptr> incidences;

    for (EteSyncEntry **iter = entries; *iter; iter++) {
        EteSyncEntryPtr entry(*iter);
        EteSyncSyncEntryPtr syncEntry(etesync_entry_get_sync_entry(entry.get(), cryptoManager.get(), prevUid));

        CharPtr contentStr(etesync_sync_entry_get_content(syncEntry.get()));

        KCalendarCore::ICalFormat format;
        KCalendarCore::Incidence::Ptr incidence = format.fromString(QStringFromCharPtr(contentStr));

        const QString action = QStringFromCharPtr(CharPtr(etesync_sync_entry_get_action(syncEntry.get())));
        if (action == QStringLiteral(ETESYNC_SYNC_ENTRY_ACTION_ADD) || action == QStringLiteral(ETESYNC_SYNC_ENTRY_ACTION_CHANGE)) {
            incidences[incidence->uid()] = incidence;
        } else if (action == QStringLiteral(ETESYNC_SYNC_ENTRY_ACTION_DELETE)) {
            if (incidences.contains(incidence->uid())) {
                incidences.remove(incidence->uid());
            } else {
                Item item;
                item.setMimeType(mimeType());
                item.setParentCollection(collection);
                item.setRemoteId(incidence->uid());
                removedItems << item;

                deleteLocalCalendar(incidence);
            }
        }

        prevUid = QStringFromCharPtr(CharPtr(etesync_entry_get_uid(entry.get())));
    }

    free(entries);

    for (auto it = incidences.constBegin(); it != incidences.constEnd(); it++) {
        Item item;
        item.setMimeType(mimeType());
        item.setParentCollection(collection);
        item.setRemoteId(it.key());
        item.setPayload<KCalendarCore::Incidence::Ptr>(it.value());
        changedItems << item;

        updateLocalCalendar(it.value());
    }
}

QString CalendarTaskBaseHandler::baseDirectoryPath() const
{
    return mResource->baseDirectoryPath() + QStringLiteral("/Calendar");
}

QString CalendarTaskBaseHandler::getLocalCalendar(const QString &incidenceUid) const
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

bool CalendarTaskBaseHandler::updateLocalCalendar(const KCalendarCore::Incidence::Ptr &incidence)
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
    file.write(format.toString(calendar).toUtf8());
    return true;
}

void CalendarTaskBaseHandler::deleteLocalCalendar(const KCalendarCore::Incidence::Ptr &incidence)
{
    const QString path = baseDirectoryPath() + QLatin1Char('/') + incidence->uid() + QLatin1String(".ical");
    QFile file(path);
    if (!file.remove()) {
        qCDebug(ETESYNC_LOG) << "Unable to remove " << path << file.errorString();
        return;
    }
}

void CalendarTaskBaseHandler::itemAdded(const Akonadi::Item &item,
                                        const Akonadi::Collection &collection)
{
    KCalendarCore::Calendar::Ptr calendar(new MemoryCalendar(QTimeZone::utc()));
    calendar->addIncidence(item.payload<Incidence::Ptr>());
    KCalendarCore::ICalFormat format;

    QString journalUid = collection.remoteId();
    const EteSyncJournalPtr &journal = mResource->getJournal(journalUid);

    EteSyncCryptoManagerPtr cryptoManager(etesync_journal_get_crypto_manager(journal.get(), mClientState->derived(), mClientState->keypair()));

    EteSyncSyncEntryPtr syncEntry(etesync_sync_entry_new(QStringLiteral(ETESYNC_SYNC_ENTRY_ACTION_ADD), format.toString(calendar)));

    if (!createEteSyncEntry(syncEntry.get(), cryptoManager.get(), collection)) {
        return;
    }

    mResource->changeCommitted(item);

    updateLocalCalendar(item.payload<Incidence::Ptr>());
}

void CalendarTaskBaseHandler::itemChanged(const Akonadi::Item &item,
                                          const QSet<QByteArray> &parts)
{
    Collection collection = item.parentCollection();

    KCalendarCore::Calendar::Ptr calendar(new MemoryCalendar(QTimeZone::utc()));
    calendar->addIncidence(item.payload<Incidence::Ptr>());
    KCalendarCore::ICalFormat format;

    QString journalUid = collection.remoteId();
    const EteSyncJournalPtr &journal = mResource->getJournal(journalUid);

    EteSyncCryptoManagerPtr cryptoManager(etesync_journal_get_crypto_manager(journal.get(), mClientState->derived(), mClientState->keypair()));

    EteSyncSyncEntryPtr syncEntry(etesync_sync_entry_new(QStringLiteral(ETESYNC_SYNC_ENTRY_ACTION_CHANGE), format.toString(calendar)));

    if (!createEteSyncEntry(syncEntry.get(), cryptoManager.get(), collection)) {
        return;
    }

    mResource->changeCommitted(item);

    updateLocalCalendar(item.payload<Incidence::Ptr>());
}

void CalendarTaskBaseHandler::itemRemoved(const Akonadi::Item &item)
{
    Collection collection = item.parentCollection();

    QString journalUid = collection.remoteId();
    const EteSyncJournalPtr &journal = mResource->getJournal(journalUid);

    EteSyncCryptoManagerPtr cryptoManager(etesync_journal_get_crypto_manager(journal.get(), mClientState->derived(), mClientState->keypair()));

    QString calendar = getLocalCalendar(item.remoteId());

    EteSyncSyncEntryPtr syncEntry(etesync_sync_entry_new(ETESYNC_SYNC_ENTRY_ACTION_DELETE, charArrFromQString(calendar)));

    if (!createEteSyncEntry(syncEntry.get(), cryptoManager.get(), collection)) {
        return;
    }

    mResource->changeCommitted(item);
}

void CalendarTaskBaseHandler::collectionAdded(const Akonadi::Collection &collection, const Akonadi::Collection &parent)
{
    QString journalUid = QStringFromCharPtr(CharPtr(etesync_gen_uid()));
    EteSyncJournalPtr journal(etesync_journal_new(journalUid, ETESYNC_CURRENT_VERSION));

    /// TODO: Description?
    EteSyncCollectionInfoPtr info(etesync_collection_info_new(etesyncCollectionType(), collection.displayName(), QString(), EteSyncDEFAULT_COLOR));

    EteSyncCryptoManagerPtr cryptoManager(etesync_journal_get_crypto_manager(journal.get(), mClientState->derived(), mClientState->keypair()));

    etesync_journal_set_info(journal.get(), cryptoManager.get(), info.get());

    const auto result = etesync_journal_manager_create(mClientState->journalManager(), journal.get());
    if (result) {
        mResource->handleTokenError();
        return;
    }

    Collection newCollection(collection);
    mResource->setupCollection(newCollection, journal.get());
    mResource->changeCommitted(newCollection);
}

void CalendarTaskBaseHandler::collectionChanged(const Akonadi::Collection &collection)
{
    QString journalUid = collection.remoteId();
    const EteSyncJournalPtr &journal = mResource->getJournal(journalUid);

    auto journalColor = EteSyncDEFAULT_COLOR;
    if (collection.hasAttribute<CollectionColorAttribute>()) {
        const CollectionColorAttribute *colorAttr = collection.attribute<CollectionColorAttribute>();
        if (colorAttr) {
            journalColor = colorAttr->color().rgb();
        }
    }

    EteSyncCollectionInfoPtr info(etesync_collection_info_new(etesyncCollectionType(), collection.displayName(), QString(), journalColor));

    EteSyncCryptoManagerPtr cryptoManager(etesync_journal_get_crypto_manager(journal.get(), mClientState->derived(), mClientState->keypair()));

    etesync_journal_set_info(journal.get(), cryptoManager.get(), info.get());

    const auto result = etesync_journal_manager_update(mClientState->journalManager(), journal.get());
    if (result) {
        mResource->handleTokenError();
        return;
    }

    mResource->changeCommitted(collection);
}

void CalendarTaskBaseHandler::collectionRemoved(const Akonadi::Collection &collection)
{
    QString journalUid = collection.remoteId();
    const EteSyncJournalPtr &journal = mResource->getJournal(journalUid);

    const auto result = etesync_journal_manager_delete(mClientState->journalManager(), journal.get());
    if (result) {
        mResource->handleTokenError();
        return;
    }
}
