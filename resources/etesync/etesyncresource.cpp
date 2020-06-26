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

#include "etesyncresource.h"

#include <AkonadiCore/ChangeRecorder>
#include <AkonadiCore/CollectionFetchScope>
#include <AkonadiCore/CollectionModifyJob>
#include <AkonadiCore/EntityDisplayAttribute>
#include <AkonadiCore/ItemFetchJob>
#include <AkonadiCore/ItemFetchScope>
#include <KLocalizedString>
#include <QDBusConnection>

#include "entriesfetchjob.h"
#include "etesync_debug.h"
#include "etesyncadapter.h"
#include "journalsfetchjob.h"
#include "settings.h"
#include "settingsadaptor.h"

using namespace EteSyncAPI;
using namespace Akonadi;

#define ROOT_COLLECTION_REMOTEID QStringLiteral("EteSyncRootCollection")

EteSyncResource::EteSyncResource(const QString &id)
    : ResourceBase(id)
{
    Settings::instance(KSharedConfig::openConfig());
    new SettingsAdaptor(Settings::self());

    QDBusConnection::sessionBus().registerObject(QStringLiteral("/Settings"),
                                                 Settings::self(),
                                                 QDBusConnection::ExportAdaptors);

    setName(i18n("EteSync Resource"));

    setNeedsNetwork(true);

    changeRecorder()->itemFetchScope().fetchFullPayload(true);
    changeRecorder()->itemFetchScope().setAncestorRetrieval(ItemFetchScope::All);
    changeRecorder()->fetchCollection(true);
    changeRecorder()->collectionFetchScope().setAncestorRetrieval(CollectionFetchScope::All);

    // Make local contacts directory
    initialiseDirectory(baseDirectoryPath());

    mClientState = new EteSyncClientState();

    connect(this, &Akonadi::AgentBase::reloadConfiguration, this, &EteSyncResource::onReloadConfiguration);

    qCDebug(ETESYNC_LOG) << "Resource started";
}

EteSyncResource::~EteSyncResource()
{
}

void EteSyncResource::retrieveCollections()
{
    // Set up root collection for resource
    mResourceCollection = Collection();
    mResourceCollection.setContentMimeTypes({Collection::mimeType(), Collection::virtualMimeType()});
    mResourceCollection.setName(mClientState->username());
    mResourceCollection.setRemoteId(ROOT_COLLECTION_REMOTEID);
    mResourceCollection.setParentCollection(Collection::root());
    mResourceCollection.setRights(Collection::CanCreateCollection);

    EntityDisplayAttribute *attr = mResourceCollection.attribute<EntityDisplayAttribute>(Collection::AddIfMissing);
    attr->setDisplayName(mClientState->username());
    attr->setIconName(QStringLiteral("akonadi-etesync"));

    auto job = new JournalsFetchJob(mClientState->client(), this);
    connect(job, &JournalsFetchJob::finished, this, [this](KJob *job) {
        if (job->error()) {
            qCWarning(ETESYNC_LOG) << "Error in fetching journals";
            return;
        }
        qCDebug(ETESYNC_LOG) << "Retrieving collections";
        EteSyncJournal **journals = qobject_cast<JournalsFetchJob *>(job)->journals();
        Collection::List list;
        list << mResourceCollection;
        for (EteSyncJournal **iter = journals; *iter; iter++) {
            EteSyncJournalPtr journal(*iter);

            Collection collection;

            /// TODO: Remove temporary hack to handle only addressbooks
            if (setupCollection(collection, journal.get())) {
                continue;
            }

            list << collection;
        }
        free(journals);
        collectionsRetrieved(list);
    });
    job->start();
}

/// TODO: Remove temporary hack, make this void
int EteSyncResource::setupCollection(Collection &collection, EteSyncJournal *journal)
{
    int typeNum;
    EteSyncCryptoManagerPtr cryptoManager(etesync_journal_get_crypto_manager(journal, mClientState->derived(), mClientState->keypair()));

    EteSyncCollectionInfoPtr info(etesync_journal_get_info(journal, cryptoManager.get()));

    const QString type = QStringFromCharPtr(CharPtr(etesync_collection_info_get_type(info.get())));

    QStringList mimeTypes;

    if (type == QStringLiteral(ETESYNC_COLLECTION_TYPE_ADDRESS_BOOK)) {
        mimeTypes << KContacts::Addressee::mimeType();
        typeNum = 0;
    } else if (type == QStringLiteral(ETESYNC_COLLECTION_TYPE_CALENDAR)) {
        mimeTypes << KContacts::Addressee::mimeType();
        typeNum = 1;
    } else if (type == QStringLiteral(ETESYNC_COLLECTION_TYPE_TASKS)) {
        mimeTypes << KContacts::Addressee::mimeType();
        typeNum = 2;
    } else {
        qCWarning(ETESYNC_LOG) << "Unknown journal type. Cannot set collection mime type.";
    }

    const QString journalUid = QStringFromCharPtr(CharPtr(etesync_journal_get_uid(journal)));
    const QString displayName = QStringFromCharPtr(CharPtr(etesync_collection_info_get_display_name(info.get())));

    collection.setParentCollection(mResourceCollection);
    collection.setRemoteId(journalUid);
    collection.setName(displayName);
    collection.setContentMimeTypes(mimeTypes);

    EntityDisplayAttribute *attr = collection.attribute<EntityDisplayAttribute>(Collection::AddIfMissing);
    attr->setIconName(QStringLiteral("view-pim-contacts"));

    return typeNum;
}

