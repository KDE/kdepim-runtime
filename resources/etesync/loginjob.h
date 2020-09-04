/*
 * SPDX-FileCopyrightText: 2020 Shashwat Jolly <shashwat.jolly@gmail.com>
 * 
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef LOGINJOB_H
#define LOGINJOB_H

#include <KJob>

#include "etesyncadapter.h"
#include "etesyncclientstate.h"

namespace EteSyncAPI {
    class LoginJob : public KJob
    {
        Q_OBJECT

    public:
        explicit LoginJob(EteSyncClientState *clientState, const QString &serverUrl, const QString &username, const QString &password, QObject *parent = nullptr);

        void start() override;

        bool getLoginResult() const
        {
            return mLoginResult;
        }

        bool getUserInfoResult() const
        {
            return mUserInfoResult;
        }

    private:
        void login();

        EteSyncClientState *mClientState = nullptr;
        QString mServerUrl;
        QString mUsername;
        QString mPassword;
        bool mLoginResult;
        bool mUserInfoResult;
    };
}  // namespace EteSyncAPI

#endif
