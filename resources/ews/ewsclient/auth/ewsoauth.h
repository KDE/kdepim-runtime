/*
    SPDX-FileCopyrightText: 2018 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef EWSOAUTH_H
#define EWSOAUTH_H

#include <QObject>
#include <QScopedPointer>

#include "ewsabstractauth.h"

class EwsOAuthPrivate;

class EwsOAuth : public EwsAbstractAuth
{
    Q_OBJECT
public:
    EwsOAuth(QObject *parent, const QString &email, const QString &appId, const QString &redirectUri);
    ~EwsOAuth() override;

    void init() override;
    bool getAuthData(QString &username, QString &password, QStringList &customHeaders) override;
    void notifyRequestAuthFailed() override;
    bool authenticate(bool interactive) override;
    const QString &reauthPrompt() const override;
    const QString &authFailedPrompt() const override;

    void walletPasswordRequestFinished(const QString &password) override;
    void walletMapRequestFinished(const QMap<QString, QString> &map) override;

private:
    QScopedPointer<EwsOAuthPrivate> d_ptr;
    Q_DECLARE_PRIVATE(EwsOAuth)
};

#endif /* EWSOAUTH_H */
