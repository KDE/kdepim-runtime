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

#include "ewsoauth.h"

#include <QAbstractOAuthReplyHandler>
#include <QDialog>
#include <QHBoxLayout>
#include <QIcon>
#include <QNetworkReply>
#include <QOAuth2AuthorizationCodeFlow>
#include <QOAuthOobReplyHandler>
#include <QPointer>
#include <QUrlQuery>
#include <QWebEngineProfile>
#include <QWebEngineUrlRequestInterceptor>
#include <QWebEngineUrlRequestJob>
#include <QWebEngineUrlSchemeHandler>
#include <QWebEngineView>

#include "ewsclient_debug.h"

static const auto o365AuthorizationUrl = QUrl(QStringLiteral("https://login.microsoftonline.com/common/oauth2/authorize"));
static const auto o365AccessTokenUrl = QUrl(QStringLiteral("https://login.microsoftonline.com/common/oauth2/token"));
static const auto o365FakeUserAgent = QStringLiteral("Mozilla/5.0 (Linux; Android 7.0; SM-G930V Build/NRD90M) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/59.0.3071.125 Mobile Safari/537.36");
static const auto o365Resource = QStringLiteral("https%3A%2F%2Foutlook.office365.com%2F");

class EwsOAuthUrlSchemeHandler final : public QWebEngineUrlSchemeHandler
{
    Q_OBJECT
public:
    EwsOAuthUrlSchemeHandler(QObject *parent = Q_NULLPTR) : QWebEngineUrlSchemeHandler(parent) {};
    ~EwsOAuthUrlSchemeHandler() override = default;
    void requestStarted(QWebEngineUrlRequestJob *request) override;
signals:
    void returnUriReceived(QUrl url);
};

class EwsOAuthReplyHandler final : public QOAuthOobReplyHandler
{
    Q_OBJECT
public:
    EwsOAuthReplyHandler(QObject *parent, const QString &returnUri)
        : QOAuthOobReplyHandler(parent), mReturnUri(returnUri) {};
    ~EwsOAuthReplyHandler() override = default;

    QString callback() const override { return mReturnUri; };
private:
    const QString mReturnUri;
};

class EwsOAuthRequestInterceptor final : public QWebEngineUrlRequestInterceptor
{
    Q_OBJECT
public:
    EwsOAuthRequestInterceptor(QObject *parent, const QString &redirectUri)
        : QWebEngineUrlRequestInterceptor(parent), mRedirectUri(redirectUri)
    {
    };

    ~EwsOAuthRequestInterceptor() override = default;

    void interceptRequest(QWebEngineUrlRequestInfo &info) override;
Q_SIGNALS:
    void redirectUriIntercepted(const QUrl &url);
private:
    const QString mRedirectUri;
};

class EwsOAuthPrivate final : public QObject
{
    Q_OBJECT
public:
    EwsOAuthPrivate(EwsOAuth *parent, const QString &email, const QString &appId, const QString &redirectUri);
    ~EwsOAuthPrivate() override = default;

    void authenticate();

    void modifyParametersFunction(QAbstractOAuth::Stage stage, QVariantMap *parameters);
    void authorizeWithBrowser(const QUrl &url);
    void redirectUriIntercepted(const QUrl &url);
    void granted();
    void error(const QString &error, const QString &errorDescription, const QUrl &uri);

    QWebEngineView mWebView;
    QWebEngineProfile mWebProfile;
    QWebEnginePage mWebPage;
    QOAuth2AuthorizationCodeFlow mOAuth2;
    EwsOAuthReplyHandler mReplyHandler;
    EwsOAuthRequestInterceptor mRequestInterceptor;
    EwsOAuthUrlSchemeHandler mSchemeHandler;
    QString mToken;
    const QString mEmail;
    const QString mRedirectUri;
    EwsOAuth::State mState;
    QWidget *mParentWindow;
    QPointer<QDialog> mWebDialog;

