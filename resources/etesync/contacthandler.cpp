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
#include <QFile>

#include "entriesfetchjob.h"
#include "etesync_debug.h"
#include "etesyncresource.h"

using namespace Akonadi;

ContactHandler::ContactHandler(EteSyncResource *resource)
    : mResource(resource), mClientState(resource->mClientState)
{
    mResource->initialiseDirectory(baseDirectoryPath());
}

void ContactHandler::setupItems(EteSyncEntry **entries, Akonadi::Collection &collection)
{
    qCDebug(ETESYNC_LOG) << "Setting up items";
    QString prevUid = collection.remoteRevision();
    QString journalUid = collection.remoteId();

    bool isIncremental = false;

    if (!prevUid.isEmpty() && !prevUid.isNull()) {
        isIncremental = true;
    }

    EteSyncJournalPtr journal(etesync_journal_manager_fetch(mClientState->journalManager(), journalUid));
    EteSyncCryptoManagerPtr cryptoManager(etesync_journal_get_crypto_manager(journal.get(), mClientState->derived(), mClientState->keypair()));

    QMap<QString, KContacts::Addressee> contacts;

    Item::List changedItems;
    Item::List removedItems;

    for (EteSyncEntry **iter = entries; *iter; iter++) {
        EteSyncEntryPtr entry(*iter);
        EteSyncSyncEntryPtr syncEntry(etesync_entry_get_sync_entry(entry.get(), cryptoManager.get(), prevUid));

        KContacts::VCardConverter converter;
        CharPtr contentStr(etesync_sync_entry_get_content(syncEntry.get()));
        QByteArray content(contentStr.get());
        const KContacts::Addressee contact = converter.parseVCard(content);

        const QString action = QStringFromCharPtr(CharPtr(etesync_sync_entry_get_action(syncEntry.get())));
        if (action == QStringLiteral(ETESYNC_SYNC_ENTRY_ACTION_ADD) || action == QStringLiteral(ETESYNC_SYNC_ENTRY_ACTION_CHANGE)) {
            contacts[contact.uid()] = contact;
        } else if (action == QStringLiteral(ETESYNC_SYNC_ENTRY_ACTION_DELETE)) {
            if (contacts.contains(contact.uid())) {
                contacts.remove(contact.uid());
            } else {
                Item item;
                item.setMimeType(KContacts::Addressee::mimeType());
                item.setParentCollection(collection);
                item.setRemoteId(contact.uid());
                item.setPayload<KContacts::Addressee>(contact);
                removedItems << item;

                deleteLocalContact(contact);
            }
        }

        prevUid = QStringFromCharPtr(CharPtr(etesync_entry_get_uid(entry.get())));
    }

    free(entries);

    collection.setRemoteRevision(prevUid);
    new CollectionModifyJob(collection, this);

    for (auto it = contacts.constBegin(); it != contacts.constEnd(); it++) {
        Item item;
        item.setMimeType(KContacts::Addressee::mimeType());
        item.setParentCollection(collection);
        item.setRemoteId(it.key());
        item.setPayload<KContacts::Addressee>(it.value());
        changedItems << item;

        updateLocalContact(it.value());
    }

    if (isIncremental) {
        mResource->itemsRetrievedIncremental(changedItems, removedItems);
    } else {
        mResource->itemsRetrieved(changedItems);
    }
}

QString ContactHandler::baseDirectoryPath() const
{
    return mResource->baseDirectoryPath() + QStringLiteral("/Contacts");
}

QString ContactHandler::getLocalContact(QString contactUid) const
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

void ContactHandler::deleteLocalContact(const KContacts::Addressee &contact)
{
    const QString path = baseDirectoryPath() + QLatin1Char('/') + contact.uid() + QLatin1String(".vcf");
    QFile file(path);
    if (!file.remove()) {
        qCDebug(ETESYNC_LOG) << "Unable to remove " << path << file.errorString();
        return;
    }
}

void ContactHandler::itemAdded(const Akonadi::Item &item,
                               const Akonadi::Collection &collection)
{
    EteSyncJournalPtr journal(etesync_journal_manager_fetch(mClientState->journalManager(), collection.remoteId()));
    EteSyncCryptoManagerPtr cryptoManager(etesync_journal_get_crypto_manager(journal.get(), mClientState->derived(), mClientState->keypair()));

    KContacts::VCardConverter converter;
    QByteArray content = converter.createVCard(item.payload<KContacts::Addressee>());

    EteSyncSyncEntryPtr syncEntry(etesync_sync_entry_new(ETESYNC_SYNC_ENTRY_ACTION_ADD, content.constData()));
    EteSyncEntryPtr entry(etesync_entry_from_sync_entry(cryptoManager.get(), syncEntry.get(), collection.remoteRevision()));

    EteSyncEntryManagerPtr entryManager(etesync_entry_manager_new(mClientState->client(), collection.remoteId()));

    EteSyncEntry *entries[] = {entry.get(), NULL};

    etesync_entry_manager_create(entryManager.get(), entries, collection.remoteRevision());

    mResource->changeCommitted(item);

    updateLocalContact(item.payload<KContacts::Addressee>());

    // Update last UID in collection remoteRevision
    const QString entryUid = QStringFromCharPtr(CharPtr(etesync_entry_get_uid(entry.get())));
    Collection col = collection;
    col.setRemoteRevision(entryUid);
    new CollectionModifyJob(col, this);
}

