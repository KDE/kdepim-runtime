/*
 * SPDX-FileCopyrightText: 2020 Shashwat Jolly <shashwat.jolly@gmail.com>
 * 
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "etesyncadapter.h"

#include "etesync_debug.h"

QString QStringFromCharPtr(const CharPtr &str)
{
    if (str.get() == nullptr) {
        return QString();
    }
    const QString ret = QString::fromUtf8(str.get());
    return ret;
}

QString etesync_auth_get_token(const EteSync *etesync, const QString &username,
                               const QString &password)
{
    CharPtr token(etesync_auth_get_token(etesync, charArrFromQString(username), charArrFromQString(password)));
    return QStringFromCharPtr(token);
}

qint32 etesync_auth_invalidate_token(const EteSync *etesync,
                                     const QString &token)
{
    return etesync_auth_invalidate_token(etesync, charArrFromQString(token));
}

EteSyncCollectionInfoPtr etesync_collection_info_new(const QString &col_type,
                                                     const QString &display_name,
                                                     const QString &description,
                                                     qint32 color)
{
    return EteSyncCollectionInfoPtr(etesync_collection_info_new(charArrFromQString(col_type), charArrFromQString(display_name), charArrFromQString(description), color));
}

QString etesync_crypto_derive_key(const EteSync *etesync, const QString &salt,
                                  const QString &password)
{
    CharPtr ret(etesync_crypto_derive_key(etesync, charArrFromQString(salt), charArrFromQString(password)));
    return QStringFromCharPtr(ret);
}

EteSyncEntryPtr etesync_entry_from_sync_entry(const EteSyncCryptoManager *crypto_manager,
                                              const EteSyncSyncEntry *sync_entry,
                                              const QString &prev_uid)
{
    return EteSyncEntryPtr(etesync_entry_from_sync_entry(crypto_manager, sync_entry, charArrFromQString(prev_uid)));
}

EteSyncSyncEntryPtr etesync_entry_get_sync_entry(const EteSyncEntry *entry,
                                                 const EteSyncCryptoManager *crypto_manager,
                                                 const QString &prev_uid)
{
    return EteSyncSyncEntryPtr(etesync_entry_get_sync_entry(entry, crypto_manager, charArrFromQString(prev_uid)));
}

qint32 etesync_entry_manager_create(const EteSyncEntryManager *entry_manager,
                                    const EteSyncEntry *const *entries,
                                    const QString &prev_uid)
{
    return etesync_entry_manager_create(entry_manager, entries, charArrFromQString(prev_uid));
}

std::pair<std::vector<EteSyncEntryPtr>, bool> etesync_entry_manager_list(const EteSyncEntryManager *entry_manager,
                                                                         const QString &prev_uid, uintptr_t limit)
{
    std::vector<EteSyncEntryPtr> rv;
    bool err = false;
    auto entries = etesync_entry_manager_list(entry_manager, charArrFromQString(prev_uid), limit);
    if (!entries) {
        err = true;
        return make_pair(std::move(rv), err);
    }
    while (*entries) {
        rv.emplace_back(EteSyncEntryPtr{*entries});
        ++entries;
    }
    return make_pair(std::move(rv), err);
}

EteSyncEntryManagerPtr etesync_entry_manager_new(const EteSync *etesync,
                                                 const QString &journal_uid)
{
    return EteSyncEntryManagerPtr(etesync_entry_manager_new(etesync, charArrFromQString(journal_uid)));
}

EteSyncCryptoManagerPtr etesync_journal_get_crypto_manager(const EteSyncJournal *journal,
                                                           const QString &key,
                                                           const EteSyncAsymmetricKeyPair *keypair)
{
    return EteSyncCryptoManagerPtr(etesync_journal_get_crypto_manager(journal, charArrFromQString(key), keypair));
}

EteSyncJournalPtr etesync_journal_manager_fetch(const EteSyncJournalManager *journal_manager,
                                                const QString &journal_uid)
{
    return EteSyncJournalPtr(etesync_journal_manager_fetch(journal_manager, charArrFromQString(journal_uid)));
}

EteSyncJournalPtr etesync_journal_new(const QString &uid, uint8_t version)
{
    return EteSyncJournalPtr(etesync_journal_new(charArrFromQString(uid), version));
}

EteSyncPtr etesync_new(const QString &client_name, const QString &server_url)
{
    return EteSyncPtr(etesync_new(charArrFromQString(client_name), charArrFromQString(server_url)));
}

void etesync_set_auth_token(EteSync *etesync, const QString &token)
{
    etesync_set_auth_token(etesync, charArrFromQString(token));
}

EteSyncSyncEntryPtr etesync_sync_entry_new(const QString &action,
                                           const QString &content)
{
    return EteSyncSyncEntryPtr(etesync_sync_entry_new(charArrFromQString(action), charArrFromQString(content)));
}

EteSyncCryptoManagerPtr etesync_user_info_get_crypto_manager(const EteSyncUserInfo *user_info,
                                                             const QString &key)
{
    return EteSyncCryptoManagerPtr(etesync_user_info_get_crypto_manager(user_info, charArrFromQString(key)));
}

EteSyncUserInfoPtr etesync_user_info_manager_fetch(const EteSyncUserInfoManager *user_info_manager,
                                                   const QString &owner)
{
    return EteSyncUserInfoPtr(etesync_user_info_manager_fetch(user_info_manager, charArrFromQString(owner)));
}

EteSyncUserInfoPtr etesync_user_info_new(const QString &owner, uint8_t version)
{
    return EteSyncUserInfoPtr(etesync_user_info_new(charArrFromQString(owner), version));
}
