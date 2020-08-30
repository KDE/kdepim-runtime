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
