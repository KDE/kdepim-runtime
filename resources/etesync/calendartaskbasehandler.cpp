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
#include <KLocalizedString>
#include <QFile>

#include "entriesfetchjob.h"
#include "etesync_debug.h"
#include "etesyncresource.h"

using namespace Akonadi;
using namespace KCalendarCore;

CalendarTaskBaseHandler::CalendarTaskBaseHandler(EteSyncResource *resource) : BaseHandler(resource)
{
    initialiseBaseDirectory();
    AttributeFactory::registerAttribute<CollectionColorAttribute>();
}

void CalendarTaskBaseHandler::getItemListFromEntries(std::vector<EteSyncEntryPtr> &entries, Item::List &changedItems, Item::List &removedItems, Collection &collection, const QString &journalUid, QString &prevUid)
{
    const EteSyncJournalPtr &journal = mResource->getJournal(journalUid);
    if (!journal) {
        qCDebug(ETESYNC_LOG) << "SetupItems: Could not get journal";
        mResource->cancelTask(i18n("Could not get journal"));
        return;
    }
    EteSyncCryptoManagerPtr cryptoManager = etesync_journal_get_crypto_manager(journal.get(), mClientState->derived(), mClientState->keypair());

    QMap<QString, KCalendarCore::Incidence::Ptr> incidences;

    for (auto &entry : entries) {
        if (!entry) {
            qCDebug(ETESYNC_LOG) << "SetupItems: Entry is null";
            prevUid = QStringFromCharPtr(CharPtr(etesync_entry_get_uid(entry.get())));
            continue;
        }
        EteSyncSyncEntryPtr syncEntry = etesync_entry_get_sync_entry(entry.get(), cryptoManager.get(), prevUid);

        if (!syncEntry) {
            qCDebug(ETESYNC_LOG) << "SetupItems: syncEntry is null for entry" << etesync_entry_get_uid(entry.get());
            qCDebug(ETESYNC_LOG) << "EteSync error" << etesync_get_error_message();
            prevUid = QStringFromCharPtr(CharPtr(etesync_entry_get_uid(entry.get())));
            continue;
        }

        CharPtr contentStr(etesync_sync_entry_get_content(syncEntry.get()));

        KCalendarCore::ICalFormat format;
        const KCalendarCore::Incidence::Ptr incidence = format.fromString(QStringFromCharPtr(contentStr));

        if (!incidence || (incidence->uid()).isEmpty()) {
            qCDebug(ETESYNC_LOG) << "Couldn't parse entry with uid" << etesync_entry_get_uid(entry.get());
            prevUid = QStringFromCharPtr(CharPtr(etesync_entry_get_uid(entry.get())));
            continue;
        }

        qCDebug(ETESYNC_LOG) << "Entry parsed into incidence - UID" << incidence->uid();

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
                removedItems.push_back(item);

                deleteLocalCalendar(incidence);
            }
        }

        prevUid = QStringFromCharPtr(CharPtr(etesync_entry_get_uid(entry.get())));
    }

    for (auto it = incidences.constBegin(); it != incidences.constEnd(); it++) {
        Item item;
        item.setMimeType(mimeType());
        item.setParentCollection(collection);
        item.setRemoteId(it.key());
        item.setPayload<KCalendarCore::Incidence::Ptr>(it.value());
        changedItems.push_back(item);

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
    if (!item.hasPayload<Incidence::Ptr>()) {
        qCDebug(ETESYNC_LOG) << "Received item with unknown payload";
        mResource->cancelTask(i18n("Received item with unknown payload %1", item.mimeType()));
        return;
    }
    KCalendarCore::Calendar::Ptr calendar(new MemoryCalendar(QTimeZone::utc()));
    const auto incidence = item.payload<Incidence::Ptr>();

    qCDebug(ETESYNC_LOG) << "Incidence mime type" << incidence->mimeType();

    if (!collection.contentMimeTypes().contains(incidence->mimeType())) {
        qCDebug(ETESYNC_LOG) << "Received item of different type";
        mResource->cancelTask(i18n("Received item of different type"));
        return;
    }

    calendar->addIncidence(incidence);
    KCalendarCore::ICalFormat format;

    const QString journalUid = collection.remoteId();
    const EteSyncJournalPtr &journal = mResource->getJournal(journalUid);

    if (!journal) {
        qCDebug(ETESYNC_LOG) << "Could not get journal";
        mResource->cancelTask(i18n("Could not get journal"));
        return;
    }

    EteSyncCryptoManagerPtr cryptoManager = etesync_journal_get_crypto_manager(journal.get(), mClientState->derived(), mClientState->keypair());

    EteSyncSyncEntryPtr syncEntry = etesync_sync_entry_new(QStringLiteral(ETESYNC_SYNC_ENTRY_ACTION_ADD), format.toString(calendar));

    if (!createEteSyncEntry(syncEntry.get(), cryptoManager.get(), collection)) {
        qCDebug(ETESYNC_LOG) << "Could not create EteSync entry";
        mResource->cancelTask(i18n("Could not create EteSync entry"));
        return;
    }

    Item newItem(item);
    newItem.setRemoteId(incidence->uid());
    newItem.setPayload<Incidence::Ptr>(incidence);
    mResource->changeCommitted(newItem);

    updateLocalCalendar(item.payload<Incidence::Ptr>());
}