    EwsOAuth *q_ptr = nullptr;
    Q_DECLARE_PUBLIC(EwsOAuth)
};


void EwsOAuthUrlSchemeHandler::requestStarted(QWebEngineUrlRequestJob *request)
{
    returnUriReceived(request->requestUrl());
}


void EwsOAuthRequestInterceptor::interceptRequest(QWebEngineUrlRequestInfo &info)
{
    const auto url = info.requestUrl();

    qCDebug(EWSCLI_LOG) << QStringLiteral("Intercepted browser navigation to ") << url;

    if (url.toString(QUrl::RemoveQuery) == mRedirectUri) {
        qCDebug(EWSCLI_LOG) << QStringLiteral("Found redirect URI - blocking request");

        redirectUriIntercepted(url);
        info.block(true);
    }
}

EwsOAuthPrivate::EwsOAuthPrivate(EwsOAuth *parent, const QString &email, const QString &appId, const QString &redirectUri)
    : QObject(nullptr), mWebView(nullptr), mWebProfile(), mWebPage(&mWebProfile), mReplyHandler(this, redirectUri),
      mRequestInterceptor(this, redirectUri), mEmail(email), mRedirectUri(redirectUri), mState(EwsOAuth::NotAuthenticated),
      mParentWindow(nullptr), q_ptr(parent)
{
    mOAuth2.setReplyHandler(&mReplyHandler);
    mOAuth2.setAuthorizationUrl(o365AuthorizationUrl);
    mOAuth2.setAccessTokenUrl(o365AccessTokenUrl);
    mOAuth2.setClientIdentifier(appId);

    /* Bad bad Microsoft...
     * When Conditional Access is enabled on the server the OAuth2 authentication server only supports Windows,
     * MacOSX, Android and iOS. No option to include Linux. Support (i.e. guarantee that it works)
     * is one thing, but blocking unsupported browsers completely is just wrong.
     * Fortunately enough this can be worked around by faking the user agent to something "supported".
     */
    mWebProfile.setHttpUserAgent(o365FakeUserAgent);

    mWebProfile.setRequestInterceptor(&mRequestInterceptor);
    mWebProfile.installUrlSchemeHandler("urn", &mSchemeHandler);

    mWebView.setPage(&mWebPage);

    mOAuth2.setModifyParametersFunction([&](QAbstractOAuth::Stage stage, QVariantMap *parameters) {
        modifyParametersFunction(stage, parameters);
    });
    connect(&mOAuth2, &QOAuth2AuthorizationCodeFlow::authorizeWithBrowser, this, &EwsOAuthPrivate::authorizeWithBrowser);
    connect(&mOAuth2, &QOAuth2AuthorizationCodeFlow::granted, this, &EwsOAuthPrivate::granted);
    connect(&mOAuth2, &QOAuth2AuthorizationCodeFlow::error, this, &EwsOAuthPrivate::error);
    connect(&mRequestInterceptor, &EwsOAuthRequestInterceptor::redirectUriIntercepted, this,
        &EwsOAuthPrivate::redirectUriIntercepted, Qt::QueuedConnection);
}

void EwsOAuthPrivate::authenticate()
{
    qCInfoNC(EWSCLI_LOG) << QStringLiteral("Starting OAuth2 authentication");

    mState = EwsOAuth::Authenticating;

    if (!mOAuth2.refreshToken().isEmpty()) {
        mOAuth2.refreshAccessToken();
    } else {
        mOAuth2.grant();
    }
}

void EwsOAuthPrivate::modifyParametersFunction(QAbstractOAuth::Stage stage, QVariantMap *parameters)
{
    switch (stage) {
    case QAbstractOAuth::Stage::RequestingAccessToken:
        parameters->insert(QStringLiteral("resource"), o365Resource);
        break;
    case QAbstractOAuth::Stage::RequestingAuthorization:
        parameters->insert(QStringLiteral("prompt"), QStringLiteral("login"));
        parameters->insert(QStringLiteral("login_hint"), mEmail);
        parameters->insert(QStringLiteral("resource"), o365Resource);
        break;
    default:
        break;
    }
}

