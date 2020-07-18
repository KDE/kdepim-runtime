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

EteSyncClientState::EteSyncClientState()
{
}

EteSyncClientState::~EteSyncClientState()
{
    etesync_auth_invalidate_token(mClient.get(), mToken);
}

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
        Q_EMIT clientInitialised(false);
        return;
    }
    EteSyncCryptoManagerPtr userInfoCryptoManager(etesync_user_info_get_crypto_manager(userInfo.get(), mDerived));

    mKeypair = EteSyncAsymmetricKeyPairPtr(etesync_user_info_get_keypair(userInfo.get(), userInfoCryptoManager.get()));

    Q_EMIT clientInitialised(true);
}

bool EteSyncClientState::initToken(QString &serverUrl, QString &username, QString &password)
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

bool EteSyncClientState::initKeypair(const QString &encryptionPassword)
{
    mEncryptionPassword = encryptionPassword;
    mJournalManager = EteSyncJournalManagerPtr(etesync_journal_manager_new(mClient.get()));
    mDerived = etesync_crypto_derive_key(mClient.get(), mUsername, mEncryptionPassword);

    // Get user keypair
    EteSyncUserInfoManagerPtr userInfoManager(etesync_user_info_manager_new(mClient.get()));
    EteSyncUserInfoPtr userInfo(etesync_user_info_manager_fetch(userInfoManager.get(), mUsername));
    if (!userInfo) {
        qCWarning(ETESYNC_LOG) << "User info obtained from server is NULL";
        return false;
    }
    EteSyncCryptoManagerPtr userInfoCryptoManager(etesync_user_info_get_crypto_manager(userInfo.get(), mDerived));
    mKeypair = EteSyncAsymmetricKeyPairPtr(etesync_user_info_get_keypair(userInfo.get(), userInfoCryptoManager.get()));
    if (!mKeypair) {
        qCDebug(ETESYNC_LOG) << "Empty keypair";
        return false;
    }
    qCDebug(ETESYNC_LOG) << "Received keypair";
    return true;
}

void EteSyncClientState::saveCredentials()
{
    Settings::self()->setBaseUrl(mServerUrl);
    Settings::self()->setUsername(mUsername);
    Settings::self()->setPassword(mPassword);
    Settings::self()->setEncryptionPassword(mEncryptionPassword);
    Settings::self()->save();
}
