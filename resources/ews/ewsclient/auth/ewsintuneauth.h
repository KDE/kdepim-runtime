/*
    SPDX-FileCopyrightText: 2025 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

class QObject;
#include "ewsabstractauth.h"

#include <memory>

class EwsInTuneAuthPrivate;

class EwsInTuneAuth : public EwsAbstractAuth
{
    Q_OBJECT
public:
    EwsInTuneAuth(QObject *parent, const QString &email, const QString &appId, const QString &redirectUri);
    ~EwsInTuneAuth() override;

    void init() override;
    bool getAuthData(QString &username, QString &password, QHash<QByteArray, QByteArray> &customHeaders) override;
    void notifyRequestAuthFailed() override;
    bool authenticate(bool interactive) override;
    const QString &reauthPrompt() const override;
    const QString &authFailedPrompt() const override;

    void walletPasswordRequestFinished(const QString &password) override;
    void walletMapRequestFinished(const QMap<QString, QString> &map) override;

private:
    std::unique_ptr<EwsInTuneAuthPrivate> const d_ptr;
    Q_DECLARE_PRIVATE(EwsInTuneAuth)
};