void CalendarTaskBaseHandler::itemChanged(const Akonadi::Item &item,
                                          const QSet<QByteArray> &parts)
{
    if (!item.hasPayload<Incidence::Ptr>()) {
        qCDebug(ETESYNC_LOG) << "Received item with unknown payload";
        mResource->cancelTask(i18n("Received item with unknown payload %1", item.mimeType()));
        return;
    }
    Collection collection = item.parentCollection();

    KCalendarCore::Calendar::Ptr calendar(new MemoryCalendar(QTimeZone::utc()));
    const auto incidence = item.payload<Incidence::Ptr>();
    calendar->addIncidence(incidence);
    KCalendarCore::ICalFormat format;

    const QString journalUid = collection.remoteId();
    const EteSyncJournalPtr &journal = mResource->getJournal(journalUid);

    if (!journal) {
        qCDebug(ETESYNC_LOG) << "Could not get journal";
        mResource->cancelTask(i18n("Could not get journal"));
        return;
    }

    EteSyncCryptoManagerPtr cryptoManager = etesync_journal_get_crypto_manager(journal.get(), mClientState->derived(), mClientState->keypair());

    EteSyncSyncEntryPtr syncEntry = etesync_sync_entry_new(QStringLiteral(ETESYNC_SYNC_ENTRY_ACTION_CHANGE), format.toString(calendar));

    if (!createEteSyncEntry(syncEntry.get(), cryptoManager.get(), collection)) {
        qCDebug(ETESYNC_LOG) << "Could not create EteSync entry";
        mResource->cancelTask(i18n("Could not create EteSync entry"));
        return;
    }

    Item newItem(item);
    newItem.setPayload<Incidence::Ptr>(incidence);
    mResource->changeCommitted(newItem);

    updateLocalCalendar(item.payload<Incidence::Ptr>());
}

void CalendarTaskBaseHandler::itemRemoved(const Akonadi::Item &item)
{
    Collection collection = item.parentCollection();

    const QString journalUid = collection.remoteId();
    const EteSyncJournalPtr &journal = mResource->getJournal(journalUid);

    if (!journal) {
        mResource->cancelTask();
        return;
    }

    EteSyncCryptoManagerPtr cryptoManager = etesync_journal_get_crypto_manager(journal.get(), mClientState->derived(), mClientState->keypair());

    const QString calendar = getLocalCalendar(item.remoteId());

    if (calendar.isEmpty()) {
        qCDebug(ETESYNC_LOG) << "Could not get local calendar";
        mResource->cancelTask(i18n("Could not get local calendar"));
        return;
    }

    EteSyncSyncEntryPtr syncEntry = etesync_sync_entry_new(QStringLiteral(ETESYNC_SYNC_ENTRY_ACTION_DELETE), calendar);

    if (!createEteSyncEntry(syncEntry.get(), cryptoManager.get(), collection)) {
        qCDebug(ETESYNC_LOG) << "Could not create EteSync entry";
        mResource->cancelTask(i18n("Could not create EteSync entry"));
        return;
    }

    mResource->changeProcessed();
}

