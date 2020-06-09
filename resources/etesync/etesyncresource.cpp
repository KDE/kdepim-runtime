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

#include <QDBusConnection>

#include "etesync_debug.h"
#include "etesyncadapter.h"
#include "settings.h"
#include "settingsadaptor.h"

using namespace Akonadi;

etesyncResource::etesyncResource(const QString &id)
    : ResourceBase(id)
{
    Settings::instance(KSharedConfig::openConfig());
    new SettingsAdaptor(Settings::self());

    QDBusConnection::sessionBus().registerObject(QStringLiteral("/Settings"),
                                                 Settings::self(),
                                                 QDBusConnection::ExportAdaptors);

    serverUrl = Settings::self()->baseUrl();
    username = Settings::self()->username();
    password = Settings::self()->password();
    encryptionPassword = Settings::self()->encryptionPassword();

    /// TODO: Fix hack
    QString emptyStr = QStringLiteral("");
    if (serverUrl != emptyStr && username != emptyStr && password != emptyStr && encryptionPassword != emptyStr) initialise();

    setNeedsNetwork(true);

    connect(this, &Akonadi::AgentBase::reloadConfiguration, this, &etesyncResource::onReloadConfiguration);

    qCDebug(ETESYNC_LOG) << "Resource started";
}

etesyncResource::~etesyncResource() {}

void etesyncResource::retrieveCollections()
{
    EteSyncJournal **journals = etesync_journal_manager_list(journalManager);

    Collection::List list;

    for (EteSyncJournal **iter = journals; *iter; iter++) {
        EteSyncJournal *journal = *iter;

        EteSyncCryptoManager *cryptoManager = etesync_journal_get_crypto_manager(journal, derived, keypair);

        EteSyncCollectionInfo *info = etesync_journal_get_info(journal, cryptoManager);

        if (QStringFromCharArr(etesync_collection_info_get_type(info)) != QStringLiteral("ADDRESS_BOOK"))
            continue;

        Collection c;
        c.setParentCollection(Collection::root());
        c.setRemoteId(QStringFromCharArr(etesync_journal_get_uid(journal)));
        c.setName(QStringFromCharArr(etesync_collection_info_get_display_name(info)));

        QStringList mimeTypes;
        mimeTypes << KContacts::Addressee::mimeType();
        c.setContentMimeTypes(mimeTypes);

        list << c;

        etesync_collection_info_destroy(info);
        etesync_crypto_manager_destroy(cryptoManager);
        etesync_journal_destroy(journal);
    }

    free(journals);
    collectionsRetrieved(list);
}

void etesyncResource::retrieveItems(const Akonadi::Collection &collection)
{
    const QString journalUid = collection.remoteId();

    EteSyncJournal *journal = etesync_journal_manager_fetch(journalManager, journalUid);

    EteSyncCryptoManager *cryptoManager = etesync_journal_get_crypto_manager(journal, derived, keypair);

    EteSyncEntryManager *entryManager = etesync_entry_manager_new(etesync, journalUid);

    EteSyncEntry **entries = etesync_entry_manager_list(entryManager, NULL, 0);

    char *prevUid = NULL;

    Item::List items;

    QMap<QString, KContacts::Addressee> contacts;
    QMap<QString, QString> remoteIDs;

    for (EteSyncEntry **iter = entries; *iter; iter++) {
        EteSyncEntry *entry = *iter;

        EteSyncSyncEntry *syncEntry = etesync_entry_get_sync_entry(entry, cryptoManager, prevUid);

        QString action = QStringFromCharArr(etesync_sync_entry_get_action(syncEntry));
        KContacts::VCardConverter converter;
        const char *contentStr = etesync_sync_entry_get_content(syncEntry);
        QByteArray content(contentStr);
        const KContacts::Addressee contact = converter.parseVCard(content);

        if (action == QStringLiteral(ETESYNC_SYNC_ENTRY_ACTION_ADD) || action == QStringLiteral(ETESYNC_SYNC_ENTRY_ACTION_CHANGE)) {
            qCDebug(ETESYNC_LOG) << action;
            qCDebug(ETESYNC_LOG) << contact.uid();
            contacts[contact.uid()] = contact;
            remoteIDs[contact.uid()] = QStringFromCharArr(etesync_entry_get_uid(entry));
        } else if (action == QStringLiteral(ETESYNC_SYNC_ENTRY_ACTION_DELETE)) {
            contacts.remove(contact.uid());
        }

        free(prevUid);
        prevUid = etesync_entry_get_uid(entry);

        etesync_sync_entry_destroy(syncEntry);

        etesync_entry_destroy(entry);
    }

    for (auto it = contacts.constBegin(); it != contacts.constEnd(); it++) {
        Item item(it.key());
        item.setMimeType(KContacts::Addressee::mimeType());
        item.setRemoteId(remoteIDs[it.key()]);
        item.setPayload<KContacts::Addressee>(it.value());
        items << item;
    }

    itemsRetrieved(items);
}

