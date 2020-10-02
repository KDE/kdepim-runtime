/*
 * SPDX-FileCopyrightText: 2020 Shashwat Jolly <shashwat.jolly@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef ETESYNCCLIENTSTATE_H
#define ETESYNCCLIENTSTATE_H

#include "etebaseadapter.h"
#include "settings.h"

#include <KWallet/KWallet>
#include <QPointer>

class EteSyncClientState : public QObject
{
    Q_OBJECT
public:
    typedef std::unique_ptr<EteSyncClientState> Ptr;
    explicit EteSyncClientState(WId winId);

    void init();
    void saveSettings();
    void invalidateToken();
    bool login(const QString &serverUrl, const QString &username, const QString &password);
    void logout();
    bool accountStatus();
    bool openWalletFolder();
    void deleteWalletEntry();
    void saveAccount();
    void getAccount();

    EtebaseAccount *account() const
    {
        return mAccount.get();
    }

    QString username() const
    {
        return mUsername;
    }

    QString serverUrl() const
    {
        return mServerUrl;
    }

    bool isInitialized()
    {
        return !mServerUrl.isEmpty();
    }

public Q_SLOTS:
    void refreshToken();

Q_SIGNALS:
    void clientInitialised(bool successful);
    void tokenRefreshed(bool successful);

private:
    bool createDefaultJournal(const QString &journalType, const QString &journalName);

    EtebaseClientPtr mClient;
    EtebaseAccountPtr mAccount;
    QString mUsername;
    QString mPassword;
    QString mServerUrl;
    WId mWinId;
    QPointer<KWallet::Wallet> mWallet;
};

#endif // ETESYNCSETTINGS_H
