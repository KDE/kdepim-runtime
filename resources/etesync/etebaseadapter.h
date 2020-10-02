/*
 * SPDX-FileCopyrightText: 2020 Shashwat Jolly <shashwat.jolly@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef ETEBASEADAPTER_H
#define ETEBASEADAPTER_H

#include <etebase.h>
#include <QString>
#include <memory>
#include <vector>

#define charArrFromQString(str) ((str).isNull() ? nullptr : qUtf8Printable((str)))

#define ETEBASE_COLLECTION_TYPE_CALENDAR QStringLiteral("etebase.vevent")
#define ETEBASE_COLLECTION_TYPE_ADDRESS_BOOK QStringLiteral("etebase.vcard")
#define ETEBASE_COLLECTION_TYPE_TASKS QStringLiteral("etebase.vtodo")

#define ETESYNC_DEFAULT_COLLECTION_COLOR QStringLiteral("#8BC34A")

struct EtebaseDeleter
{
    void operator()(EtebaseClient *ptr)
    {
        etebase_client_destroy(ptr);
    }

    void operator()(EtebaseAccount *ptr)
    {
        etebase_account_destroy(ptr);
    }

    void operator()(EtebaseFetchOptions *ptr)
    {
        etebase_fetch_options_destroy(ptr);
    }

    void operator()(EtebaseCollectionListResponse *ptr)
    {
        etebase_collection_list_response_destroy(ptr);
    }

    void operator()(EtebaseCollection *ptr)
    {
        etebase_collection_destroy(ptr);
    }

    void operator()(EtebaseCollectionMetadata *ptr)
    {
        etebase_collection_metadata_destroy(ptr);
    }

    void operator()(EtebaseItemManager *ptr)
    {
        etebase_item_manager_destroy(ptr);
    }

    void operator()(EtebaseItemListResponse *ptr)
    {
        etebase_item_list_response_destroy(ptr);
    }

    void operator()(EtebaseItemMetadata *ptr)
    {
        etebase_item_metadata_destroy(ptr);
    }

    void operator()(EtebaseItem *ptr)
    {
        etebase_item_destroy(ptr);
    }

    void operator()(char *ptr)
    {
        std::free(ptr);
    }

    void operator()(void *ptr)
    {
        std::free(ptr);
    }
};

using EtebaseClientPtr = std::unique_ptr<EtebaseClient, EtebaseDeleter>;
using EtebaseAccountPtr = std::unique_ptr<EtebaseAccount, EtebaseDeleter>;
using EtebaseFetchOptionsPtr = std::unique_ptr<EtebaseFetchOptions, EtebaseDeleter>;
using EtebaseCollectionListResponsePtr = std::unique_ptr<EtebaseCollectionListResponse, EtebaseDeleter>;
using EtebaseCollectionManagerPtr = std::unique_ptr<EtebaseCollectionManager, EtebaseDeleter>;
using EtebaseCollectionPtr = std::unique_ptr<EtebaseCollection, EtebaseDeleter>;
using EtebaseCollectionMetadataPtr = std::unique_ptr<EtebaseCollectionMetadata, EtebaseDeleter>;
using EtebaseItemManagerPtr = std::unique_ptr<EtebaseItemManager, EtebaseDeleter>;
using EtebaseItemListResponsePtr = std::unique_ptr<EtebaseItemListResponse, EtebaseDeleter>;
using EtebaseItemMetadataPtr = std::unique_ptr<EtebaseItemMetadata, EtebaseDeleter>;
using EtebaseItemPtr = std::unique_ptr<EtebaseItem, EtebaseDeleter>;
using EtebaseCachePtr = std::unique_ptr<void, EtebaseDeleter>;
using CharPtr = std::unique_ptr<char, EtebaseDeleter>;

QString QStringFromCharPtr(const CharPtr &str);
void saveEtebaseAccountCache(EtebaseAccount *account, const QString &username, const QString &cacheDir);
void saveEtebaseCollectionCache(const EtebaseCollectionManager *collectionManager, const EtebaseCollection *etesyncCollection, const QString &cacheDir);
void saveEtebaseItemCache(const EtebaseItemManager *itemManager, const EtebaseItem *etesyncItem, const QString &cacheDir);
EtebaseAccountPtr getEtebaseAccountFromCache(const EtebaseClient *client, const QString &username, const QString &cacheDir);
EtebaseCollectionPtr getEtebaseCollectionFromCache(const EtebaseCollectionManager *collectionManager, const QString &collectionUid, const QString &cacheDir);
EtebaseItemPtr getEtebaseItemFromCache(const EtebaseItemManager *itemManager, const QString &itemUid, const QString &cacheDir);
void deleteCacheFile(const QString &etebaseUid, const QString &cacheDir);

EtebaseClientPtr etebase_client_new(const QString &client_name, const QString &server_url);

EtebaseAccountPtr etebase_account_login(const EtebaseClient *client, const QString &username, const QString &password);

void etebase_fetch_options_set_stoken(EtebaseFetchOptions *fetch_options, const QString &stoken);

EtebaseCollectionMetadataPtr etebase_collection_metadata_new(const QString &type, const QString &name);

void etebase_collection_metadata_set_color(EtebaseCollectionMetadata *meta_data, const QString &color);

void etebase_collection_metadata_set_name(EtebaseCollectionMetadata *meta_data, const QString &name);

#endif
