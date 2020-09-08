/*
 * SPDX-FileCopyrightText: 2020 Shashwat Jolly <shashwat.jolly@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "loginjob.h"

#include <QtConcurrent>

#include "etesync_debug.h"

using namespace EteSyncAPI;

LoginJob::LoginJob(EteSyncClientState *clientState, const QString &serverUrl, const QString &username, const QString &password, QObject *parent)
    : KJob(parent)
    , mClientState(clientState)
    , mServerUrl(serverUrl)
    , mUsername(username)
    , mPassword(password)
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
