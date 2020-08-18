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

#include "contacthandler.h"

#include <kcontacts/vcardconverter.h>

#include <AkonadiCore/CollectionModifyJob>
#include <KLocalizedString>
#include <QFile>

#include "entriesfetchjob.h"
#include "etesync_debug.h"
#include "etesyncresource.h"

using namespace Akonadi;

ContactHandler::ContactHandler(EteSyncResource *resource) : BaseHandler(resource)
{
    initialiseBaseDirectory();
}

const QString ContactHandler::mimeType()
{
    return KContacts::Addressee::mimeType();
}

const QString ContactHandler::etesyncCollectionType()
{
    return QStringLiteral(ETESYNC_COLLECTION_TYPE_ADDRESS_BOOK);
}

void ContactHandler::getItemListFromEntries(std::vector<EteSyncEntryPtr> &entries, Item::List &changedItems, Item::List &removedItems, Collection &collection, const QString &journalUid, QString &prevUid)
{
    const EteSyncJournalPtr &journal = mResource->getJournal(journalUid);
    if (!journal) {
        qCDebug(ETESYNC_LOG) << "SetupItems: Could not get journal";
        mResource->cancelTask(i18n("Could not get journal"));
        return;
    }
    EteSyncCryptoManagerPtr cryptoManager = etesync_journal_get_crypto_manager(journal.get(), mClientState->derived(), mClientState->keypair());

    QMap<QString, KContacts::Addressee> contacts;

    for (auto &entry : entries) {
        if (!entry) {
            qCDebug(ETESYNC_LOG) << "SetupItems: Entry is null";
            prevUid = QStringFromCharPtr(CharPtr(etesync_entry_get_uid(entry.get())));
            continue;
        }
        EteSyncSyncEntryPtr syncEntry = etesync_entry_get_sync_entry(entry.get(), cryptoManager.get(), prevUid);

        if (!syncEntry) {
            qCDebug(ETESYNC_LOG) << "SetupItems: syncEntry is null for entry" << etesync_entry_get_uid(entry.get());
            qCDebug(ETESYNC_LOG) << "EteSync error" << QStringFromCharPtr(CharPtr(etesync_get_error_message()));
            prevUid = QStringFromCharPtr(CharPtr(etesync_entry_get_uid(entry.get())));
            continue;
        }

        KContacts::VCardConverter converter;
        CharPtr contentStr(etesync_sync_entry_get_content(syncEntry.get()));
        QByteArray content(contentStr.get());
        const KContacts::Addressee contact = converter.parseVCard(content);

        if ((contact.uid()).isEmpty()) {
            qCDebug(ETESYNC_LOG) << "Couldn't parse entry with uid" << etesync_entry_get_uid(entry.get());
            prevUid = QStringFromCharPtr(CharPtr(etesync_entry_get_uid(entry.get())));
            continue;
        }

        qCDebug(ETESYNC_LOG) << "Entry parsed into contact - UID" << contact.uid();

        const QString action = QStringFromCharPtr(CharPtr(etesync_sync_entry_get_action(syncEntry.get())));
        if (action == QStringLiteral(ETESYNC_SYNC_ENTRY_ACTION_ADD) || action == QStringLiteral(ETESYNC_SYNC_ENTRY_ACTION_CHANGE)) {
            contacts[contact.uid()] = contact;
        } else if (action == QStringLiteral(ETESYNC_SYNC_ENTRY_ACTION_DELETE)) {
            if (contacts.contains(contact.uid())) {
                contacts.remove(contact.uid());
            } else {
                Item item;
                item.setMimeType(mimeType());
                item.setParentCollection(collection);
                item.setRemoteId(contact.uid());
                removedItems.push_back(item);

                deleteLocalContact(contact.uid());
            }
        }

        prevUid = QStringFromCharPtr(CharPtr(etesync_entry_get_uid(entry.get())));
    }

    for (auto it = contacts.constBegin(); it != contacts.constEnd(); it++) {
        Item item;
        item.setMimeType(mimeType());
        item.setParentCollection(collection);
        item.setRemoteId(it.key());
        item.setPayload<KContacts::Addressee>(it.value());
        changedItems.push_back(item);

        updateLocalContact(it.value());
    }
}

QString ContactHandler::baseDirectoryPath() const
{
    return mResource->baseDirectoryPath() + QStringLiteral("/Contacts");
}

QString ContactHandler::getLocalContact(const QString &contactUid) const
{
    const QString path = baseDirectoryPath() + QLatin1Char('/') + contactUid + QLatin1String(".vcf");

    QFile file(path);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        qCDebug(ETESYNC_LOG) << "Unable to read " << path << file.errorString();
        return QString();
    }
    QTextStream in(&file);
    return in.readAll();
}

