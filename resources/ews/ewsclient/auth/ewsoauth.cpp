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

#include <QUrlQuery>
#ifdef EWSOAUTH_UNITTEST
#include "ewsoauth_ut_mock.h"
using namespace Mock;
#else
#include <QAbstractOAuthReplyHandler>
#include <QDialog>
#include <QHBoxLayout>
#include <QIcon>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QOAuth2AuthorizationCodeFlow>
#include <QPointer>
#include <QWebEngineProfile>
#include <QWebEngineUrlRequestInterceptor>
#include <QWebEngineUrlRequestJob>
#include <QWebEngineUrlSchemeHandler>
#include <QWebEngineView>
#include <KLocalizedString>
#ifdef HAVE_QCA
#include "ewspkeyauthjob.h"
#endif
#endif

#include "ewsclient_debug.h"

static const auto o365AuthorizationUrl = QUrl(QStringLiteral("https://login.microsoftonline.com/common/oauth2/authorize"));
static const auto o365AccessTokenUrl = QUrl(QStringLiteral("https://login.microsoftonline.com/common/oauth2/token"));
static const auto o365FakeUserAgent = QStringLiteral("Mozilla/5.0 (Linux; Android 7.0; SM-G930V Build/NRD90M) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/59.0.3071.125 Mobile Safari/537.36");
static const auto o365Resource = QStringLiteral("https%3A%2F%2Foutlook.office365.com%2F");

#ifdef HAVE_QCA
static const auto pkeyAuthSuffix = QStringLiteral(" PKeyAuth/1.0");
static const auto pkeyRedirectUri = QStringLiteral("urn:http-auth:PKeyAuth");
static const QString pkeyPasswordMapKey = QStringLiteral("pkey-password");
#endif

static const QString accessTokenMapKey = QStringLiteral("access-token");
static const QString refreshTokenMapKey = QStringLiteral("refresh-token");

class EwsOAuthUrlSchemeHandler final : public QWebEngineUrlSchemeHandler
{
    Q_OBJECT
public:
    EwsOAuthUrlSchemeHandler(QObject *parent = nullptr) : QWebEngineUrlSchemeHandler(parent)
    {
    }

    ~EwsOAuthUrlSchemeHandler() override = default;
    void requestStarted(QWebEngineUrlRequestJob *request) override;
Q_SIGNALS:
    void returnUriReceived(const QUrl &url);
};

class EwsOAuthReplyHandler final : public QAbstractOAuthReplyHandler
{
    Q_OBJECT
public:
    EwsOAuthReplyHandler(QObject *parent, const QString &returnUri)
        : QAbstractOAuthReplyHandler(parent)
        , mReturnUri(returnUri)
    {
    }

    ~EwsOAuthReplyHandler() override = default;

    QString callback() const override
    {
        return mReturnUri;
    }

    void networkReplyFinished(QNetworkReply *reply) override;
Q_SIGNALS:
    void replyError(const QString &error);
private:
    const QString mReturnUri;
};

class EwsOAuthRequestInterceptor final : public QWebEngineUrlRequestInterceptor
{
    Q_OBJECT
public:
    EwsOAuthRequestInterceptor(QObject *parent, const QString &redirectUri)
        : QWebEngineUrlRequestInterceptor(parent)
        , mRedirectUri(redirectUri)
    {
    }

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

    bool authenticate(bool interactive);

    void modifyParametersFunction(QAbstractOAuth::Stage stage, QVariantMap *parameters);
    void authorizeWithBrowser(const QUrl &url);
    void redirectUriIntercepted(const QUrl &url);
    void granted();
    void error(const QString &error, const QString &errorDescription, const QUrl &uri);
    QVariantMap queryToVarmap(const QUrl &url);
#ifdef HAVE_QCA
    void pkeyAuthResult(KJob *job);
#endif

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
    bool mAuthenticated;
    QPointer<QDialog> mWebDialog;
#ifdef HAVE_QCA
    QString mPKeyPassword;
#endif

