/*
 * SPDX-FileCopyrightText: 2020 Shashwat Jolly <shashwat.jolly@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "etesyncclientstate.h"

#include <KLocalizedString>

#include "etesync_debug.h"

using namespace KWallet;

static const QString etebaseWalletFolder = QStringLiteral("Akonadi Etebase");

EteSyncClientState::EteSyncClientState(WId winId) : mWinId(winId)
{
}

void EteSyncClientState::init()
{
    // Load settings
    Settings::self()->load();
    mServerUrl = Settings::self()->baseUrl();
    mUsername = Settings::self()->username();

    if (mServerUrl.isEmpty() || mUsername.isEmpty()) {
        Q_EMIT clientInitialised(false);
        return;
    }

    // Initialize client object
    mClient = etebase_client_new(QStringLiteral("Akonadi EteSync Resource"), mServerUrl);
    if (!mClient) {
        qCDebug(ETESYNC_LOG) << "Could not initialise Etebase client";
        qCDebug(ETESYNC_LOG) << "Etebase error" << etebase_error_get_message();
        Q_EMIT clientInitialised(false);
        return;
    }

    // Load Etebase account from cache
    getAccount();

    Q_EMIT clientInitialised(true);
}

bool EteSyncClientState::openWalletFolder()
{
    mWallet = Wallet::openWallet(Wallet::NetworkWallet(), mWinId, Wallet::Synchronous);
    if (mWallet) {
        qCDebug(ETESYNC_LOG) << "Wallet opened";
    } else {
        qCWarning(ETESYNC_LOG) << "Failed to open wallet!";
        return false;
    }
    if (!mWallet->hasFolder(etebaseWalletFolder) && !mWallet->createFolder(etebaseWalletFolder)) {
        qCWarning(ETESYNC_LOG) << "Failed to create wallet folder" << etebaseWalletFolder;
        return false;
    }

    if (!mWallet->setFolder(etebaseWalletFolder)) {
        qWarning() << "Failed to open wallet folder" << etebaseWalletFolder;
        return false;
    }
    qCDebug(ETESYNC_LOG) << "Wallet opened" << etebaseWalletFolder;
    return true;
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
    return true;
}

void EteSyncClientState::logout()
{
    if (etebase_account_logout(mAccount.get())) {
        qCDebug(ETESYNC_LOG) << "Could not logout";
    }
    deleteWalletEntry();
}

bool EteSyncClientState::accountStatus()
{
    if (!mAccount) {
        qCDebug(ETESYNC_LOG) << "Could not fetch collection list with limit 1";
        qCDebug(ETESYNC_LOG) << "Etebase account is null";
        return false;
    }
    EtebaseCollectionManagerPtr collectionManager(etebase_account_get_collection_manager(mAccount.get()));
    EtebaseFetchOptionsPtr fetchOptions(etebase_fetch_options_new());
    etebase_fetch_options_set_limit(fetchOptions.get(), 1);

    EtebaseCollectionListResponsePtr collectionList(etebase_collection_manager_list(collectionManager.get(), fetchOptions.get()));
    if (!collectionList) {
        qCDebug(ETESYNC_LOG) << "Could not fetch collection list with limit 1";
        qCDebug(ETESYNC_LOG) << "Etebase error" << etebase_error_get_message();
        return false;
    }
    return true;
}

void EteSyncClientState::refreshToken()
{
    qCDebug(ETESYNC_LOG) << "Refreshing token";
    if (etebase_account_fetch_token(mAccount.get())) {
        tokenRefreshed(false);
        return;
    }
    tokenRefreshed(true);
    return;
}

void EteSyncClientState::saveSettings()
{
    Settings::self()->setBaseUrl(mServerUrl);
    Settings::self()->setUsername(mUsername);

    const QString cacheDir = Settings::self()->basePath() + QStringLiteral("/") + mUsername;
    Settings::self()->setCacheDir(cacheDir);

    Settings::self()->save();
}

void EteSyncClientState::saveAccount()
{
    if (!mWallet) {
        qCDebug(ETESYNC_LOG) << "Save account - wallet not opened";
        if (!openWalletFolder()) {
            return;
        }
    }

    QByteArray encryptionKey(32, '\0');
    etebase_utils_randombytes(encryptionKey.data(), encryptionKey.size());
    if (mWallet->writeEntry(mUsername, encryptionKey)) {
        qCDebug(ETESYNC_LOG) << "Could not store encryption key for account" << mUsername << "in KWallet";
        return;
    }

    qCDebug(ETESYNC_LOG) << "Wrote encryption key to wallet";

    saveEtebaseAccountCache(mAccount.get(), mUsername, encryptionKey, Settings::self()->cacheDir());
}

void EteSyncClientState::getAccount()
{
    if (!mWallet) {
        qCDebug(ETESYNC_LOG) << "Get account - wallet not opened";
        if (!openWalletFolder()) {
            return;
        }
    }

    if (!mWallet->entryList().contains(mUsername)) {
        qCDebug(ETESYNC_LOG) << "Encryption key for account" << mUsername << "not found in KWallet";
        return;
    }

    QByteArray encryptionKey;
    if (mWallet->readEntry(mUsername, encryptionKey)) {
        qCDebug(ETESYNC_LOG) << "Could not read encryption key for account" << mUsername << "from KWallet";
        return;
    }

    qCDebug(ETESYNC_LOG) << "Read encryption key from wallet";

    mAccount = getEtebaseAccountFromCache(mClient.get(), mUsername, encryptionKey, Settings::self()->cacheDir());
}

void EteSyncClientState::deleteWalletEntry()
{
    qCDebug(ETESYNC_LOG) << "Deleting wallet entry";

    if (!mWallet) {
        qCDebug(ETESYNC_LOG) << "Delete wallet entry - wallet not opened";
        if (!openWalletFolder()) {
            return;
        }
    }

    if (mWallet->removeEntry(mUsername)) {
        qCDebug(ETESYNC_LOG) << "Unable to delete wallet entry";
    }

    qCDebug(ETESYNC_LOG) << "Deleted wallet entry";
}