void ContactHandler::updateLocalContact(const KContacts::Addressee &contact)
{
    const QString path = baseDirectoryPath() + QLatin1Char('/') + contact.uid() + QLatin1String(".vcf");

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        qCDebug(ETESYNC_LOG) << "Unable to open " << path << file.errorString();
        return;
    }
    KContacts::VCardConverter converter;
    file.write(converter.createVCard(contact));
}

void ContactHandler::deleteLocalContact(const QString &contactUid)
{
    const QString path = baseDirectoryPath() + QLatin1Char('/') + contactUid + QLatin1String(".vcf");
    QFile file(path);
    if (!file.remove()) {
        qCDebug(ETESYNC_LOG) << "Unable to remove " << path << file.errorString();
        return;
    }
}

void ContactHandler::itemAdded(const Akonadi::Item &item,
                               const Akonadi::Collection &collection)
{
    if (!item.hasPayload<KContacts::Addressee>()) {
        qCDebug(ETESYNC_LOG) << "Received item with unknown payload";
        mResource->cancelTask(i18n("Received item with unknown payload %1", item.mimeType()));
        return;
    }
    const QString journalUid = collection.remoteId();
    const EteSyncJournalPtr &journal = mResource->getJournal(journalUid);

    if (!journal) {
        qCDebug(ETESYNC_LOG) << "Could not get journal";
        mResource->cancelTask(i18n("Could not get journal"));
        return;
    }

    EteSyncCryptoManagerPtr cryptoManager = etesync_journal_get_crypto_manager(journal.get(), mClientState->derived(), mClientState->keypair());

    KContacts::VCardConverter converter;
    const auto contact = item.payload<KContacts::Addressee>();
    QByteArray content = converter.createVCard(contact);

    if (content.isEmpty()) {
        qCDebug(ETESYNC_LOG) << "Could not create vcard from payload";
        mResource->cancelTask(i18n("Could not create vcard from payload"));
        return;
    }

    EteSyncSyncEntryPtr syncEntry = etesync_sync_entry_new(QStringLiteral(ETESYNC_SYNC_ENTRY_ACTION_ADD), QString::fromUtf8(content));

    if (!createEteSyncEntry(syncEntry.get(), cryptoManager.get(), collection)) {
        qCDebug(ETESYNC_LOG) << "Could not create EteSync entry";
        mResource->cancelTask(i18n("Could not create EteSync entry"));
        return;
    }

    Item newItem(item);
    newItem.setRemoteId(contact.uid());
    newItem.setPayload<KContacts::Addressee>(contact);
    mResource->changeCommitted(newItem);

    updateLocalContact(item.payload<KContacts::Addressee>());
}

void ContactHandler::itemChanged(const Akonadi::Item &item,
                                 const QSet<QByteArray> &parts)
{
    if (!item.hasPayload<KContacts::Addressee>()) {
        qCDebug(ETESYNC_LOG) << "Received item with unknown payload";
        mResource->cancelTask(i18n("Received item with unknown payload %1", item.mimeType()));
        return;
    }
    Collection collection = item.parentCollection();

    const QString journalUid = collection.remoteId();
    const EteSyncJournalPtr &journal = mResource->getJournal(journalUid);

    if (!journal) {
        qCDebug(ETESYNC_LOG) << "Could not get journal";
        mResource->cancelTask(i18n("Could not get journal"));
        return;
    }

    EteSyncCryptoManagerPtr cryptoManager = etesync_journal_get_crypto_manager(journal.get(), mClientState->derived(), mClientState->keypair());

    KContacts::VCardConverter converter;
    const auto contact = item.payload<KContacts::Addressee>();
    QByteArray content = converter.createVCard(contact);

    if (content.isEmpty()) {
        qCDebug(ETESYNC_LOG) << "Could not create vcard from content";
        mResource->cancelTask(i18n("Could not create vcard from content"));
        return;
    }

    EteSyncSyncEntryPtr syncEntry = etesync_sync_entry_new(QStringLiteral(ETESYNC_SYNC_ENTRY_ACTION_CHANGE), QString::fromUtf8(content));

    if (!createEteSyncEntry(syncEntry.get(), cryptoManager.get(), collection)) {
        qCDebug(ETESYNC_LOG) << "Could not create EteSync entry";
        mResource->cancelTask(i18n("Could not create EteSync entry"));
        return;
    }

    // Using ItemModifyJob + changeProcessed() instead of changeCommitted to handle conflict error - ItemSync modifies local item payload
    Item newItem(item);
    newItem.setPayload<KContacts::Addressee>(contact);
    Akonadi::ItemModifyJob *modifyJob = new Akonadi::ItemModifyJob(newItem);
    modifyJob->disableRevisionCheck();
    mResource->changeProcessed();

    updateLocalContact(item.payload<KContacts::Addressee>());
}