void EwsOAuthPrivate::authorizeWithBrowser(const QUrl &url)
{
    qCInfoNC(EWSCLI_LOG) << QStringLiteral("Launching browser for authentication");

    mWebDialog = new QDialog(mParentWindow);
    mWebDialog->setObjectName(QStringLiteral("Akonadi EWS Resource - Authentication"));
    mWebDialog->setWindowIcon(QIcon("akonadi-ews"));
    mWebDialog->resize(400, 500);
    auto layout = new QHBoxLayout(mWebDialog);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(&mWebView);
    mWebView.show();

    mWebView.load(url);
    mWebDialog->show();
}

void EwsOAuthPrivate::redirectUriIntercepted(const QUrl &url)
{
    qCDebug(EWSCLI_LOG) << QStringLiteral("Intercepted redirect URI from browser");

    QUrlQuery query(url);
    QVariantMap varmap;
    for (const auto item : query.queryItems()) {
        varmap[item.first] = item.second;
    }
    mOAuth2.authorizationCallbackReceived(varmap);
    mWebView.stop();
    mWebDialog->hide();
}

void EwsOAuthPrivate::granted()
{
    Q_Q(EwsOAuth);

    mState = EwsOAuth::Authenticated;

    // TODO: Store refreshUri

    qCInfo(EWSCLI_LOG) << QStringLiteral("Authentication succeeded");

    Q_EMIT(q->granted());
}

void EwsOAuthPrivate::error(const QString &error, const QString &errorDescription, const QUrl &uri)
{
    Q_Q(EwsOAuth);

    if (!mOAuth2.refreshToken().isEmpty()) {
        qCInfoNC(EWSCLI_LOG) << QStringLiteral("Refresh token failed. Falling back to full authentication: ")
                             << error << errorDescription;
        mOAuth2.setRefreshToken(QString());
        mOAuth2.grant();
    }

    mState = EwsOAuth::AuthenticationFailed;

    qCInfoNC(EWSCLI_LOG) << QStringLiteral("Authentication failed: ") << error << errorDescription;

    Q_EMIT(q->error(error, errorDescription, uri));
}

EwsOAuth::EwsOAuth(QObject *parent, const QString &email, const QString &appId, const QString &redirectUri)
    : QObject(parent), d_ptr(new EwsOAuthPrivate(this, email, appId, redirectUri))
{
}

EwsOAuth::~EwsOAuth()
{
}

void EwsOAuth::authenticate()
{
    Q_D(EwsOAuth);

    d->authenticate();
}

QString EwsOAuth::token() const
{
    Q_D(const EwsOAuth);

    return d->mOAuth2.token();
}

QString EwsOAuth::refreshToken() const
{
    Q_D(const EwsOAuth);

    return d->mOAuth2.refreshToken();
}

EwsOAuth::State EwsOAuth::state() const
{
    Q_D(const EwsOAuth);

    return d->mState;
}

void EwsOAuth::setParentWindow(QWidget *window)
{
    Q_D(EwsOAuth);

    d->mParentWindow = window;
}

void EwsOAuth::setAccessToken(const QString &accessToken)
{
    Q_D(EwsOAuth);

    d->mOAuth2.setToken(accessToken);
    d->mState = Authenticated;
}

void EwsOAuth::setRefreshToken(const QString &refreshToken)
{
    Q_D(EwsOAuth);

    d->mOAuth2.setRefreshToken(refreshToken);
}

void EwsOAuth::resetAccessToken()
{
    Q_D(EwsOAuth);

    d->mOAuth2.setToken(QString());
    d->mState = NotAuthenticated;
}

#include "ewsoauth.moc"
