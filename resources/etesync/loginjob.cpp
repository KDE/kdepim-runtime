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

#include "loginjob.h"

#include <QtConcurrent>

#include "etesync_debug.h"

using namespace EteSyncAPI;

LoginJob::LoginJob(EteSyncClientState *clientState, const QString &serverUrl, const QString &username, const QString &password, QObject *parent)
    : KJob(parent), mClientState(clientState), mServerUrl(serverUrl), mUsername(username), mPassword(password)
{
}

void LoginJob::start()
{
    QtConcurrent::run(this, &LoginJob::login);
}

void LoginJob::login()
{
    mLoginResult = mClientState->initToken(mServerUrl, mUsername, mPassword);
    if (!mLoginResult) {
        qCDebug(ETESYNC_LOG) << "Returning error from LoginJob";
        setError(etesync_get_error_code());
        CharPtr err(etesync_get_error_message());
        setErrorText(QStringFromCharPtr(err));
        emitResult();
        return;
    }
    mUserInfoResult = mClientState->initUserInfo();
    if (!mUserInfoResult) {
        qCDebug(ETESYNC_LOG) << "Returning error from LoginJob";
        setError(etesync_get_error_code());
        CharPtr err(etesync_get_error_message());
        setErrorText(QStringFromCharPtr(err));
        emitResult();
        return;
    }
    emitResult();
}