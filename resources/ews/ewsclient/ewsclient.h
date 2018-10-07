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

#include <QString>
#include <QUrl>
#include <QObject>
#include "ewsserverversion.h"

#ifdef HAVE_NETWORKAUTH
#include <QPointer>
class EwsOAuth;
#endif

class EwsClient : public QObject
{
    Q_OBJECT
public:
    enum AuthMode {
        Unknown,
        UsernamePassword,
#ifdef HAVE_NETWORKAUTH
        OAuth2,
#endif
    };

    explicit EwsClient(QObject *parent = nullptr);
    ~EwsClient();

    AuthMode authMode() const
    {
        return mAuthMode;
    }

    void setUrl(const QString &url)
    {
        mUrl.setUrl(url);
    }

    void setCredentials(const QString &username, const QString &password)
    {
        mUrl.setUserName(username);
        mUrl.setPassword(password);

        mAuthMode = UsernamePassword;
    }

#ifdef HAVE_NETWORKAUTH
    void setOAuthData(const QString &email, const QString &appId, const QString &redirectUri);
    void setOAuthTokens(const QString &authToken, const QString &refreshToken);
    EwsOAuth *oAuth();
#endif

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

private:
    AuthMode mAuthMode;

    QUrl mUrl;
    QString mUsername;
    QString mPassword;

#ifdef HAVE_NETWORKAUTH
    QString mEmail;
    QString mAppId;
    QString mRedirectUri;
    QString mAccessToken;
    QString mRefreshToken;
    QPointer<EwsOAuth> mOAuth;
#endif

    QString mUserAgent;
    bool mEnableNTLMv2;

    EwsServerVersion mServerVersion;

    friend class EwsRequest;
};

#endif
