/*
 * SPDX-FileCopyrightText: 2020 Shashwat Jolly <shashwat.jolly@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "etebaseadapter.h"

#include "etesync_debug.h"

QString QStringFromCharPtr(const CharPtr &str)
{
    if (str.get() == nullptr) {
        return QString();
    }
    const QString ret = QString::fromUtf8(str.get());
    return ret;
}

EtebaseClientPtr etebase_client_new(const QString &client_name, const QString &server_url)
{
    return EtebaseClientPtr(etebase_client_new(charArrFromQString(client_name), charArrFromQString(server_url)));
}

EtebaseAccountPtr etebase_account_login(const EtebaseClient *client, const QString &username, const QString &password)
{
    return EtebaseAccountPtr(etebase_account_login(client, charArrFromQString(username), charArrFromQString(password)));
}

void etebase_fetch_options_set_stoken(EtebaseFetchOptions *fetch_options, const QString &stoken)
{
    etebase_fetch_options_set_stoken(fetch_options, charArrFromQString(stoken));
}

EtebaseCollectionMetadataPtr etebase_collection_metadata_new(const QString &type, const QString &name)
{
    return EtebaseCollectionMetadataPtr(etebase_collection_metadata_new(charArrFromQString(type), charArrFromQString(name)));
}

void etebase_collection_metadata_set_color(EtebaseCollectionMetadata *meta_data, const QString &color)
{
    etebase_collection_metadata_set_color(meta_data, charArrFromQString(color));
}

void etebase_collection_metadata_set_name(EtebaseCollectionMetadata *meta_data, const QString &name)
{
    etebase_collection_metadata_set_name(meta_data, charArrFromQString(name));
}

EtebaseFileSystemCachePtr etebase_fs_cache_new(const QString &path, const QString &username)
{
    return EtebaseFileSystemCachePtr(etebase_fs_cache_new(charArrFromQString(path), charArrFromQString(username)));
}

int32_t etebase_fs_cache_item_set(const EtebaseFileSystemCache *file_cache, const EtebaseItemManager *item_mgr, const QString &col_uid, const EtebaseItem *item)
{
    return etebase_fs_cache_item_set(file_cache, item_mgr, charArrFromQString(col_uid), item);
}

EtebaseCollectionPtr etebase_fs_cache_collection_get(const EtebaseFileSystemCache *fs_cache, const EtebaseCollectionManager *col_mgr, const QString &col_uid)
{
    return EtebaseCollectionPtr(etebase_fs_cache_collection_get(fs_cache, col_mgr, charArrFromQString(col_uid)));
}

EtebaseItemPtr etebase_fs_cache_item_get(const EtebaseFileSystemCache *fs_cache, const EtebaseItemManager *item_mgr, const QString &col_uid, const QString &item_uid)
{
    return EtebaseItemPtr(etebase_fs_cache_item_get(fs_cache, item_mgr, charArrFromQString(col_uid), charArrFromQString(item_uid)));
}

int32_t etebase_fs_cache_collection_unset(const EtebaseFileSystemCache *fs_cache, const EtebaseCollectionManager *col_mgr, const QString &col_uid)
{
    return etebase_fs_cache_collection_unset(fs_cache, col_mgr, charArrFromQString(col_uid));
}

int32_t etebase_fs_cache_item_unset(const EtebaseFileSystemCache *fs_cache, const EtebaseItemManager *item_mgr, const QString &col_uid, const QString &item_uid)
{
    return etebase_fs_cache_item_unset(fs_cache, item_mgr, charArrFromQString(col_uid), charArrFromQString(item_uid));
}
