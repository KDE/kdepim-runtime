/*
 * SPDX-FileCopyrightText: 2020 Shashwat Jolly <shashwat.jolly@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QFile>

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

void saveEtebaseCollectionCache(const EtebaseCollectionManager *collectionManager, const EtebaseCollection *etesyncCollection, const QString &cacheDir)
{
    if (!etesyncCollection) {
        qCDebug(ETESYNC_LOG) << "Unable to save collection cache - etesyncCollection is null";
        return;
    }

    QString etesyncCollectionUid = QString::fromUtf8(etebase_collection_get_uid(etesyncCollection));

    qCDebug(ETESYNC_LOG) << "Saving cache for collection" << etesyncCollectionUid;
    uintptr_t ret_size;
    EtebaseCachePtr cache(etebase_collection_manager_cache_save(collectionManager, etesyncCollection, &ret_size));
    QByteArray cacheData((char *)cache.get(), ret_size);

    QString filePath = cacheDir + QStringLiteral("/") + etesyncCollectionUid;
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qCDebug(ETESYNC_LOG) << "Unable to open " << filePath << file.errorString();
        return;
    }

    file.write(cacheData);
}

void saveEtebaseItemCache(const EtebaseItemManager *itemManager, const EtebaseItem *etesyncItem, const QString &cacheDir)
{
    if (!etesyncItem) {
        qCDebug(ETESYNC_LOG) << "Unable to save item cache - etesyncItem is null";
        return;
    }

    QString etesyncItemUid = QString::fromUtf8(etebase_item_get_uid(etesyncItem));

    qCDebug(ETESYNC_LOG) << "Saving cache for item" << etesyncItemUid;
    uintptr_t ret_size;
    EtebaseCachePtr cache(etebase_item_manager_cache_save(itemManager, etesyncItem, &ret_size));
    QByteArray cacheData((char *)cache.get(), ret_size);

    const QString filePath = cacheDir + QStringLiteral("/") + etesyncItemUid;
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qCDebug(ETESYNC_LOG) << "Unable to open " << filePath << file.errorString();
        return;
    }
    file.write(cacheData);
}

EtebaseCollectionPtr getEtebaseCollectionFromCache(const EtebaseCollectionManager *collectionManager, const QString &collectionUid, const QString &cachePath)
{
    if (collectionUid.isEmpty()) {
        qCDebug(ETESYNC_LOG) << "Unable to get collection cache - uid is empty";
        return nullptr;
    }

    qCDebug(ETESYNC_LOG) << "Getting cache for collection" << collectionUid;

    QString collectionCachePath = cachePath + QLatin1Char('/') + collectionUid;

    QFile collectionCacheFile(collectionCachePath);

    if (!collectionCacheFile.exists()) {
        qCDebug(ETESYNC_LOG) << "No cache file for collection" << collectionUid;
        return nullptr;
    }

    if (!collectionCacheFile.open(QIODevice::ReadOnly)) {
        qCDebug(ETESYNC_LOG) << "Unable to open " << collectionCachePath << collectionCacheFile.errorString();
        return nullptr;
    }

    QByteArray collectionCache = collectionCacheFile.readAll();

    if (collectionCache.isEmpty()) {
        qCDebug(ETESYNC_LOG) << "Empty cache file for collection" << collectionUid;
        return nullptr;
    }

    EtebaseCollectionPtr etesyncCollection(etebase_collection_manager_cache_load(collectionManager, collectionCache.constData(), collectionCache.size()));

    return etesyncCollection;
}

EtebaseItemPtr getEtebaseItemFromCache(const EtebaseItemManager *itemManager, const QString &itemUid, const QString &cacheDir)
{
    if (itemUid.isEmpty()) {
        qCDebug(ETESYNC_LOG) << "Unable to get item cache - uid is empty";
        return nullptr;
    }

    qCDebug(ETESYNC_LOG) << "Getting cache for item" << itemUid;

    QString itemCachePath = cacheDir + QLatin1Char('/') + itemUid;

    QFile itemCacheFile(itemCachePath);

    if (!itemCacheFile.exists()) {
        qCDebug(ETESYNC_LOG) << "No cache file for item" << itemUid;
        return nullptr;
    }

    if (!itemCacheFile.open(QIODevice::ReadOnly)) {
        qCDebug(ETESYNC_LOG) << "Unable to open " << itemCachePath << itemCacheFile.errorString();
        return nullptr;
    }

    QByteArray itemCache = itemCacheFile.readAll();

    if (itemCache.isEmpty()) {
        qCDebug(ETESYNC_LOG) << "Empty cache file for item" << itemUid;
        return nullptr;
    }

    EtebaseItemPtr etesyncItem(etebase_item_manager_cache_load(itemManager, itemCache.constData(), itemCache.size()));

    return etesyncItem;
}

void deleteCacheFile(const QString &etebaseUid, const QString &cacheDir)
{
    qCDebug(ETESYNC_LOG) << "Deleting cache file" << etebaseUid;
    const QString path = cacheDir + QLatin1Char('/') + etebaseUid;
    QFile file(path);
    if (!file.remove()) {
        qCDebug(ETESYNC_LOG) << "Unable to remove " << path << file.errorString();
        return;
    }
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
