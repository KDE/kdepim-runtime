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

#include "etesyncadapter.h"

#include "etesync_debug.h"

QString QStringFromCharPtr(const CharPtr &str)
{
    if (str.get() == NULL) {
        return QString();
    }
    QString ret = QString::fromLatin1(str.get());
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

EteSyncCollectionInfo *etesync_collection_info_new(const QString &col_type,
                                                   const QString &display_name,
                                                   const QString &description,
                                                   qint32 color)
{
    return etesync_collection_info_new(charArrFromQString(col_type), charArrFromQString(display_name), charArrFromQString(description), color);
}

QString etesync_crypto_derive_key(const EteSync *etesync, const QString &salt,
                                  const QString &password)
{
    CharPtr ret(etesync_crypto_derive_key(etesync, charArrFromQString(salt), charArrFromQString(password)));
    return QStringFromCharPtr(ret);
}

EteSyncEntry *etesync_entry_from_sync_entry(const EteSyncCryptoManager *crypto_manager,
                                            const EteSyncSyncEntry *sync_entry,
                                            const QString &prev_uid)
{
    return etesync_entry_from_sync_entry(crypto_manager, sync_entry, charArrFromQString(prev_uid));
}

EteSyncSyncEntry *etesync_entry_get_sync_entry(const EteSyncEntry *entry,
                                               const EteSyncCryptoManager *crypto_manager,
                                               const QString &prev_uid)
{
    return etesync_entry_get_sync_entry(entry, crypto_manager, charArrFromQString(prev_uid));
}

qint32 etesync_entry_manager_create(const EteSyncEntryManager *entry_manager,
                                    const EteSyncEntry *const *entries,
                                    const QString &prev_uid)
{
    return etesync_entry_manager_create(entry_manager, entries, charArrFromQString(prev_uid));
}

EteSyncEntry **etesync_entry_manager_list(const EteSyncEntryManager *entry_manager,
                                          const QString &prev_uid, uintptr_t limit)
{
    return etesync_entry_manager_list(entry_manager, charArrFromQString(prev_uid), limit);
}

EteSyncEntryManager *etesync_entry_manager_new(const EteSync *etesync,
                                               const QString &journal_uid)
{
    return etesync_entry_manager_new(etesync, charArrFromQString(journal_uid));
}

EteSyncCryptoManager *etesync_journal_get_crypto_manager(const EteSyncJournal *journal,
                                                         const QString &key,
                                                         const EteSyncAsymmetricKeyPair *keypair)
{
    return etesync_journal_get_crypto_manager(journal, charArrFromQString(key), keypair);
}

EteSyncJournal *etesync_journal_manager_fetch(const EteSyncJournalManager *journal_manager,
                                              const QString &journal_uid)
{
    return etesync_journal_manager_fetch(journal_manager, charArrFromQString(journal_uid));
}

EteSyncJournal *etesync_journal_new(const QString &uid, uint8_t version)
{
    return etesync_journal_new(charArrFromQString(uid), version);
}

EteSync *etesync_new(const QString &client_name, const QString &server_url)
{
    return etesync_new(charArrFromQString(client_name), charArrFromQString(server_url));
}

void etesync_set_auth_token(EteSync *etesync, const QString &token)
{
    etesync_set_auth_token(etesync, charArrFromQString(token));
}

EteSyncSyncEntry *etesync_sync_entry_new(const QString &action,
                                         const QString &content)
{
    return etesync_sync_entry_new(charArrFromQString(action), charArrFromQString(content));
}

EteSyncCryptoManager *etesync_user_info_get_crypto_manager(const EteSyncUserInfo *user_info,
                                                           const QString &key)
{
    return etesync_user_info_get_crypto_manager(user_info, charArrFromQString(key));
}

EteSyncUserInfo *etesync_user_info_manager_fetch(const EteSyncUserInfoManager *user_info_manager,
                                                 const QString &owner)
{
    return etesync_user_info_manager_fetch(user_info_manager, charArrFromQString(owner));
}
