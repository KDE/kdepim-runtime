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

#ifndef ETESYNCADAPTER_H
#define ETESYNCADAPTER_H

#include <etesync/etesync.h>

#include <QString>

QString QStringFromCharArr(const char *str);

const char *charArrFromQString(const QString &str);

QString etesync_auth_get_token(const EteSync *etesync, const QString &username,
                               const QString &password);

qint32 etesync_auth_invalidate_token(const EteSync *etesync,
                                     const QString &token);

EteSyncCollectionInfo *etesync_collection_info_new(const QString &col_type,
                                                   const QString &display_name,
                                                   const QString &description,
                                                   qint32 color);

QString etesync_crypto_derive_key(const EteSync *_etesync, const QString &salt,
                                  const QString &password);

EteSyncEntry *etesync_entry_from_sync_entry(const EteSyncCryptoManager *crypto_manager,
                                            const EteSyncSyncEntry *sync_entry,
                                            const QString &prev_uid);

EteSyncSyncEntry *etesync_entry_get_sync_entry(const EteSyncEntry *entry,
                                               const EteSyncCryptoManager *crypto_manager,
                                               const QString &prev_uid);

qint32 etesync_entry_manager_create(const EteSyncEntryManager *entry_manager,
                                    const EteSyncEntry *const *entries,
                                    const QString &prev_uid);

EteSyncEntry **etesync_entry_manager_list(const EteSyncEntryManager *entry_manager,
                                          const QString &prev_uid, uintptr_t limit);

EteSyncEntryManager *etesync_entry_manager_new(const EteSync *etesync,
                                               const QString &journal_uid);

EteSyncCryptoManager *etesync_journal_get_crypto_manager(const EteSyncJournal *journal,
                                                         const QString &key,
                                                         const EteSyncAsymmetricKeyPair *keypair);

EteSyncJournal *etesync_journal_manager_fetch(const EteSyncJournalManager *journal_manager,
                                              const QString &journal_uid);

EteSyncJournal *etesync_journal_new(const QString &uid, uint8_t version);

EteSync *etesync_new(const QString &client_name, const QString &server_url);

void etesync_set_auth_token(EteSync *etesync, const QString &token);

EteSyncSyncEntry *etesync_sync_entry_new(const QString &action,
                                         const QString &content);

EteSyncCryptoManager *etesync_user_info_get_crypto_manager(const EteSyncUserInfo *user_info,
                                                           const QString &key);

EteSyncUserInfo *etesync_user_info_manager_fetch(const EteSyncUserInfoManager *user_info_manager,
                                                 const QString &owner);

#endif