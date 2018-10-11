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

#ifndef EWSOAUTH_UT_MOCK_H
#define EWSOAUTH_UT_MOCK_H

#include <functional>

#include <QBuffer>
#include <QDialog>
#include <QHBoxLayout>
#include <QIcon>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QPointer>
#include <QUrl>
#include <QWidget>

Q_DECLARE_LOGGING_CATEGORY(EWSCLI_LOG)

namespace Mock {

class QWebEngineUrlRequestJob : public QObject
{
    Q_OBJECT
public:
    QWebEngineUrlRequestJob(const QUrl &url, QObject *parent) : QObject(parent), mUrl(url) {};
    ~QWebEngineUrlRequestJob() override = default;

    QUrl requestUrl() const;
    
    QUrl mUrl;
};

class QWebEngineUrlRequestInfo : public QObject
{
    Q_OBJECT
public:
    QWebEngineUrlRequestInfo(const QUrl &url, QObject *parent) : QObject(parent), mBlocked(false), mUrl(url) {};
    ~QWebEngineUrlRequestInfo() override = default;

    QUrl requestUrl() const;
    void block(bool shouldBlock);

    bool mBlocked;
    QUrl mUrl;
};

class QWebEngineUrlRequestInterceptor : public QObject
{
    Q_OBJECT
public:
    QWebEngineUrlRequestInterceptor(QObject *parent);
    ~QWebEngineUrlRequestInterceptor() override;

    virtual void interceptRequest(QWebEngineUrlRequestInfo &info) = 0;
};

class QWebEngineUrlSchemeHandler : public QObject
{
    Q_OBJECT
public:
    QWebEngineUrlSchemeHandler(QObject *parent);
    ~QWebEngineUrlSchemeHandler() override;

    virtual void requestStarted(QWebEngineUrlRequestJob *request) = 0;
};

class QWebEngineProfile : public QObject
{
    Q_OBJECT
public:
    QWebEngineProfile(QObject *parent = nullptr);
    ~QWebEngineProfile() override;

    void setHttpUserAgent(const QString &ua);
    void setRequestInterceptor(QWebEngineUrlRequestInterceptor *interceptor);
    void installUrlSchemeHandler(QByteArray const &scheme, QWebEngineUrlSchemeHandler *handler);
Q_SIGNALS:
    void logEvent(const QString &event);
public:
    QString mUserAgent;
    QWebEngineUrlRequestInterceptor *mInterceptor;
    QString mScheme;
    QWebEngineUrlSchemeHandler *mHandler;
};

class QWebEnginePage : public QObject
{
    Q_OBJECT
public:
    QWebEnginePage(QWebEngineProfile *profile, QObject *parent = nullptr);
    ~QWebEnginePage() override;
Q_SIGNALS:
    void logEvent(const QString &event);
public:
    QWebEngineProfile *mProfile;
};

class QWebEngineView : public QWidget
{
    Q_OBJECT
public:
    typedef std::function<void(const QUrl &, QVariantMap &)> AuthFunc;
    
    QWebEngineView(QWidget *parent);
    ~QWebEngineView() override;

    void load(const QUrl &url);
    void setPage(QWebEnginePage *page);
    void stop();

    void setAuthFunction(const AuthFunc &func);
    void setRedirectUri(const QString &uri);

    static QPointer<QWebEngineView> instance;
Q_SIGNALS:
    void logEvent(const QString &event);
protected:
    void simulatePageLoad(const QUrl &url);

    QWebEnginePage *mPage;
    QString mRedirectUri;
    AuthFunc mAuthFunction;
};

class QNetworkRequest
{
public:
    enum KnownHeaders {
        ContentTypeHeader
    };
};

class QNetworkReply : public QBuffer
{
    Q_OBJECT
public:
    enum NetworkError {
        NoError = 0,
    };
    Q_ENUM(NetworkError)

    QNetworkReply(QObject *parent) : QBuffer(parent) {};
    ~QNetworkReply() override = default;

    NetworkError error() const;
    QVariant header(QNetworkRequest::KnownHeaders header) const;