void EteSyncResource::retrieveItems(const Akonadi::Collection &collection)
{
    QString journalUid = collection.remoteId();
    QString prevUid = collection.remoteRevision();

    auto job = new EntriesFetchJob(mClientState->client(), journalUid, prevUid, this);

    connect(job, &EntriesFetchJob::finished, this, [this](KJob *job) {
        if (job->error()) {
            qCWarning(ETESYNC_LOG) << job->errorText();
            return;
        }

        Collection collection = currentCollection();
        QString prevUid = collection.remoteRevision();
        QString journalUid = collection.remoteId();

        bool isIncremental = false;

        if (!prevUid.isEmpty() && !prevUid.isNull()) {
            isIncremental = true;
        }

        qCDebug(ETESYNC_LOG) << "Retrieving entries";
        EteSyncEntry **entries = qobject_cast<EntriesFetchJob *>(job)->entries();

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
            // qCDebug(ETESYNC_LOG) << action;
            // qCDebug(ETESYNC_LOG) << contact.uid();
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
            itemsRetrievedIncremental(changedItems, removedItems);
        } else {
            itemsRetrieved(changedItems);
        }
    });

    job->start();
}

void EteSyncResource::aboutToQuit()
{
    delete mClientState;
}

void EteSyncResource::onReloadConfiguration()
{
    qCDebug(ETESYNC_LOG) << "Resource config reload";

    connect(mClientState, &EteSyncClientState::clientInitialised, this, [this](bool successful) {
        qCDebug(ETESYNC_LOG) << "Resource intialised";
        if (successful) {
            synchronize();
        }
    });
    mClientState->init();
}

QString EteSyncResource::baseDirectoryPath() const
{
    return Settings::self()->contactsPath();
}

QString EteSyncResource::getLocalContact(QString contactUid) const
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

void EteSyncResource::updateLocalContact(const KContacts::Addressee &contact)
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

void EteSyncResource::deleteLocalContact(const KContacts::Addressee &contact)
{
    const QString path = baseDirectoryPath() + QLatin1Char('/') + contact.uid() + QLatin1String(".vcf");
    QFile file(path);
    if (!file.remove()) {
        qCDebug(ETESYNC_LOG) << "Unable to remove " << path << file.errorString();
        return;
    }
}

void EteSyncResource::initialiseDirectory(const QString &path) const
{
    QDir dir(path);

    // if folder does not exists, create it
    QDir::root().mkpath(dir.absolutePath());

    // check whether warning file is in place...
    QFile file(dir.absolutePath() + QStringLiteral("/WARNING_README.txt"));
    if (!file.exists()) {
        // ... if not, create it
        file.open(QIODevice::WriteOnly);
        file.write(
            "Important warning!\n\n"
            "Do not create or copy vCards inside this folder manually, they are managed by the Akonadi framework!\n");
        file.close();
    }
}

void EteSyncResource::itemAdded(const Akonadi::Item &item,
                                const Akonadi::Collection &collection)
{
    qCDebug(ETESYNC_LOG) << "Item added";

    EteSyncJournalPtr journal(etesync_journal_manager_fetch(mClientState->journalManager(), collection.remoteId()));
    EteSyncCryptoManagerPtr cryptoManager(etesync_journal_get_crypto_manager(journal.get(), mClientState->derived(), mClientState->keypair()));

    KContacts::VCardConverter converter;
    QByteArray content = converter.createVCard(item.payload<KContacts::Addressee>());

    EteSyncSyncEntryPtr syncEntry(etesync_sync_entry_new(ETESYNC_SYNC_ENTRY_ACTION_ADD, content.constData()));
    EteSyncEntryPtr entry(etesync_entry_from_sync_entry(cryptoManager.get(), syncEntry.get(), collection.remoteRevision()));

    EteSyncEntryManagerPtr entryManager(etesync_entry_manager_new(mClientState->client(), collection.remoteId()));

    EteSyncEntry *entries[] = {entry.get(), NULL};

    etesync_entry_manager_create(entryManager.get(), entries, collection.remoteRevision());

    changeCommitted(item);

    updateLocalContact(item.payload<KContacts::Addressee>());

    // Update last UID in collection remoteRevision
    const QString entryUid = QStringFromCharPtr(CharPtr(etesync_entry_get_uid(entry.get())));
    Collection col = collection;
    col.setRemoteRevision(entryUid);
    new CollectionModifyJob(col, this);
}

