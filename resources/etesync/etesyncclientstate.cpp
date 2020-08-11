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

#include "etesyncclientstate.h"

#include "etesync_debug.h"

void EteSyncClientState::init()
{
    // Load settings
    Settings::self()->load();
    mServerUrl = Settings::self()->baseUrl();
    mUsername = Settings::self()->username();
    mPassword = Settings::self()->password();
    mEncryptionPassword = Settings::self()->encryptionPassword();

    // mServerUrl = QStringLiteral("http://0.0.0.0:9966");
    // mUsername = QStringLiteral("test@localhost");
    // mPassword = QStringLiteral("testetesync");
    // mEncryptionPassword = QStringLiteral("etesync");

    if (mServerUrl.isEmpty() || mUsername.isEmpty() || mPassword.isEmpty() || mEncryptionPassword.isEmpty()) {
        Q_EMIT clientInitialised(false);
        return;
    }

    // Initialise EteSync client state
    mClient = EteSyncPtr(etesync_new(QStringLiteral("Akonadi EteSync Resource"), mServerUrl));
    mToken = etesync_auth_get_token(mClient.get(), mUsername, mPassword);
    if (mToken.isEmpty()) {
        qCWarning(ETESYNC_LOG) << "Unable to obtain token from server" << QString::fromLatin1(etesync_get_error_message());
        Q_EMIT clientInitialised(false);
        return;
    }
    etesync_set_auth_token(mClient.get(), mToken);
    mJournalManager = EteSyncJournalManagerPtr(etesync_journal_manager_new(mClient.get()));
    mDerived = etesync_crypto_derive_key(mClient.get(), mUsername, mEncryptionPassword);

    // Get user keypair
    EteSyncUserInfoManagerPtr userInfoManager(etesync_user_info_manager_new(mClient.get()));
    EteSyncUserInfoPtr userInfo(etesync_user_info_manager_fetch(userInfoManager.get(), mUsername));
    if (!userInfo) {
        qCWarning(ETESYNC_LOG) << "User info obtained from server is NULL";
        invalidateToken();
        Q_EMIT clientInitialised(false);
        return;
    }
    EteSyncCryptoManagerPtr userInfoCryptoManager(etesync_user_info_get_crypto_manager(userInfo.get(), mDerived));

    mKeypair = EteSyncAsymmetricKeyPairPtr(etesync_user_info_get_keypair(userInfo.get(), userInfoCryptoManager.get()));

    Q_EMIT clientInitialised(true);
}

bool EteSyncClientState::initToken(const QString &serverUrl, const QString &username, const QString &password)
{
    mServerUrl = serverUrl;
    mUsername = username;
    mPassword = password;

    // Initialise EteSync client state
    mClient = EteSyncPtr(etesync_new(QStringLiteral("Akonadi EteSync Resource"), mServerUrl));
    mToken = etesync_auth_get_token(mClient.get(), mUsername, mPassword);
    if (mToken.isEmpty()) {
        qCDebug(ETESYNC_LOG) << "Empty token";
        return false;
    }
    qCDebug(ETESYNC_LOG) << "Received token" << mToken;
    etesync_set_auth_token(mClient.get(), mToken);
    return true;
}

void EteSyncClientState::refreshToken()
{
    mToken = etesync_auth_get_token(mClient.get(), mUsername, mPassword);
    if (mToken.isEmpty()) {
        qCDebug(ETESYNC_LOG) << "Empty token";
        return;
    }
    qCDebug(ETESYNC_LOG) << "Received token" << mToken;
    etesync_set_auth_token(mClient.get(), mToken);
    tokenRefreshed();
}

bool EteSyncClientState::initUserInfo()
{
    mJournalManager = EteSyncJournalManagerPtr(etesync_journal_manager_new(mClient.get()));
    EteSyncUserInfoManagerPtr userInfoManager(etesync_user_info_manager_new(mClient.get()));
    mUserInfo = EteSyncUserInfoPtr(etesync_user_info_manager_fetch(userInfoManager.get(), mUsername));
    if (!mUserInfo) {
        qCWarning(ETESYNC_LOG) << "User info obtained from server is NULL";
        invalidateToken();
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

    EteSyncCryptoManagerPtr userInfoCryptoManager(etesync_user_info_get_crypto_manager(mUserInfo.get(), mDerived));
    mKeypair = EteSyncAsymmetricKeyPairPtr(etesync_user_info_get_keypair(mUserInfo.get(), userInfoCryptoManager.get()));
    if (!mKeypair) {
        qCDebug(ETESYNC_LOG) << "Empty keypair";
        return false;
    }
    qCDebug(ETESYNC_LOG) << "Received keypair";
    return true;
}

/// TODO: Create default journals
void EteSyncClientState::initAccount(const QString &encryptionPassword)
{
    mEncryptionPassword = encryptionPassword;
    mDerived = etesync_crypto_derive_key(mClient.get(), mUsername, mEncryptionPassword);
    mUserInfo = EteSyncUserInfoPtr(etesync_user_info_new(mUsername, ETESYNC_CURRENT_VERSION));
    mKeypair = EteSyncAsymmetricKeyPairPtr(etesync_crypto_generate_keypair(mClient.get()));
    EteSyncCryptoManagerPtr userInfoCryptoManager(etesync_user_info_get_crypto_manager(mUserInfo.get(), mDerived));
    etesync_user_info_set_keypair(mUserInfo.get(), userInfoCryptoManager.get(), mKeypair.get());
    EteSyncUserInfoManagerPtr userInfoManager(etesync_user_info_manager_new(mClient.get()));
    etesync_user_info_manager_create(userInfoManager.get(), mUserInfo.get());
}

void EteSyncClientState::saveSettings()
{
    Settings::self()->setBaseUrl(mServerUrl);
    Settings::self()->setUsername(mUsername);
    Settings::self()->setPassword(mPassword);
    Settings::self()->setEncryptionPassword(mEncryptionPassword);
    Settings::self()->save();
}
