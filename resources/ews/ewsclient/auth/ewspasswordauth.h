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
#ifndef EWSPASSWORDAUTH_H
#define EWSPASSWORDAUTH_H

#include "ewsabstractauth.h"

class EwsPasswordAuth : public EwsAbstractAuth
{
    Q_OBJECT
public:
    EwsPasswordAuth(const QString &username, QObject *parent = nullptr);
    ~EwsPasswordAuth() override = default;

    void init() override;
    bool getAuthData(QString &username, QString &password, QStringList &customHeaders) override;
    void notifyRequestAuthFailed() override;
    bool authenticate(bool interactive) override;
    const QString &reauthPrompt() const override;
    const QString &authFailedPrompt() const override;

    void walletPasswordRequestFinished(const QString &password) override;
    void walletMapRequestFinished(const QMap<QString, QString> &map) override;

    void setUsername(const QString &username);
protected:
    QString mUsername;
    QString mPassword;
};

#endif /* EWSPASSWORDAUTH_H */
