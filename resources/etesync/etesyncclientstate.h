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

#ifndef ETESYNCCLIENTSTATE_H
#define ETESYNCCLIENTSTATE_H

#include "etesyncadapter.h"
#include "settings.h"

class EteSyncClientState : public QObject
{
    Q_OBJECT
public:
    explicit EteSyncClientState() = default;

    void init();
    bool initToken(const QString &serverUrl, const QString &username, const QString &password);
    bool initUserInfo();
    bool initKeypair(const QString &encryptionPassword);
    void initAccount(const QString &encryptionPassword);
    void saveSettings();
    void invalidateToken();

    EteSync *client() const
    {
        return mClient.get();
    }

    QString derived() const
    {
        return mDerived;
    }

    EteSyncJournalManager *journalManager() const
    {
        return mJournalManager.get();
    }

    EteSyncAsymmetricKeyPair *keypair() const
    {
        return mKeypair.get();
    }

    QString username() const
    {
        return mUsername;
    }

public Q_SLOTS:
    void refreshToken();

Q_SIGNALS:
    void clientInitialised(bool successful);
    void tokenRefreshed();

private:
    EteSyncPtr mClient;
    QString mDerived;
    QString mToken;
    EteSyncJournalManagerPtr mJournalManager;
    EteSyncUserInfoPtr mUserInfo;
    EteSyncAsymmetricKeyPairPtr mKeypair;
    QString mUsername;
    QString mPassword;
    QString mServerUrl;
    QString mEncryptionPassword;
};

#endif  // ETESYNCSETTINGS_H
