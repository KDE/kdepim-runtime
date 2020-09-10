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
    QFutureWatcher<void> *watcher = new QFutureWatcher<void>(this);
    connect(watcher, &QFutureWatcher<void>::finished, this, [this] {
        qCDebug(ETESYNC_LOG) << "emitResult from LoginJob";
        emitResult();
    });
    QFuture<void> loginFuture = QtConcurrent::run(this, &LoginJob::login);
    watcher->setFuture(loginFuture);
}

void LoginJob::login()
{
    qCDebug(ETESYNC_LOG) << "Logging in" << mServerUrl << mUsername << mPassword;
    mLoginResult = mClientState->login(mServerUrl, mUsername, mPassword);
    qCDebug(ETESYNC_LOG) << "Login result" << mLoginResult;
    if (!mLoginResult) {
        qCDebug(ETESYNC_LOG) << "Returning error from LoginJob";
        setError(etebase_error_get_code());
        const char *err = etebase_error_get_message();
        setErrorText(QString::fromUtf8(err));
    }
}