    EwsOAuth *q_ptr = nullptr;
    Q_DECLARE_PUBLIC(EwsOAuth)
};

void EwsOAuthUrlSchemeHandler::requestStarted(QWebEngineUrlRequestJob *request)
{
    returnUriReceived(request->requestUrl());
}

void EwsOAuthReplyHandler::networkReplyFinished(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        Q_EMIT replyError(reply->errorString());
        return;
    } else if (reply->header(QNetworkRequest::ContentTypeHeader).isNull()) {
        Q_EMIT replyError(QStringLiteral("Empty or no Content-type header"));
        return;
    }
    const auto cth = reply->header(QNetworkRequest::ContentTypeHeader);
    const auto ct = cth.isNull() ? QStringLiteral("text/html") : cth.toString();
    const auto data = reply->readAll();
    if (data.isEmpty()) {
        Q_EMIT replyError(QStringLiteral("No data received"));
        return;
    }
    Q_EMIT replyDataReceived(data);
    QVariantMap tokens;
    if (ct.startsWith(QStringLiteral("text/html"))
        || ct.startsWith(QStringLiteral("application/x-www-form-urlencoded"))) {
        QUrlQuery q(QString::fromUtf8(data));
        const auto items = q.queryItems(QUrl::FullyDecoded);
        for (const auto &it : items) {
            tokens.insert(it.first, it.second);
        }
    } else if (ct.startsWith(QStringLiteral("application/json"))
               || ct.startsWith(QStringLiteral("text/javascript"))) {
        const auto document = QJsonDocument::fromJson(data);
        if (!document.isObject()) {
            Q_EMIT replyError(QStringLiteral("Invalid JSON data received"));
            return;
        }
        const auto object = document.object();
        if (object.isEmpty()) {
            Q_EMIT replyError(QStringLiteral("Empty JSON data received"));
            return;
        }
        tokens = object.toVariantMap();
    } else {
        Q_EMIT replyError(QStringLiteral("Unknown content type"));
        return;
    }

    const auto error = tokens.value(QStringLiteral("error"));
    if (error.isValid()) {
        Q_EMIT replyError(QStringLiteral("Received error response: ") + error.toString());
        return;
    }
    const auto accessToken = tokens.value(QStringLiteral("access_token"));
    if (!accessToken.isValid() || accessToken.toString().isEmpty()) {
        Q_EMIT replyError(QStringLiteral("Received empty or no access token"));
        return;
    }

    Q_EMIT tokensReceived(tokens);
}

void EwsOAuthRequestInterceptor::interceptRequest(QWebEngineUrlRequestInfo &info)
{
    const auto url = info.requestUrl();

    qCDebugNC(EWSCLI_LOG) << QStringLiteral("Intercepted browser navigation to ") << url;

    if ((url.toString(QUrl::RemoveQuery) == mRedirectUri)
#ifdef HAVE_QCA
        || (url.toString(QUrl::RemoveQuery) == pkeyRedirectUri)
#endif
        ) {
        qCDebug(EWSCLI_LOG) << QStringLiteral("Found redirect URI - blocking request");

        redirectUriIntercepted(url);
        info.block(true);
    }
}

EwsOAuthPrivate::EwsOAuthPrivate(EwsOAuth *parent, const QString &email, const QString &appId, const QString &redirectUri)
    : QObject(nullptr)
    , mWebView(nullptr)
    , mWebProfile()
    , mWebPage(&mWebProfile)
    , mReplyHandler(this, redirectUri)
    , mRequestInterceptor(this, redirectUri)
    , mEmail(email)
    , mRedirectUri(redirectUri)
    , mAuthenticated(false)
    , q_ptr(parent)
{
    mOAuth2.setReplyHandler(&mReplyHandler);
    mOAuth2.setAuthorizationUrl(o365AuthorizationUrl);
    mOAuth2.setAccessTokenUrl(o365AccessTokenUrl);
    mOAuth2.setClientIdentifier(appId);
#if QT_VERSION < QT_VERSION_CHECK(5, 13, 0)
    mWebProfile.setRequestInterceptor(&mRequestInterceptor);
#else
    mWebProfile.setUrlRequestInterceptor(&mRequestInterceptor);
#endif
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
    connect(&mReplyHandler, &EwsOAuthReplyHandler::replyError, this, [this](const QString &err) {
        error(QStringLiteral("Network reply error"), err, QUrl());
    });
}

