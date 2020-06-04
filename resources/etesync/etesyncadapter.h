#include <etesync/etesync.h>

QString QStringFromCharArr(const char *str) { return QLatin1String(str); }

const char *charArrFromQString(const QString &str)
{
    QByteArray ba = str.toLocal8Bit();
    const char *ret = qstrdup(ba.data());
    return ret;
}

QString etesync_auth_get_token(const EteSync *etesync, const QString &username,
                               const QString &password)
{
    const char *uname = charArrFromQString(username);
    const char *pass = charArrFromQString(password);
    char *token = etesync_auth_get_token(etesync, uname, pass);
    return QStringFromCharArr(token);
}

qint32 etesync_auth_invalidate_token(const EteSync *etesync,
                                     const QString &token)
{
    const char *tok = charArrFromQString(token);
    return etesync_auth_invalidate_token(etesync, tok);
}

EteSyncCollectionInfo *etesync_collection_info_new(const QString &col_type,
                                                   const QString &display_name,
                                                   const QString &description,
                                                   qint32 color)
{
    const char *colType = charArrFromQString(col_type);
    const char *displayName = charArrFromQString(display_name);
    const char *desc = charArrFromQString(description);
    return etesync_collection_info_new(colType, displayName, desc, color);
}

QString etesync_crypto_derive_key(const EteSync *_etesync, const QString &salt,
                                  const QString &password)
{
    const char *s = charArrFromQString(salt);
    const char *pass = charArrFromQString(password);
    char *ret = etesync_crypto_derive_key(_etesync, s, pass);
    return QStringFromCharArr(ret);
}

EteSyncEntry *etesync_entry_from_sync_entry(const EteSyncCryptoManager *crypto_manager,
                                            const EteSyncSyncEntry *sync_entry,
                                            const QString &prev_uid)
{
    const char *prevUid = charArrFromQString(prev_uid);
    return etesync_entry_from_sync_entry(crypto_manager, sync_entry, prevUid);
}

EteSyncSyncEntry *etesync_entry_get_sync_entry(const EteSyncEntry *entry,
                                               const EteSyncCryptoManager *crypto_manager,
                                               const QString &prev_uid)
{
    const char *prevUid = charArrFromQString(prev_uid);
    return etesync_entry_get_sync_entry(entry, crypto_manager, prevUid);
}

qint32 etesync_entry_manager_create(const EteSyncEntryManager *entry_manager,
                                    const EteSyncEntry *const *entries,
                                    const QString &prev_uid)
{
    const char *prevUid = charArrFromQString(prev_uid);
    return etesync_entry_manager_create(entry_manager, entries, prevUid);
}

EteSyncEntry **etesync_entry_manager_list(const EteSyncEntryManager *entry_manager,
                                          const QString &prev_uid, uintptr_t limit)
{
    const char *prevUid = charArrFromQString(prev_uid);
    return etesync_entry_manager_list(entry_manager, prevUid, limit);
}

EteSyncEntryManager *etesync_entry_manager_new(const EteSync *etesync,
                                               const QString &journal_uid)
{
    const char *journalUid = charArrFromQString(journal_uid);
    return etesync_entry_manager_new(etesync, journalUid);
}

EteSyncCryptoManager *etesync_journal_get_crypto_manager(const EteSyncJournal *journal,
                                                         const QString &key,
                                                         const EteSyncAsymmetricKeyPair *keypair)
{
    const char *k = charArrFromQString(key);
    return etesync_journal_get_crypto_manager(journal, k, keypair);
}

EteSyncJournal *etesync_journal_manager_fetch(const EteSyncJournalManager *journal_manager,
                                              const QString &journal_uid)
{
    const char *journalUid = charArrFromQString(journal_uid);
    return etesync_journal_manager_fetch(journal_manager, journalUid);
}

EteSyncJournal *etesync_journal_new(const QString &uid, uint8_t version)
{
    const char *id = charArrFromQString(uid);
    return etesync_journal_new(id, version);
}

EteSync *etesync_new(const QString &client_name, const QString &server_url)
{
    const char *clientName = charArrFromQString(client_name);
    const char *serverUrl = charArrFromQString(server_url);
    return etesync_new(clientName, serverUrl);
}

void etesync_set_auth_token(EteSync *etesync, const QString &token)
{
    const char *tok = charArrFromQString(token);
    etesync_set_auth_token(etesync, tok);
}

EteSyncSyncEntry *etesync_sync_entry_new(const QString &action,
                                         const QString &content)
{
    const char *act = charArrFromQString(action);
    const char *cont = charArrFromQString(content);
    return etesync_sync_entry_new(act, cont);
}

EteSyncCryptoManager *etesync_user_info_get_crypto_manager(const EteSyncUserInfo *user_info,
                                                           const QString &key)
{
    const char *k = charArrFromQString(key);
    return etesync_user_info_get_crypto_manager(user_info, k);
}

EteSyncUserInfo *etesync_user_info_manager_fetch(const EteSyncUserInfoManager *user_info_manager,
                                                 const QString &owner)
{
    const char *own = charArrFromQString(owner);
    return etesync_user_info_manager_fetch(user_info_manager, own);
}
