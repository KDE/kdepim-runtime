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
    mClientXXX = etebase_client_new(QStringLiteral("Akonadi EteSync Resource"), mServerUrl);
    if (!mClientXXX) {
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

bool EteSyncClientState::initToken(const QString &serverUrl, const QString &username, const QString &password)
{
    mServerUrl = serverUrl;
    mUsername = username;
    mPassword = password;

    // Initialise EteSync client state
    mClient = etesync_new(QStringLiteral("Akonadi EteSync Resource"), mServerUrl);
    if (!mClient) {
        qCDebug(ETESYNC_LOG) << "Could not initialize EteSync client";
        qCDebug(ETESYNC_LOG) << "EteSync error" << QStringFromCharPtr(CharPtr(etesync_get_error_message()));
        return false;
    }
    mToken = etesync_auth_get_token(mClient.get(), mUsername, mPassword);
    if (mToken.isEmpty()) {
        qCDebug(ETESYNC_LOG) << "Empty token";
        qCDebug(ETESYNC_LOG) << "EteSync error" << QStringFromCharPtr(CharPtr(etesync_get_error_message()));
        return false;
    }
    qCDebug(ETESYNC_LOG) << "Received token" << mToken;
    etesync_set_auth_token(mClient.get(), mToken);
    return true;
}

bool EteSyncClientState::login(const QString &serverUrl, const QString &username, const QString &password)
{
    mServerUrl = serverUrl;
    mUsername = username;

    mClientXXX = etebase_client_new(QStringLiteral("Akonadi EteSync Resource"), mServerUrl);
    if (!mClientXXX) {
        qCDebug(ETESYNC_LOG) << "Could not initialise Etebase client";
        qCDebug(ETESYNC_LOG) << "Etebase error" << etebase_error_get_message();
        return false;
    }
    mAccountXXX = etebase_account_login(mClientXXX.get(), mUsername, password);
    if (!mAccountXXX) {
        qCDebug(ETESYNC_LOG) << "Could not fetch Etebase account";
        qCDebug(ETESYNC_LOG) << "Etebase error" << etebase_error_get_message();
        return false;
    }
    return true;
}

void EteSyncClientState::logout()
{
    if (etebase_account_logout(mAccountXXX.get())) {
        qCDebug(ETESYNC_LOG) << "Could not logout";
    }
    deleteWalletEntry();
}

bool EteSyncClientState::accountStatus()
{
    if (!mAccountXXX) {
        qCDebug(ETESYNC_LOG) << "Could not fetch collection list with limit 1";
        qCDebug(ETESYNC_LOG) << "Etebase account is null";
        return false;
    }
    EtebaseCollectionManagerPtr collectionManager(etebase_account_get_collection_manager(mAccountXXX.get()));
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
    if (etebase_account_fetch_token(mAccountXXX.get())) {
        tokenRefreshed(false);
        return;
    }
    tokenRefreshed(true);
    return;
}

void EteSyncClientState::refreshUserInfo()
{
    EteSyncUserInfoManagerPtr userInfoManager(etesync_user_info_manager_new(mClient.get()));
    mUserInfo = etesync_user_info_manager_fetch(userInfoManager.get(), mUsername);
    if (!mUserInfo) {
        qCWarning(ETESYNC_LOG) << "refreshUserInfo() - User info obtained from server is NULL";
        qCDebug(ETESYNC_LOG) << "EteSync error" << QStringFromCharPtr(CharPtr(etesync_get_error_message()));
        return;
    }
    EteSyncCryptoManagerPtr userInfoCryptoManager = etesync_user_info_get_crypto_manager(mUserInfo.get(), mDerived);
    mKeypair = EteSyncAsymmetricKeyPairPtr(etesync_user_info_get_keypair(mUserInfo.get(), userInfoCryptoManager.get()));
    if (!mKeypair) {
        qCDebug(ETESYNC_LOG) << "Empty keypair";
        qCDebug(ETESYNC_LOG) << "EteSync error" << QStringFromCharPtr(CharPtr(etesync_get_error_message()));
        return;
    }
    qCDebug(ETESYNC_LOG) << "Received keypair";
}

bool EteSyncClientState::initUserInfo()
{
    mJournalManager = EteSyncJournalManagerPtr(etesync_journal_manager_new(mClient.get()));
    EteSyncUserInfoManagerPtr userInfoManager(etesync_user_info_manager_new(mClient.get()));
    mUserInfo = etesync_user_info_manager_fetch(userInfoManager.get(), mUsername);
    if (!mUserInfo) {
        qCWarning(ETESYNC_LOG) << "initUserInfo() - User info obtained from server is NULL";
        qCDebug(ETESYNC_LOG) << "EteSync error" << QStringFromCharPtr(CharPtr(etesync_get_error_message()));
        return false;
    }
    return true;
}

void EteSyncClientState::invalidateToken()
{
    etesync_auth_invalidate_token(mClient.get(), mToken);
}

bool EteSyncClientState::initKeypair(const QString &encryptionPassword)
{
    mEncryptionPassword = encryptionPassword;
    mDerived = etesync_crypto_derive_key(mClient.get(), mUsername, mEncryptionPassword);

    EteSyncCryptoManagerPtr userInfoCryptoManager = etesync_user_info_get_crypto_manager(mUserInfo.get(), mDerived);
    mKeypair = EteSyncAsymmetricKeyPairPtr(etesync_user_info_get_keypair(mUserInfo.get(), userInfoCryptoManager.get()));
    if (!mKeypair) {
        qCDebug(ETESYNC_LOG) << "Empty keypair";
        qCDebug(ETESYNC_LOG) << "EteSync error" << QStringFromCharPtr(CharPtr(etesync_get_error_message()));
        return false;
    }
    qCDebug(ETESYNC_LOG) << "Received keypair";
    return true;
}

bool EteSyncClientState::initAccount(const QString &encryptionPassword)
{
    mEncryptionPassword = encryptionPassword;
    mDerived = etesync_crypto_derive_key(mClient.get(), mUsername, mEncryptionPassword);
    if (mDerived.isEmpty()) {
        qCDebug(ETESYNC_LOG) << "Could not derive key from encryption password";
        qCDebug(ETESYNC_LOG) << "EteSync error" << QStringFromCharPtr(CharPtr(etesync_get_error_message()));
        return false;
    }
    mUserInfo = etesync_user_info_new(mUsername, ETESYNC_CURRENT_VERSION);
    mKeypair = EteSyncAsymmetricKeyPairPtr(etesync_crypto_generate_keypair(mClient.get()));
    EteSyncCryptoManagerPtr userInfoCryptoManager = etesync_user_info_get_crypto_manager(mUserInfo.get(), mDerived);
    if (etesync_user_info_set_keypair(mUserInfo.get(), userInfoCryptoManager.get(), mKeypair.get())) {
        qCDebug(ETESYNC_LOG) << "Could not create set user info keypair";
        qCDebug(ETESYNC_LOG) << "EteSync error" << QStringFromCharPtr(CharPtr(etesync_get_error_message()));
        return false;
    }
    EteSyncUserInfoManagerPtr userInfoManager(etesync_user_info_manager_new(mClient.get()));
    if (etesync_user_info_manager_create(userInfoManager.get(), mUserInfo.get())) {
        qCDebug(ETESYNC_LOG) << "Could not create user info";
        qCDebug(ETESYNC_LOG) << "EteSync error" << QStringFromCharPtr(CharPtr(etesync_get_error_message()));
        return false;
    }

    const auto calendarResult = createDefaultJournal(QStringLiteral(ETESYNC_COLLECTION_TYPE_CALENDAR), i18n("My Calendar"));
    const auto contactsResult = createDefaultJournal(QStringLiteral(ETESYNC_COLLECTION_TYPE_ADDRESS_BOOK), i18n("My Contacts"));
    const auto tasksResult = createDefaultJournal(QStringLiteral(ETESYNC_COLLECTION_TYPE_TASKS), i18n("My Tasks"));

    return calendarResult && contactsResult && tasksResult;
}

bool EteSyncClientState::createDefaultJournal(const QString &journalType, const QString &journalName)
{
    const QString journalUid = QStringFromCharPtr(CharPtr(etesync_gen_uid()));
    EteSyncJournalPtr journal = etesync_journal_new(journalUid, ETESYNC_CURRENT_VERSION);

    EteSyncCollectionInfoPtr info = etesync_collection_info_new(journalType, journalName, QString(), ETESYNC_COLLECTION_DEFAULT_COLOR);

    EteSyncCryptoManagerPtr cryptoManager = etesync_journal_get_crypto_manager(journal.get(), mDerived, mKeypair.get());

    if (etesync_journal_set_info(journal.get(), cryptoManager.get(), info.get())) {
        qCDebug(ETESYNC_LOG) << "Could not set journal info";
        qCDebug(ETESYNC_LOG) << "EteSync error" << QStringFromCharPtr(CharPtr(etesync_get_error_message()));
        return false;
    }

    if (etesync_journal_manager_create(mJournalManager.get(), journal.get())) {
        qCDebug(ETESYNC_LOG) << "Could not create journal";
        qCDebug(ETESYNC_LOG) << "EteSync error" << QStringFromCharPtr(CharPtr(etesync_get_error_message()));
        return false;
    }
    return true;
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

    saveEtebaseAccountCache(mAccountXXX.get(), mUsername, encryptionKey, Settings::self()->cacheDir());
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

    mAccountXXX = getEtebaseAccountFromCache(mClientXXX.get(), mUsername, encryptionKey, Settings::self()->cacheDir());
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
