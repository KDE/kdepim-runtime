/*
    Copyright (C) 2018 Krzysztof Nowicki <krissn@op.pl>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "ewspasswordauth.h"

#include <KLocalizedString>

#include "ewsclient_debug.h"


EwsPasswordAuth::EwsPasswordAuth(const QString &username, QObject *parent)
    : EwsAbstractAuth(parent), mUsername(username)
{
}

void EwsPasswordAuth::init()
{
    Q_EMIT requestWalletPassword(false);
}

bool EwsPasswordAuth::getAuthData(QString &username, QString &password, QStringList &customHeaders)
{
    Q_UNUSED(customHeaders);

    if (!mPassword.isNull()) {
        username = mUsername;
        password = mPassword;
        return true;
    }
    return false;
}

void EwsPasswordAuth::notifyRequestAuthFailed()
{
    EwsAbstractAuth::notifyRequestAuthFailed();
}

bool EwsPasswordAuth::authenticate(bool interactive)
{
    Q_UNUSED(interactive);

    return false;
}

const QString &EwsPasswordAuth::reauthPrompt() const
{
    static const QString prompt;
    return prompt;
}

const QString &EwsPasswordAuth::authFailedPrompt() const
{
    static const QString prompt = i18nc("@info", "The username/password for the Microsoft Exchange EWS account <b>%1</b> is not valid. Please update it in the account settings page.");
    return prompt;
}

void EwsPasswordAuth::walletPasswordRequestFinished(const QString &password)
{
    mPassword = password;

    if (!password.isNull()) {
        Q_EMIT authSucceeded();
    } else {
        Q_EMIT authFailed(QStringLiteral("Password request failed"));
    }
}

void EwsPasswordAuth::walletMapRequestFinished(const QMap<QString, QString> &map)
{
    Q_UNUSED(map);
}

void EwsPasswordAuth::setUsername(const QString &username)
{
    mUsername = username;
}