void EteSyncResource::itemChanged(const Akonadi::Item &item,
                                  const QSet<QByteArray> &parts)
{
    qCDebug(ETESYNC_LOG) << "Item changed";

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

    changeCommitted(item);

    updateLocalContact(item.payload<KContacts::Addressee>());

    // Update last UID in collection remoteRevision
    const QString entryUid = QStringFromCharPtr(CharPtr(etesync_entry_get_uid(entry.get())));
    collection.setRemoteRevision(entryUid);
    new CollectionModifyJob(collection, this);
}

void EteSyncResource::itemRemoved(const Akonadi::Item &item)
{
    qCDebug(ETESYNC_LOG) << "Item removed" << item.remoteId();

    Collection collection = item.parentCollection();

    EteSyncJournalPtr journal(etesync_journal_manager_fetch(mClientState->journalManager(), collection.remoteId()));
    EteSyncCryptoManagerPtr cryptoManager(etesync_journal_get_crypto_manager(journal.get(), mClientState->derived(), mClientState->keypair()));

    QString contact = getLocalContact(item.remoteId());

    EteSyncSyncEntryPtr syncEntry(etesync_sync_entry_new(ETESYNC_SYNC_ENTRY_ACTION_DELETE, charArrFromQString(contact)));
    EteSyncEntryPtr entry(etesync_entry_from_sync_entry(cryptoManager.get(), syncEntry.get(), collection.remoteRevision()));

    EteSyncEntryManagerPtr entryManager(etesync_entry_manager_new(mClientState->client(), collection.remoteId()));

    EteSyncEntry *entries[] = {entry.get(), NULL};

    etesync_entry_manager_create(entryManager.get(), entries, collection.remoteRevision());

    changeCommitted(item);

    // Update last UID in collection remoteRevision
    const QString entryUid = QStringFromCharPtr(CharPtr(etesync_entry_get_uid(entry.get())));
    collection.setRemoteRevision(entryUid);
    new CollectionModifyJob(collection, this);
}

void EteSyncResource::collectionAdded(const Akonadi::Collection &collection, const Akonadi::Collection &parent)
{
    qCDebug(ETESYNC_LOG) << "Collection added" << collection.mimeType();

    QString journalUid = QStringFromCharPtr(CharPtr(etesync_gen_uid()));
    EteSyncJournalPtr journal(etesync_journal_new(journalUid, ETESYNC_CURRENT_VERSION));

    EteSyncCollectionInfoPtr info(etesync_collection_info_new(QStringLiteral(ETESYNC_COLLECTION_TYPE_ADDRESS_BOOK), collection.displayName(), QString(), EteSyncDEFAULT_COLOR));

    EteSyncCryptoManagerPtr cryptoManager(etesync_journal_get_crypto_manager(journal.get(), mClientState->derived(), mClientState->keypair()));

    etesync_journal_set_info(journal.get(), cryptoManager.get(), info.get());

    etesync_journal_manager_create(mClientState->journalManager(), journal.get());

    Collection newCollection(collection);
    newCollection.setRemoteId(journalUid);
    changeCommitted(newCollection);
}

void EteSyncResource::collectionChanged(const Akonadi::Collection &collection)
{
    qCDebug(ETESYNC_LOG) << "Collection changed" << collection.mimeType();

    QString journalUid = collection.remoteId();
    EteSyncJournalPtr journal(etesync_journal_manager_fetch(mClientState->journalManager(), journalUid));

    EteSyncCollectionInfoPtr info(etesync_collection_info_new(QStringLiteral(ETESYNC_COLLECTION_TYPE_ADDRESS_BOOK), collection.displayName(), QString(), EteSyncDEFAULT_COLOR));

    EteSyncCryptoManagerPtr cryptoManager(etesync_journal_get_crypto_manager(journal.get(), mClientState->derived(), mClientState->keypair()));

    etesync_journal_set_info(journal.get(), cryptoManager.get(), info.get());

    etesync_journal_manager_update(mClientState->journalManager(), journal.get());

    Collection newCollection(collection);
    newCollection.setRemoteId(journalUid);
    changeCommitted(newCollection);
}

void EteSyncResource::collectionRemoved(const Akonadi::Collection &collection)
{
    qCDebug(ETESYNC_LOG) << "Collection removed" << collection.mimeType();

    QString journalUid = collection.remoteId();
    EteSyncJournalPtr journal(etesync_journal_manager_fetch(mClientState->journalManager(), journalUid));

    etesync_journal_manager_delete(mClientState->journalManager(), journal.get());
}

AKONADI_RESOURCE_MAIN(EteSyncResource)