void ContactHandler::itemChanged(const Akonadi::Item &item,
                                 const QSet<QByteArray> &parts)
{
    Collection collection = item.parentCollection();

    EteSyncJournalPtr journal(etesync_journal_manager_fetch(mClientState->journalManager(), collection.remoteId()));
    EteSyncCryptoManagerPtr cryptoManager(etesync_journal_get_crypto_manager(journal.get(), mClientState->derived(), mClientState->keypair()));

    KContacts::VCardConverter converter;
    QByteArray content = converter.createVCard(item.payload<KContacts::Addressee>());

    EteSyncSyncEntryPtr syncEntry(etesync_sync_entry_new(ETESYNC_SYNC_ENTRY_ACTION_CHANGE, content.constData()));
    EteSyncEntryPtr entry(etesync_entry_from_sync_entry(cryptoManager.get(), syncEntry.get(), collection.remoteRevision()));

    EteSyncEntryManagerPtr entryManager(etesync_entry_manager_new(mClientState->client(), collection.remoteId()));

    EteSyncEntry *entries[] = {entry.get(), NULL};

    etesync_entry_manager_create(entryManager.get(), entries, collection.remoteRevision());

    mResource->changeCommitted(item);

    updateLocalContact(item.payload<KContacts::Addressee>());

    // Update last UID in collection remoteRevision
    const QString entryUid = QStringFromCharPtr(CharPtr(etesync_entry_get_uid(entry.get())));
    collection.setRemoteRevision(entryUid);
    new CollectionModifyJob(collection, this);
}

void ContactHandler::itemRemoved(const Akonadi::Item &item)
{
    Collection collection = item.parentCollection();

    EteSyncJournalPtr journal(etesync_journal_manager_fetch(mClientState->journalManager(), collection.remoteId()));
    EteSyncCryptoManagerPtr cryptoManager(etesync_journal_get_crypto_manager(journal.get(), mClientState->derived(), mClientState->keypair()));

    QString contact = getLocalContact(item.remoteId());

    EteSyncSyncEntryPtr syncEntry(etesync_sync_entry_new(ETESYNC_SYNC_ENTRY_ACTION_DELETE, charArrFromQString(contact)));
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

void ContactHandler::collectionAdded(const Akonadi::Collection &collection, const Akonadi::Collection &parent)
{
    QString journalUid = QStringFromCharPtr(CharPtr(etesync_gen_uid()));
    EteSyncJournalPtr journal(etesync_journal_new(journalUid, ETESYNC_CURRENT_VERSION));

    EteSyncCollectionInfoPtr info(etesync_collection_info_new(QStringLiteral(ETESYNC_COLLECTION_TYPE_ADDRESS_BOOK), collection.displayName(), QString(), EteSyncDEFAULT_COLOR));

    EteSyncCryptoManagerPtr cryptoManager(etesync_journal_get_crypto_manager(journal.get(), mClientState->derived(), mClientState->keypair()));

    etesync_journal_set_info(journal.get(), cryptoManager.get(), info.get());

    etesync_journal_manager_create(mClientState->journalManager(), journal.get());

    Collection newCollection(collection);
    mResource->setupCollection(newCollection, journal.get());
    mResource->changeCommitted(newCollection);
}

void ContactHandler::collectionChanged(const Akonadi::Collection &collection)
{
    QString journalUid = collection.remoteId();
    EteSyncJournalPtr journal(etesync_journal_manager_fetch(mClientState->journalManager(), journalUid));

    EteSyncCollectionInfoPtr info(etesync_collection_info_new(QStringLiteral(ETESYNC_COLLECTION_TYPE_ADDRESS_BOOK), collection.displayName(), QString(), EteSyncDEFAULT_COLOR));

    EteSyncCryptoManagerPtr cryptoManager(etesync_journal_get_crypto_manager(journal.get(), mClientState->derived(), mClientState->keypair()));

    etesync_journal_set_info(journal.get(), cryptoManager.get(), info.get());

    etesync_journal_manager_update(mClientState->journalManager(), journal.get());
}

void ContactHandler::collectionRemoved(const Akonadi::Collection &collection)
{
    QString journalUid = collection.remoteId();
    EteSyncJournalPtr journal(etesync_journal_manager_fetch(mClientState->journalManager(), journalUid));

    etesync_journal_manager_delete(mClientState->journalManager(), journal.get());
}
