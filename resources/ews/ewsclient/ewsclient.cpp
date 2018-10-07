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

#include "ewsclient.h"

#include <QString>

#ifdef HAVE_NETWORKAUTH
#include "ewsoauth.h"
#endif
#include "ewsclient_debug.h"

QHash<QString, QString> EwsClient::folderHash;

EwsClient::EwsClient(QObject *parent)
    : QObject(parent), mAuthMode(Unknown), mEnableNTLMv2(true)
{

}

EwsClient::~EwsClient()
{
}

void EwsClient::setServerVersion(const EwsServerVersion &version)
{
    if (mServerVersion.isValid() && mServerVersion != version) {
        qCWarning(EWSCLI_LOG) << "Warning - server version changed." << mServerVersion << version;
    }
    mServerVersion = version;
}

QUrl EwsClient::url() const
{
    auto url = mUrl;
#ifdef HAVE_NETWORKAUTH
    if (mAuthMode == OAuth2) {
        url.setUserInfo(QString());
    }
#endif
    return url;
}

#ifdef HAVE_NETWORKAUTH
EwsOAuth *EwsClient::oAuth()
{
    if (mAuthMode == OAuth2 && !mOAuth) {
        mOAuth = new EwsOAuth(this, mEmail, mAppId, mRedirectUri);
        if (!mAccessToken.isEmpty()) {
            mOAuth->setAccessToken(mAccessToken);
        }
        if (!mRefreshToken.isEmpty()) {
            mOAuth->setRefreshToken(mRefreshToken);
        }
        connect(mOAuth, &EwsOAuth::granted, this, [this]() {
                mAccessToken = mOAuth->token();
                mRefreshToken = mOAuth->refreshToken();
                Q_EMIT oAuthTokensChanged(mAccessToken, mRefreshToken);
            });
    }
    return mOAuth;
}

void EwsClient::setOAuthData(const QString &email, const QString &appId, const QString &redirectUri)
{
    mEmail = email;
    mAppId = appId;
    mRedirectUri = redirectUri;

    mAuthMode = OAuth2;
    delete mOAuth;
}

void EwsClient::setOAuthTokens(const QString &accessToken, const QString &refreshToken)
{
    mAccessToken = accessToken;
    mRefreshToken = refreshToken;
    if (mOAuth) {
        if (!mAccessToken.isEmpty()) {
            mOAuth->setAccessToken(mAccessToken);
        }
        if (!mRefreshToken.isEmpty()) {
            mOAuth->setRefreshToken(mRefreshToken);
        }
    }
}

#endif

