/*
 * SPDX-FileCopyrightText: 2020 Shashwat Jolly <shashwat.jolly@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "etebaseadapter.h"
#include "settings.h"

#include <QPointer>

class EteSyncClientState : public QObject
{
    Q_OBJECT
public:
    typedef std::unique_ptr<EteSyncClientState> Ptr;
    explicit EteSyncClientState(Settings *const settings, const QString &agentId);

    enum AccountStatus {
        OK,
        NEW_ACCOUNT,
        ERROR,
    };

    void init();
    void saveSettings();
    bool login(const QString &serverUrl, const QString &username, const QString &password);
    void logout();
    AccountStatus accountStatus();
    void deleteKeychainEntry();
    void saveAccount();
    void loadAccount();
    void saveEtebaseCollectionCache(const EtebaseCollection *etesyncCollection) const;
    void saveEtebaseItemCache(const EtebaseItem *etesyncItem, const EtebaseCollection *parentCollection) const;
    EtebaseCollectionPtr getEtebaseCollectionFromCache(const QString &collectionUid) const;
    EtebaseItemPtr getEtebaseItemFromCache(const QString &itemUid, const EtebaseCollection *parentCollection) const;
    void deleteEtebaseCollectionCache(const QString &collectionUid);
    void deleteEtebaseItemCache(const QString &itemUid, const EtebaseCollection *parentCollection);
    void deleteEtebaseUserCache();

    EtebaseAccount *account() const
    {
        return mAccount.get();
    }

    [[nodiscard]] QString username() const
    {
        return mUsername;
    }

    [[nodiscard]] QString serverUrl() const
    {
        return mServerUrl;
    }

    [[nodiscard]] bool isInitialized() const
    {
        return !mServerUrl.isEmpty();
    }

public Q_SLOTS:
    void refreshToken();

Q_SIGNALS:
    void clientInitialised(bool successful, const QString &error = {});
    void tokenRefreshed(bool successful);

private:
    EtebaseClientPtr mClient;
    EtebaseAccountPtr mAccount;
    EtebaseFileSystemCachePtr mEtebaseCache;
    Settings *mSettings = nullptr;
    QString mUsername;
    QString mPassword;
    QString mServerUrl;
    const QString mAgentId;
};
