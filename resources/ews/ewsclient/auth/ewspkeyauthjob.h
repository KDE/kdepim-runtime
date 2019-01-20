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

#ifndef EWSPKEYAUTHJOB_H
#define EWSPKEYAUTHJOB_H

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
private:
    QByteArray buildAuthResponse(const QMap<QString, QString> &params);
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

#endif /* EWSPKEYAUTHJOB_H */