    QMap<QNetworkRequest::KnownHeaders, QVariant> mHeaders;
    NetworkError mError;
};

class QAbstractOAuthReplyHandler : public QObject
{
    Q_OBJECT
public:
    QAbstractOAuthReplyHandler(QObject *parent);
    ~QAbstractOAuthReplyHandler() override;

    virtual QString callback() const = 0;
    virtual void networkReplyFinished(QNetworkReply *reply) = 0;
Q_SIGNALS:
    void replyDataReceived(const QByteArray &data);
    void tokensReceived(const QVariantMap &tokens);
};

class QAbstractOAuth : public QObject
{
    Q_OBJECT
public:
    Q_ENUMS(Stage)

    enum Stage {
        RequestingTemporaryCredentials,
        RequestingAuthorization,
        RequestingAccessToken,
        RefreshingAccessToken
    };

    QAbstractOAuth(QObject *parent);
    ~QAbstractOAuth() override = default;

    void setReplyHandler(QAbstractOAuthReplyHandler *handler);
    void setAuthorizationUrl(const QUrl &url);
    void setClientIdentifier(const QString &identifier);
    void setModifyParametersFunction(const std::function<void (QAbstractOAuth::Stage, QMap<QString, QVariant>*)> &func);
    QString token() const;
    void setToken(const QString &token);
Q_SIGNALS:
    void authorizeWithBrowser(const QUrl &url);
    void granted();
    void logEvent(const QString &event);
protected:
    QAbstractOAuthReplyHandler *mReplyHandler;
    QUrl mAuthUrl;
    QString mClientId;
    std::function<void (QAbstractOAuth::Stage, QMap<QString, QVariant>*)> mModifyParamsFunc;
    QString mToken;
    QString mRefreshToken;
    QUrl mTokenUrl;
    QString mResource;
};

class QAbstractOAuth2 : public QAbstractOAuth
{
    Q_OBJECT
public:
    QAbstractOAuth2(QObject *parent);
    ~QAbstractOAuth2() override = default;

    QString refreshToken() const;
    void setRefreshToken(const QString &token);

Q_SIGNALS:
    void authorizationCallbackReceived(QMap<QString, QVariant> const &params);
    void error(const QString &error, const QString &errorDescription, const QUrl &uri);
};

class QOAuth2AuthorizationCodeFlow : public QAbstractOAuth2
{
    Q_OBJECT
public:
    typedef std::function<QNetworkReply::NetworkError(QString &, QMap<QNetworkRequest::KnownHeaders, QVariant> &)> TokenFunc;
    
    QOAuth2AuthorizationCodeFlow(QObject *parent = nullptr);
    ~QOAuth2AuthorizationCodeFlow() override;

    void setAccessTokenUrl(const QUrl &url);
    void grant();
    void refreshAccessToken();

    QString redirectUri() const;
    void setTokenFunction(const TokenFunc &func);
    void setState(const QString &state);

    static QUrlQuery mapToSortedQuery(QMap<QString, QVariant> const &map);

    static QPointer<QOAuth2AuthorizationCodeFlow> instance;
protected:
    void authCallbackReceived(QMap<QString, QVariant> const &params);
    void replyDataCallbackReceived(const QByteArray &data);
    void tokenCallbackReceived(const QVariantMap &tokens);

    TokenFunc mTokenFunc;
    QString mState;
};

QString browserDisplayRequestString();
QString modifyParamsAuthString(const QString &clientId, const QString &returnUri, const QString &state);
QString authUrlString(const QString &authUrl, const QString &clientId, const QString &returnUri,
                      const QString &email, const QString &resource, const QString &state);
QString authorizeWithBrowserString(const QString &url);
QString loadWebPageString(const QString &url);
QString interceptRequestString(const QString &url);
QString interceptRequestBlockedString(bool blocked);
QString authorizationCallbackReceivedString(const QString &code);
QString modifyParamsTokenString(const QString &clientId, const QString &returnUri, const QString &code);
QString networkReplyFinishedString(const QString &data);
QString replyDataCallbackString(const QString &data);
QString tokenCallbackString(const QString &accessToken, const QString &refreshToken, const QString &idToken,
                            quint64 time, unsigned int tokenLifetime, unsigned int extTokenLifetime,
                            const QString &resource);

}

#endif /* EWSOAUTH_UT_MOCK_H */