void ContactHandler::itemRemoved(const Akonadi::Item &item)
{
    Collection collection = item.parentCollection();

    const QString contact = getLocalContact(item.remoteId());

    if (contact.isEmpty()) {
        qCDebug(ETESYNC_LOG) << "Could not get local contact";
        mResource->cancelTask(i18n("Could not get local contact"));
        return;
    }

    // Delete now, because itemRemoved() may be called when collection is removed
    deleteLocalContact(item.remoteId());

    const QString journalUid = collection.remoteId();
    const EteSyncJournalPtr &journal = mResource->getJournal(journalUid);

    if (!journal) {
        mResource->cancelTask();
        return;
    }

    EteSyncCryptoManagerPtr cryptoManager = etesync_journal_get_crypto_manager(journal.get(), mClientState->derived(), mClientState->keypair());

    EteSyncSyncEntryPtr syncEntry = etesync_sync_entry_new(QStringLiteral(ETESYNC_SYNC_ENTRY_ACTION_DELETE), contact);

    if (!createEteSyncEntry(syncEntry.get(), cryptoManager.get(), collection)) {
        qCDebug(ETESYNC_LOG) << "Could not create EteSync entry";
        mResource->cancelTask(i18n("Could not create EteSync entry"));
        return;
    }

    mResource->changeProcessed();
}

void ContactHandler::collectionAdded(const Akonadi::Collection &collection, const Akonadi::Collection &parent)
{
    const QString journalUid = QStringFromCharPtr(CharPtr(etesync_gen_uid()));
    EteSyncJournalPtr journal = etesync_journal_new(journalUid, ETESYNC_CURRENT_VERSION);

    EteSyncCollectionInfoPtr info = etesync_collection_info_new(etesyncCollectionType(), collection.displayName(), QString(), ETESYNC_COLLECTION_DEFAULT_COLOR);

    EteSyncCryptoManagerPtr cryptoManager = etesync_journal_get_crypto_manager(journal.get(), mClientState->derived(), mClientState->keypair());

    if (etesync_journal_set_info(journal.get(), cryptoManager.get(), info.get())) {
        qCDebug(ETESYNC_LOG) << "Could not set journal info";
        qCDebug(ETESYNC_LOG) << "EteSync error" << QStringFromCharPtr(CharPtr(etesync_get_error_message()));
        mResource->cancelTask(i18n("Could not set journal info"));
        return;
    };

    if (etesync_journal_manager_create(mClientState->journalManager(), journal.get())) {
        qCDebug(ETESYNC_LOG) << "Could not create journal";
        qCDebug(ETESYNC_LOG) << "EteSync error" << QStringFromCharPtr(CharPtr(etesync_get_error_message()));
        mResource->handleTokenError();
        return;
    }

    Collection newCollection(collection);
    mResource->setupCollection(newCollection, journal.get());
    mResource->mJournalsCache[newCollection.remoteId()] = std::move(journal);
    mResource->changeCommitted(newCollection);
}

void ContactHandler::collectionChanged(const Akonadi::Collection &collection)
{
    const QString journalUid = collection.remoteId();
    const EteSyncJournalPtr &journal = mResource->getJournal(journalUid);

    if (!journal) {
        qCDebug(ETESYNC_LOG) << "Could not get journal";
        mResource->cancelTask(i18n("Could not get journal"));
        return;
    }

    EteSyncCollectionInfoPtr info = etesync_collection_info_new(etesyncCollectionType(), collection.displayName(), QString(), EteSyncDEFAULT_COLOR);

    EteSyncCryptoManagerPtr cryptoManager = etesync_journal_get_crypto_manager(journal.get(), mClientState->derived(), mClientState->keypair());

    if (etesync_journal_set_info(journal.get(), cryptoManager.get(), info.get())) {
        qCDebug(ETESYNC_LOG) << "Could not set journal info";
        qCDebug(ETESYNC_LOG) << "EteSync error" << QStringFromCharPtr(CharPtr(etesync_get_error_message()));
        mResource->cancelTask(i18n("Could not set journal info"));
        return;
    };

    if (etesync_journal_manager_update(mClientState->journalManager(), journal.get())) {
        qCDebug(ETESYNC_LOG) << "Could not update journal";
        qCDebug(ETESYNC_LOG) << "EteSync error" << QStringFromCharPtr(CharPtr(etesync_get_error_message()));
        mResource->handleTokenError();
        return;
    }

    Collection newCollection(collection);
    mResource->setupCollection(newCollection, journal.get());
    mResource->changeCommitted(newCollection);
}

void ContactHandler::collectionRemoved(const Akonadi::Collection &collection)
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
        qCDebug(ETESYNC_LOG) << "EteSync error" << QStringFromCharPtr(CharPtr(etesync_get_error_message()));
        mResource->handleTokenError();
        return;
    }
    mResource->changeProcessed();
}
