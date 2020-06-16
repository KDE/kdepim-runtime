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

#include <AkonadiCore/CollectionModifyJob>
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

    serverUrl = Settings::self()->baseUrl();
    username = Settings::self()->username();
    password = Settings::self()->password();
    encryptionPassword = Settings::self()->encryptionPassword();

    connect(this, &Akonadi::AgentBase::reloadConfiguration, this, &EteSyncResource::onReloadConfiguration);

    qCDebug(ETESYNC_LOG) << "Resource started";
}

EteSyncResource::~EteSyncResource()
{
}

void EteSyncResource::retrieveCollections()
{
    auto job = new JournalsFetchJob(mClient.get(), this);
    connect(job, &JournalsFetchJob::finished, this, [this](KJob *job) {
        if (job->error()) {
            qCWarning(ETESYNC_LOG) << "Error in fetching journals";
            return;
        }
        qCDebug(ETESYNC_LOG) << "Retrieving collections";
        EteSyncJournal **journals = qobject_cast<JournalsFetchJob *>(job)->journals();
        Collection::List list;
        for (EteSyncJournal **iter = journals; *iter; iter++) {
            EteSyncJournalPtr journal(*iter);

            Collection collection;

            /// TODO: Remove temporary hack to handle only addressbooks
            if (setupCollection(collection, journal.get())) continue;

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
    EteSyncCryptoManagerPtr cryptoManager(etesync_journal_get_crypto_manager(journal, derived, keypair.get()));

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

    collection.setParentCollection(Collection::root());
    collection.setRemoteId(journalUid);
    collection.setName(displayName);
    collection.setContentMimeTypes(mimeTypes);

    return typeNum;
}

void EteSyncResource::retrieveItems(const Akonadi::Collection &collection)
{
    QString journalUid = collection.remoteId();
    QString prevUid = collection.remoteRevision();

    auto job = new EntriesFetchJob(mClient.get(), journalUid, prevUid, this);

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

        EteSyncJournalPtr journal(etesync_journal_manager_fetch(journalManager.get(), journalUid));
        EteSyncCryptoManagerPtr cryptoManager(etesync_journal_get_crypto_manager(journal.get(), derived, keypair.get()));

        QMap<QString, KContacts::Addressee> contacts;
        QMap<QString, QString> remoteIDs;

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
                remoteIDs[contact.uid()] = QStringFromCharPtr(CharPtr(etesync_entry_get_uid(entry.get())));
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
        }

        if (isIncremental)
            itemsRetrievedIncremental(changedItems, removedItems);
        else
            itemsRetrieved(changedItems);
    });

    job->start();
}

bool EteSyncResource::retrieveItem(const Akonadi::Item &item,
                                   const QSet<QByteArray> &parts)
{
    // TODO: this method is called when Akonadi wants more data for a given
    // item. You can only provide the parts that have been requested but you are
    // allowed to provide all in one go

    return true;
}

void EteSyncResource::aboutToQuit()
{
    // TODO: any cleanup you need to do while there is still an active
    // event loop. The resource will terminate after this method returns
}

void EteSyncResource::onReloadConfiguration()
{
    Settings::self()->load();
    serverUrl = Settings::self()->baseUrl();
    username = Settings::self()->username();
    password = Settings::self()->password();
    encryptionPassword = Settings::self()->encryptionPassword();

    // serverUrl = QStringLiteral("http://0.0.0.0:9966");
    // username = QStringLiteral("test@localhost");
    // password = QStringLiteral("testetesync");
    // encryptionPassword = QStringLiteral("etesync");

    qCDebug(ETESYNC_LOG) << "Resource config reload";

    connect(this, &EteSyncResource::initialiseDone, this, [this](bool successful) {
        qCDebug(ETESYNC_LOG) << "Resource intialised";
        if (successful) synchronize();
    });

    initialise();
}

void EteSyncResource::initialise()
{
    // Initialise EteSync client state
    mClient = EteSyncPtr(etesync_new(QStringLiteral("Akonadi EteSync Resource"), serverUrl));
    QString token = etesync_auth_get_token(mClient.get(), username, password);
    if (token.isEmpty()) {
        qCWarning(ETESYNC_LOG) << "Unable to obtain token from server";
        Q_EMIT initialiseDone(false);
        return;
    }
    etesync_set_auth_token(mClient.get(), token);
    journalManager = EteSyncJournalManagerPtr(etesync_journal_manager_new(mClient.get()));
    derived = etesync_crypto_derive_key(mClient.get(), username, encryptionPassword);

    // Get user keypair
    EteSyncUserInfoManagerPtr userInfoManager(etesync_user_info_manager_new(mClient.get()));
    EteSyncUserInfoPtr userInfo(etesync_user_info_manager_fetch(userInfoManager.get(), username));
    if (!userInfo) {
        qCWarning(ETESYNC_LOG) << "User info obtained from server is NULL";
        Q_EMIT initialiseDone(false);
        return;
    }
    EteSyncCryptoManagerPtr userInfoCryptoManager(etesync_user_info_get_crypto_manager(userInfo.get(), derived));

    keypair = EteSyncAsymmetricKeyPairPtr(etesync_user_info_get_keypair(userInfo.get(), userInfoCryptoManager.get()));

    Q_EMIT initialiseDone(true);
}

void EteSyncResource::itemAdded(const Akonadi::Item &item,
                                const Akonadi::Collection &collection)
{
    // TODO: this method is called when somebody else, e.g. a client
    // application, has created an item in a collection managed by your
    // resource.

    // NOTE: There is an equivalent method for collections, but it isn't part
    // of this template code to keep it simple
}

void EteSyncResource::itemChanged(const Akonadi::Item &item,
                                  const QSet<QByteArray> &parts)
{
    // TODO: this method is called when somebody else, e.g. a client
    // application, has changed an item managed by your resource.

    // NOTE: There is an equivalent method for collections, but it isn't part
    // of this template code to keep it simple
}

void EteSyncResource::itemRemoved(const Akonadi::Item &item)
{
    // TODO: this method is called when somebody else, e.g. a client
    // application, has deleted an item managed by your resource.

    // NOTE: There is an equivalent method for collections, but it isn't part
    // of this template code to keep it simple
}

AKONADI_RESOURCE_MAIN(EteSyncResource)