bool etesyncResource::retrieveItem(const Akonadi::Item &item,
                                   const QSet<QByteArray> &parts)
{
    // TODO: this method is called when Akonadi wants more data for a given
    // item. You can only provide the parts that have been requested but you are
    // allowed to provide all in one go

    return true;
}

void etesyncResource::aboutToQuit()
{
    // TODO: any cleanup you need to do while there is still an active
    // event loop. The resource will terminate after this method returns
}

void etesyncResource::onReloadConfiguration()
{
    serverUrl = Settings::self()->baseUrl();
    username = Settings::self()->username();
    password = Settings::self()->password();
    encryptionPassword = Settings::self()->encryptionPassword();

    qCDebug(ETESYNC_LOG) << "Resource config reload";

    initialise();
}

void etesyncResource::initialise()
{
    // Initialise EteSync client state
    etesync = etesync_new(QStringLiteral("Akonadi EteSync Resource"), serverUrl);
    derived = etesync_crypto_derive_key(etesync, username, encryptionPassword);
    QString token = etesync_auth_get_token(etesync, username, password);
    etesync_set_auth_token(etesync, token);
    journalManager = etesync_journal_manager_new(etesync);

    // Get user keypair
    EteSyncUserInfoManager *userInfoManager = etesync_user_info_manager_new(etesync);
    EteSyncUserInfo *userInfo = etesync_user_info_manager_fetch(userInfoManager, username);
    EteSyncCryptoManager *userInfoCryptoManager = etesync_user_info_get_crypto_manager(userInfo, derived);

    keypair = etesync_user_info_get_keypair(userInfo, userInfoCryptoManager);
    etesync_crypto_manager_destroy(userInfoCryptoManager);
    etesync_user_info_destroy(userInfo);
    etesync_user_info_manager_destroy(userInfoManager);
    synchronize();
}

void etesyncResource::itemAdded(const Akonadi::Item &item,
                                const Akonadi::Collection &collection)
{
    // TODO: this method is called when somebody else, e.g. a client
    // application, has created an item in a collection managed by your
    // resource.

    // NOTE: There is an equivalent method for collections, but it isn't part
    // of this template code to keep it simple
}

void etesyncResource::itemChanged(const Akonadi::Item &item,
                                  const QSet<QByteArray> &parts)
{
    // TODO: this method is called when somebody else, e.g. a client
    // application, has changed an item managed by your resource.

    // NOTE: There is an equivalent method for collections, but it isn't part
    // of this template code to keep it simple
}

void etesyncResource::itemRemoved(const Akonadi::Item &item)
{
    // TODO: this method is called when somebody else, e.g. a client
    // application, has deleted an item managed by your resource.

    // NOTE: There is an equivalent method for collections, but it isn't part
    // of this template code to keep it simple
}

AKONADI_RESOURCE_MAIN(etesyncResource)
