/*
    Copyright (C) 2015-2018 Krzysztof Nowicki <krissn@op.pl>

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

#ifndef EWSCLIENT_H
#define EWSCLIENT_H

#include <QObject>
#include <QPointer>
#include <QString>
#include <QUrl>

#include "ewsserverversion.h"

class EwsAbstractAuth;

class EwsClient : public QObject
{
    Q_OBJECT
public:
    explicit EwsClient(QObject *parent = nullptr);
    ~EwsClient();

    void setUrl(const QString &url)
    {
        mUrl.setUrl(url);
    }

    void setAuth(EwsAbstractAuth *auth);
    EwsAbstractAuth *auth() const;

    enum RequestedConfiguration {
        MailTips = 0,
        UnifiedMessagingConfiguration,
        ProtectionRules
    };

    QUrl url() const;

    bool isConfigured() const
    {
        return !mUrl.isEmpty();
    }

    void setServerVersion(const EwsServerVersion &version);
    const EwsServerVersion &serverVersion() const
    {
        return mServerVersion;
    }

    void setUserAgent(const QString &userAgent)
    {
        mUserAgent = userAgent;
    }
    const QString &userAgent() const
    {
        return mUserAgent;
    }

    void setEnableNTLMv2(bool enable)
    {
        mEnableNTLMv2 = enable;
    }
    bool isNTLMv2Enabled() const
    {
        return mEnableNTLMv2;
    }

    static QHash<QString, QString> folderHash;
Q_SIGNALS:
    void oAuthTokensChanged(const QString &accessToken, const QString &refreshToken);
    void oAuthBrowserDisplayRequest();

private:
    QUrl mUrl;

    QPointer<EwsAbstractAuth> mAuth;

    QString mUserAgent;
    bool mEnableNTLMv2;

    EwsServerVersion mServerVersion;

    friend class EwsRequest;
};

#endif