bool EwsOAuthPrivate::authenticate(bool interactive)
{
    Q_Q(EwsOAuth);

    qCInfoNC(EWSCLI_LOG) << QStringLiteral("Starting OAuth2 authentication");

    if (!mOAuth2.refreshToken().isEmpty()) {
        mOAuth2.refreshAccessToken();
        return true;
    } else if (interactive) {
        mOAuth2.grant();
        return true;
    } else {
        return false;
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
    Q_Q(EwsOAuth);

    qCInfoNC(EWSCLI_LOG) << QStringLiteral("Launching browser for authentication");

    /* Bad bad Microsoft...
     * When Conditional Access is enabled on the server the OAuth2 authentication server only supports Windows,
     * MacOSX, Android and iOS. No option to include Linux. Support (i.e. guarantee that it works)
     * is one thing, but blocking unsupported browsers completely is just wrong.
     * Fortunately enough this can be worked around by faking the user agent to something "supported".
     */
    auto userAgent = o365FakeUserAgent;
#ifdef HAVE_QCA
    if (!q->mPKeyCertFile.isNull() && !q->mPKeyKeyFile.isNull()) {
        qCInfoNC(EWSCLI_LOG) << QStringLiteral("Found PKeyAuth certificates");
        userAgent += pkeyAuthSuffix;
    } else {
        qCInfoNC(EWSCLI_LOG) << QStringLiteral("PKeyAuth certificates not found");
    }
#endif
    mWebProfile.setHttpUserAgent(userAgent);

    mWebDialog = new QDialog(q->mAuthParentWidget);
    mWebDialog->setObjectName(QStringLiteral("Akonadi EWS Resource - Authentication"));
    mWebDialog->setWindowIcon(QIcon(QStringLiteral("akonadi-ews")));
    mWebDialog->resize(400, 500);
    auto layout = new QHBoxLayout(mWebDialog);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(&mWebView);
    mWebView.show();

    connect(mWebDialog.data(), &QDialog::rejected, this, [this]() {
        error(QStringLiteral("User cancellation"), QStringLiteral("The authentication browser was closed"), QUrl());
    });

    mWebView.load(url);
    mWebDialog->show();
}

QVariantMap EwsOAuthPrivate::queryToVarmap(const QUrl &url)
{
    QUrlQuery query(url);
    QVariantMap varmap;
    for (const auto &item : query.queryItems()) {
        varmap[item.first] = item.second;
    }
    return varmap;
}

void EwsOAuthPrivate::redirectUriIntercepted(const QUrl &url)
{
    qCDebugNC(EWSCLI_LOG) << QStringLiteral("Intercepted redirect URI from browser: ") << url;

    mWebView.stop();
    mWebDialog->hide();

#ifdef HAVE_QCA
    Q_Q(EwsOAuth);
    if (url.toString(QUrl::RemoveQuery) == pkeyRedirectUri) {
        qCDebugNC(EWSCLI_LOG) << QStringLiteral("Found PKeyAuth URI");

        auto pkeyAuthJob = new EwsPKeyAuthJob(url, q->mPKeyCertFile, q->mPKeyKeyFile, mPKeyPassword, this);

        connect(pkeyAuthJob, &KJob::result, this, &EwsOAuthPrivate::pkeyAuthResult);

        pkeyAuthJob->start();

        return;
    }
#endif
    mOAuth2.authorizationCallbackReceived(queryToVarmap(url));
}

#ifdef HAVE_QCA
void EwsOAuthPrivate::pkeyAuthResult(KJob *j)
{
    EwsPKeyAuthJob *job = qobject_cast<EwsPKeyAuthJob *>(j);

    qCDebugNC(EWSCLI_LOG) << QStringLiteral("PKeyAuth result: %1").arg(job->error());
    QVariantMap varmap;
    if (job->error() == 0) {
        varmap = queryToVarmap(job->resultUri());
    } else {
        varmap[QStringLiteral("error")] = job->errorString();
    }
    mOAuth2.authorizationCallbackReceived(varmap);
}

#endif

void EwsOAuthPrivate::granted()
{
    Q_Q(EwsOAuth);

    qCInfoNC(EWSCLI_LOG) << QStringLiteral("Authentication succeeded");

    mAuthenticated = true;

    QMap<QString, QString> map;
    map[accessTokenMapKey] = mOAuth2.token();
    map[refreshTokenMapKey] = mOAuth2.refreshToken();
    Q_EMIT q->setWalletMap(map);

    Q_EMIT q->authSucceeded();
}

void EwsOAuthPrivate::error(const QString &error, const QString &errorDescription, const QUrl &uri)
{
    Q_Q(EwsOAuth);

    Q_UNUSED(uri);

    mAuthenticated = false;

    mOAuth2.setRefreshToken(QString());
    qCInfoNC(EWSCLI_LOG) << QStringLiteral("Authentication failed: ") << error << errorDescription;

    Q_EMIT q->authFailed(error);
}

EwsOAuth::EwsOAuth(QObject *parent, const QString &email, const QString &appId, const QString &redirectUri)
    : EwsAbstractAuth(parent)
    , d_ptr(new EwsOAuthPrivate(this, email, appId, redirectUri))
{
}

EwsOAuth::~EwsOAuth()
{
}

void EwsOAuth::init()
{
    requestWalletMap();
}

bool EwsOAuth::getAuthData(QString &username, QString &password, QStringList &customHeaders)
{
    Q_D(const EwsOAuth);

    Q_UNUSED(username);
    Q_UNUSED(password);

    if (d->mAuthenticated) {
        customHeaders.append(QStringLiteral("Authorization: Bearer ") + d->mOAuth2.token());
        return true;
    } else {
        return false;
    }
}

void EwsOAuth::notifyRequestAuthFailed()
{
    Q_D(EwsOAuth);

    d->mOAuth2.setToken(QString());
    d->mAuthenticated = false;

    EwsAbstractAuth::notifyRequestAuthFailed();
}

bool EwsOAuth::authenticate(bool interactive)
{
    Q_D(EwsOAuth);

    return d->authenticate(interactive);
}

const QString &EwsOAuth::reauthPrompt() const
{
    static const QString prompt = i18nc("@info", "Microsoft Exchange credentials for the account <b>%1</b> are no longer valid. You need to authenticate in order to continue using it.");
    return prompt;
}

const QString &EwsOAuth::authFailedPrompt() const
{
    static const QString prompt = i18nc("@info", "Failed to obtain credentials for Microsoft Exchange account <b>%1</b>. Please update it in the account settings page.");
    return prompt;
}

void EwsOAuth::walletPasswordRequestFinished(const QString &password)
{
    Q_UNUSED(password);
}

void EwsOAuth::walletMapRequestFinished(const QMap<QString, QString> &map)
{
    Q_D(EwsOAuth);

#ifdef HAVE_QCA
    if (map.contains(pkeyPasswordMapKey)) {
        d->mPKeyPassword = map[pkeyPasswordMapKey];
    }
#endif
    if (map.contains(refreshTokenMapKey)) {
        d->mOAuth2.setRefreshToken(map[refreshTokenMapKey]);
    }
    if (map.contains(accessTokenMapKey)) {
        d->mOAuth2.setToken(map[accessTokenMapKey]);
        d->mAuthenticated = true;
        Q_EMIT authSucceeded();
    } else {
        Q_EMIT authFailed(QStringLiteral("Access token request failed"));
    }
}

#include "ewsoauth.moc"