void CalendarTaskBaseHandler::collectionAdded(const Akonadi::Collection &collection, const Akonadi::Collection &parent)
{
    const QString journalUid = QStringFromCharPtr(CharPtr(etesync_gen_uid()));
    EteSyncJournalPtr journal = etesync_journal_new(journalUid, ETESYNC_CURRENT_VERSION);

    /// TODO: Description?
    EteSyncCollectionInfoPtr info = etesync_collection_info_new(etesyncCollectionType(), collection.displayName(), QString(), ETESYNC_COLLECTION_DEFAULT_COLOR);

    EteSyncCryptoManagerPtr cryptoManager = etesync_journal_get_crypto_manager(journal.get(), mClientState->derived(), mClientState->keypair());

    if (etesync_journal_set_info(journal.get(), cryptoManager.get(), info.get())) {
        qCDebug(ETESYNC_LOG) << "Could not set journal info";
        qCDebug(ETESYNC_LOG) << "EteSync error" << etesync_get_error_message;
        mResource->cancelTask(i18n("Could not set journal info"));
        return;
    };

    if (etesync_journal_manager_create(mClientState->journalManager(), journal.get())) {
        qCDebug(ETESYNC_LOG) << "Could not create journal";
        qCDebug(ETESYNC_LOG) << "EteSync error" << etesync_get_error_message;
        mResource->handleTokenError();
        return;
    }

    Collection newCollection(collection);
    mResource->setupCollection(newCollection, journal.get());
    mResource->mJournalsCache[newCollection.remoteId()] = std::move(journal);
    mResource->changeCommitted(newCollection);
}

void CalendarTaskBaseHandler::collectionChanged(const Akonadi::Collection &collection)
{
    const QString journalUid = collection.remoteId();
    const EteSyncJournalPtr &journal = mResource->getJournal(journalUid);

    if (!journal) {
        qCDebug(ETESYNC_LOG) << "Could not get journal";
        mResource->cancelTask(i18n("Could not get journal"));
        return;
    }

    auto journalColor = ETESYNC_COLLECTION_DEFAULT_COLOR;
    if (collection.hasAttribute<CollectionColorAttribute>()) {
        const CollectionColorAttribute *colorAttr = collection.attribute<CollectionColorAttribute>();
        if (colorAttr) {
            journalColor = colorAttr->color().rgb();
        }
    }

    EteSyncCollectionInfoPtr info = etesync_collection_info_new(etesyncCollectionType(), collection.displayName(), QString(), journalColor);

    EteSyncCryptoManagerPtr cryptoManager = etesync_journal_get_crypto_manager(journal.get(), mClientState->derived(), mClientState->keypair());

    if (etesync_journal_set_info(journal.get(), cryptoManager.get(), info.get())) {
        qCDebug(ETESYNC_LOG) << "Could not set journal info";
        qCDebug(ETESYNC_LOG) << "EteSync error" << etesync_get_error_message;
        mResource->cancelTask(i18n("Could not set journal info"));
        return;
    };

    if (etesync_journal_manager_update(mClientState->journalManager(), journal.get())) {
        qCDebug(ETESYNC_LOG) << "Could not update journal";
        qCDebug(ETESYNC_LOG) << "EteSync error" << etesync_get_error_message;
        mResource->handleTokenError();
        return;
    }

    Collection newCollection(collection);
    mResource->setupCollection(newCollection, journal.get());
    mResource->changeCommitted(newCollection);
}

void CalendarTaskBaseHandler::collectionRemoved(const Akonadi::Collection &collection)
{
    const QString journalUid = collection.remoteId();
    const EteSyncJournalPtr &journal = mResource->getJournal(journalUid);

    if (!journal) {
        qCDebug(ETESYNC_LOG) << "Could not get journal";
        mResource->cancelTask(i18n("Could not get journal"));
        return;
    }

    if (etesync_journal_manager_delete(mClientState->journalManager(), journal.get())) {
        qCDebug(ETESYNC_LOG) << "Could not delete journal";
        qCDebug(ETESYNC_LOG) << "EteSync error" << etesync_get_error_message;
        mResource->handleTokenError();
        return;
    }
    mResource->changeProcessed();
}
