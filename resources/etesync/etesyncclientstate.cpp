/*
 * SPDX-FileCopyrightText: 2020 Shashwat Jolly <shashwat.jolly@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "etesyncclientstate.h"
#include "settings.h"

#include "etesync_debug.h"

#include <KLocalizedString>
#include <qt6keychain/keychain.h>

using namespace QKeychain;

static constexpr QLatin1StringView etebaseKeychainFolder("Akonadi Etebase");

EteSyncClientState::EteSyncClientState(Settings *const settings, const QString &agentId)
    : mSettings(settings)
    , mAgentId(agentId)
{
}

void EteSyncClientState::init()
{
    // Load settings
    Q_ASSERT(mSettings != nullptr);
    mSettings->load();
    mServerUrl = mSettings->baseUrl();
    mUsername = mSettings->username();

    if (mServerUrl.isEmpty() || mUsername.isEmpty()) {
        Q_EMIT clientInitialised(false, i18nc("@info:status", "Empty account configuration"));
        return;
    }

    // Initialize client object
    mClient = etebase_client_new(QStringLiteral("Akonadi EteSync Resource"), mServerUrl);
    if (!mClient) {
        qCDebug(ETESYNC_LOG) << "Could not initialise Etebase client";
        qCDebug(ETESYNC_LOG) << "Etebase error" << etebase_error_get_message();
        Q_EMIT clientInitialised(false, i18nc("@info:status", "Etebase error when initializing client: %1", QLatin1StringView(etebase_error_get_message())));
        return;
    }

    // Initialize etebase file cache
    mEtebaseCache = etebase_fs_cache_new(mSettings->basePath(), mUsername + u'_' + mAgentId);

    // Load Etebase account from cache
    loadAccount();
}

bool EteSyncClientState::login(const QString &serverUrl, const QString &username, const QString &password)
{
    mServerUrl = serverUrl;
    mUsername = username;

    mClient = etebase_client_new(QStringLiteral("Akonadi EteSync Resource"), mServerUrl);
    if (!mClient) {
        qCDebug(ETESYNC_LOG) << "Could not initialise Etebase client";
        qCDebug(ETESYNC_LOG) << "Etebase error" << etebase_error_get_message();
        return false;
    }
    mAccount = etebase_account_login(mClient.get(), mUsername, password);
    if (!mAccount) {
        qCDebug(ETESYNC_LOG) << "Could not fetch Etebase account";
        qCDebug(ETESYNC_LOG) << "Etebase error" << etebase_error_get_message();
        return false;
    }
    mEtebaseCache = etebase_fs_cache_new(mSettings->basePath(), mUsername + QStringLiteral("_") + mAgentId);
    return true;
}

void EteSyncClientState::logout()
{
    if (etebase_account_logout(mAccount.get())) {
        qCDebug(ETESYNC_LOG) << "Could not logout";
    }
    deleteKeychainEntry();
}

EteSyncClientState::AccountStatus EteSyncClientState::accountStatus()
{
    if (!mAccount) {
        qCDebug(ETESYNC_LOG) << "Could not fetch collection list with limit 1";
        qCDebug(ETESYNC_LOG) << "Etebase account is null";
        return ERROR;
    }
    EtebaseCollectionManagerPtr collectionManager(etebase_account_get_collection_manager(mAccount.get()));
    EtebaseFetchOptionsPtr fetchOptions(etebase_fetch_options_new());
    etebase_fetch_options_set_limit(fetchOptions.get(), 1);
    etebase_fetch_options_set_prefetch(fetchOptions.get(), ETEBASE_PREFETCH_OPTION_MEDIUM);

    EtebaseCollectionListResponsePtr collectionList(
        etebase_collection_manager_list_multi(collectionManager.get(), ETESYNC_COLLECTION_TYPES, ETESYNC_COLLECTION_TYPES_SIZE, fetchOptions.get()));
    if (!collectionList) {
        qCDebug(ETESYNC_LOG) << "Could not fetch collection list with limit 1";
        qCDebug(ETESYNC_LOG) << "Etebase error" << etebase_error_get_message();
        return ERROR;
    }

    uintptr_t dataLength = etebase_collection_list_response_get_data_length(collectionList.get());
    if (dataLength == 0) {
        return NEW_ACCOUNT;
    }
    return OK;
}

void EteSyncClientState::refreshToken()
{
    qCDebug(ETESYNC_LOG) << "Refreshing token";
    if (etebase_account_fetch_token(mAccount.get())) {
        tokenRefreshed(false);
        return;
    }
    tokenRefreshed(true);
}

void EteSyncClientState::saveSettings()
{
    mSettings->setBaseUrl(mServerUrl);
    mSettings->setUsername(mUsername);

    mSettings->save();
}

void EteSyncClientState::saveAccount()
{
    QByteArray encryptionKey(32, '\0');
    etebase_utils_randombytes(encryptionKey.data(), encryptionKey.size());

    auto job = new QKeychain::WritePasswordJob(etebaseKeychainFolder);
    job->setKey(mUsername);
    job->setBinaryData(encryptionKey);

    connect(job, &QKeychain::Job::finished, this, [this, encryptionKey, job] {
        if (job->error() != QKeychain::Error::NoError) {
            qCWarning(ETESYNC_LOG) << "Could not store encryption key for account" << mUsername << "in keychain" << job->error();
            return;
        }

        qCDebug(ETESYNC_LOG) << "Wrote encryption key to keychain";

        if (etebase_fs_cache_save_account(mEtebaseCache.get(), mAccount.get(), encryptionKey.constData(), encryptionKey.size())) {
            qCDebug(ETESYNC_LOG) << "Could not save account to cache";
            qCDebug(ETESYNC_LOG) << "Etebase error:" << etebase_error_get_code() << etebase_error_get_message();
        }
    });
    job->start();
}

void EteSyncClientState::loadAccount()
{
    auto job = new QKeychain::ReadPasswordJob(etebaseKeychainFolder);
    job->setKey(mUsername);
    connect(job, &QKeychain::Job::finished, this, [this, job] {
        if (job->error() != QKeychain::Error::NoError) {
            qCWarning(ETESYNC_LOG) << "Unable to read password:" << job->errorString();
            Q_EMIT clientInitialised(false, i18nc("@info:status", "Unable to read password: %1", job->errorString()));
            return;
        }

        const auto encryptionKey = job->binaryData();
        mAccount = EtebaseAccountPtr(etebase_fs_cache_load_account(mEtebaseCache.get(), mClient.get(), encryptionKey.constData(), encryptionKey.size()));

        if (!mAccount) {
            qCDebug(ETESYNC_LOG) << "Could not get etebase account from cache";
            Q_EMIT clientInitialised(false, i18nc("@info:status", "Could not get etebase account from cache"));
            return;
        }

        Q_EMIT clientInitialised(true);
    });
    job->start();
}

void EteSyncClientState::deleteKeychainEntry()
{
    qCDebug(ETESYNC_LOG) << "Deleting keychain entry";

    auto job = new QKeychain::DeletePasswordJob(etebaseKeychainFolder);
    job->setKey(mUsername);
    connect(job, &QKeychain::Job::finished, this, [this, job] {
        if (job->error() != QKeychain::Error::NoError) {
            qCDebug(ETESYNC_LOG) << "Unable to delete keychain entry";
            return;
        }
        qCDebug(ETESYNC_LOG) << "Deleted keychain entry";
    });
    job->start();
}

void EteSyncClientState::saveEtebaseCollectionCache(const EtebaseCollection *etesyncCollection) const
{
    if (!mAccount) {
        qCDebug(ETESYNC_LOG) << "Unable to save collection cache - account is null";
        return;
    }

    if (!etesyncCollection) {
        qCDebug(ETESYNC_LOG) << "Unable to save collection cache - etesyncCollection is null";
        return;
    }

    EtebaseCollectionManagerPtr collectionManager(etebase_account_get_collection_manager(mAccount.get()));
    if (etebase_fs_cache_collection_set(mEtebaseCache.get(), collectionManager.get(), etesyncCollection)) {
        qCDebug(ETESYNC_LOG) << "Could not save etebase collection cache for collection" << etebase_collection_get_uid(etesyncCollection);
        return;
    }

    qCDebug(ETESYNC_LOG) << "Saved cache for collection" << etebase_collection_get_uid(etesyncCollection);
}

void EteSyncClientState::saveEtebaseItemCache(const EtebaseItem *etesyncItem, const EtebaseCollection *parentCollection) const
{
    if (!mAccount) {
        qCDebug(ETESYNC_LOG) << "Unable to save collection cache - account is null";
        return;
    }

    if (!etesyncItem) {
        qCDebug(ETESYNC_LOG) << "Unable to save item cache - etesyncItem is null";
        return;
    }

    EtebaseCollectionManagerPtr collectionManager(etebase_account_get_collection_manager(mAccount.get()));
    EtebaseItemManagerPtr itemManager(etebase_collection_manager_get_item_manager(collectionManager.get(), parentCollection));

    QString collectionUid = QString::fromUtf8(etebase_collection_get_uid(parentCollection));

    if (etebase_fs_cache_item_set(mEtebaseCache.get(), itemManager.get(), collectionUid, etesyncItem)) {
        qCDebug(ETESYNC_LOG) << "Could not save etebase item cache for item" << etebase_item_get_uid(etesyncItem);
        return;
    }

    qCDebug(ETESYNC_LOG) << "Saved cache for item" << etebase_item_get_uid(etesyncItem);
}

EtebaseCollectionPtr EteSyncClientState::getEtebaseCollectionFromCache(const QString &collectionUid) const
{
    if (!mAccount) {
        qCDebug(ETESYNC_LOG) << "Unable to get collection cache - account is null";
        return nullptr;
    }

    if (collectionUid.isEmpty()) {
        qCDebug(ETESYNC_LOG) << "Unable to get collection cache - uid is empty";
        return nullptr;
    }

    qCDebug(ETESYNC_LOG) << "Getting cache for collection" << collectionUid;

    EtebaseCollectionManagerPtr collectionManager(etebase_account_get_collection_manager(mAccount.get()));

    return etebase_fs_cache_collection_get(mEtebaseCache.get(), collectionManager.get(), collectionUid);
}

EtebaseItemPtr EteSyncClientState::getEtebaseItemFromCache(const QString &itemUid, const EtebaseCollection *parentCollection) const
{
    if (!mAccount) {
        qCDebug(ETESYNC_LOG) << "Unable to get item cache - account is null";
        return nullptr;
    }

    if (!parentCollection) {
        qCDebug(ETESYNC_LOG) << "Unable to get item cache - parentCollection is null";
        return nullptr;
    }

    if (itemUid.isEmpty()) {
        qCDebug(ETESYNC_LOG) << "Unable to get item cache - uid is empty";
        return nullptr;
    }

    qCDebug(ETESYNC_LOG) << "Getting cache for item" << itemUid;

    EtebaseCollectionManagerPtr collectionManager(etebase_account_get_collection_manager(mAccount.get()));
    EtebaseItemManagerPtr itemManager(etebase_collection_manager_get_item_manager(collectionManager.get(), parentCollection));

    QString collectionUid = QString::fromUtf8(etebase_collection_get_uid(parentCollection));

    return etebase_fs_cache_item_get(mEtebaseCache.get(), itemManager.get(), collectionUid, itemUid);
}

void EteSyncClientState::deleteEtebaseCollectionCache(const QString &collectionUid)
{
    EtebaseCollectionManagerPtr collectionManager(etebase_account_get_collection_manager(mAccount.get()));
    if (etebase_fs_cache_collection_unset(mEtebaseCache.get(), collectionManager.get(), collectionUid)) {
        qCDebug(ETESYNC_LOG) << "Could not delete cache for collection" << collectionUid;
        qCDebug(ETESYNC_LOG) << "Etebase error:" << etebase_error_get_code() << etebase_error_get_message();
        return;
    }
    qCDebug(ETESYNC_LOG) << "Deleted cache for collection" << collectionUid;
}

void EteSyncClientState::deleteEtebaseItemCache(const QString &itemUid, const EtebaseCollection *parentCollection)
{
    EtebaseCollectionManagerPtr collectionManager(etebase_account_get_collection_manager(mAccount.get()));
    EtebaseItemManagerPtr itemManager(etebase_collection_manager_get_item_manager(collectionManager.get(), parentCollection));

    const QString collectionUid = QString::fromUtf8(etebase_collection_get_uid(parentCollection));
    if (etebase_fs_cache_item_unset(mEtebaseCache.get(), itemManager.get(), collectionUid, itemUid)) {
        qCDebug(ETESYNC_LOG) << "Could not delete cache for item" << itemUid;
        qCDebug(ETESYNC_LOG) << "Etebase error:" << etebase_error_get_code() << etebase_error_get_message();
        return;
    }
    qCDebug(ETESYNC_LOG) << "Deleted cache for item" << itemUid;
}

void EteSyncClientState::deleteEtebaseUserCache()
{
    if (etebase_fs_cache_clear_user(mEtebaseCache.get())) {
        qCDebug(ETESYNC_LOG) << "Could not remove cache for user";
    }
}

#include "moc_etesyncclientstate.cpp"
