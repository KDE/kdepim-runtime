/*
    SPDX-FileCopyrightText: 2018 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "ewsjob.h"

#include <QScopedPointer>
#include <QUrl>

class QNetworkAccessManager;
class QNetworkReply;

class EwsPKeyAuthJob : public EwsJob
{
    Q_OBJECT
public:
    explicit EwsPKeyAuthJob(const QUrl &pkeyUri, const QString &certFile, const QString &keyFile, const QString &keyPassword, QObject *parent);
    ~EwsPKeyAuthJob() override;

    const QUrl &resultUri() const;
    void start() override;

    [[nodiscard]] QString getAuthHeader();

private:
    [[nodiscard]] QByteArray buildAuthResponse(const QMap<QString, QString> &params);
    void sendAuthRequest(const QByteArray &respToken, const QUrl &submitUrl, const QString &context);
    void authRequestFinished();

    const QUrl mPKeyUri;
    const QString mCertFile;
    const QString mKeyFile;
    const QString mKeyPassword;

    QScopedPointer<QNetworkAccessManager> mNetworkAccessManager;
    QScopedPointer<QNetworkReply> mAuthReply;

    QUrl mResultUri;
};
