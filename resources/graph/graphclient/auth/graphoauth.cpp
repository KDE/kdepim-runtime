/*
    SPDX-FileCopyrightText: 2026 Malte Zilinski <malte@zilinski.eu>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "graphoauth.h"

#include "graphclient_debug.h"

#include <KLocalizedString>

#include <QDateTime>
#include <QDesktopServices>
#include <QHostAddress>
#include <QOAuth2AuthorizationCodeFlow>
#include <QOAuthHttpServerReplyHandler>
#include <QTimer>
#include <QUrl>

#include <qt6keychain/keychain.h>

#include <limits>

// v2.0 endpoints (EWS uses the legacy v1 endpoint with a `resource=` param; Graph uses v2 scopes).
static QString authorizationUrl(const QString &tenant)
{
    return QStringLiteral("https://login.microsoftonline.com/%1/oauth2/v2.0/authorize").arg(tenant);
}
static QString accessTokenUrl(const QString &tenant)
{
    return QStringLiteral("https://login.microsoftonline.com/%1/oauth2/v2.0/token").arg(tenant);
}

// Loopback redirect — http://localhost:53682/callback must be registered as a
// "Mobile and desktop applications" redirect URI in the Azure app registration.
static constexpr quint16 kCallbackPort = 53682;
static const auto kCallbackPath = QLatin1String("/callback");

static const auto kKeychainService = QLatin1String("akonadi_graph_resource");

GraphOAuth::GraphOAuth(const QString &tenantId, const QString &clientId, const QString &walletKey, QObject *parent)
    : QObject(parent)
    , mTenantId(tenantId.isEmpty() ? QStringLiteral("common") : tenantId)
    , mClientId(clientId)
    , mWalletKey(walletKey.isEmpty() ? QStringLiteral("refresh_token") : walletKey)
{
}

GraphOAuth::~GraphOAuth() = default;

QSet<QByteArray> GraphOAuth::graphScopes()
{
    // offline_access -> we get a refresh token for silent renewals.
    return {
        QByteArrayLiteral("offline_access"),
        QByteArrayLiteral("https://graph.microsoft.com/User.Read"),
        QByteArrayLiteral("https://graph.microsoft.com/Mail.ReadWrite"),
        QByteArrayLiteral("https://graph.microsoft.com/Mail.Send"),
        QByteArrayLiteral("https://graph.microsoft.com/Calendars.ReadWrite"),
        QByteArrayLiteral("https://graph.microsoft.com/Contacts.ReadWrite"),
        QByteArrayLiteral("https://graph.microsoft.com/Tasks.ReadWrite"),
    };
}

void GraphOAuth::setUpFlow()
{
    if (mFlow) {
        return;
    }
    mFlow = std::make_unique<QOAuth2AuthorizationCodeFlow>(this);
    mFlow->setAuthorizationUrl(QUrl(authorizationUrl(mTenantId)));
    mFlow->setTokenUrl(QUrl(accessTokenUrl(mTenantId)));
    mFlow->setClientIdentifier(mClientId);
    mFlow->setRequestedScopeTokens(graphScopes());

    // Qt 6.5+ adds PKCE (S256) automatically; only push the prompt parameter ourselves.
    mFlow->setModifyParametersFunction([](QAbstractOAuth::Stage stage, QMultiMap<QString, QVariant> *params) {
        if (stage == QAbstractOAuth::Stage::RequestingAuthorization) {
            params->insert(QStringLiteral("prompt"), QStringLiteral("select_account"));
        }
    });

    connect(mFlow.get(), &QOAuth2AuthorizationCodeFlow::authorizeWithBrowser, this, [](const QUrl &url) {
        qCInfo(GRAPHCLIENT_LOG) << "opening browser for login:" << url.toString();
        QDesktopServices::openUrl(url);
    });
    connect(mFlow.get(), &QAbstractOAuth::granted, this, &GraphOAuth::onGranted);
    connect(mFlow.get(), &QAbstractOAuth::requestFailed, this, &GraphOAuth::onRequestFailed);
}

void GraphOAuth::authenticate()
{
    // Test hook: bypass the whole flow with a caller-supplied bearer token
    // (handy for tests and headless debugging).
    if (qEnvironmentVariableIsSet("GRAPH_ACCESS_TOKEN")) {
        mEnvToken = qEnvironmentVariable("GRAPH_ACCESS_TOKEN");
        QMetaObject::invokeMethod(this, &GraphOAuth::ready, Qt::QueuedConnection);
        return;
    }

    setUpFlow();

    auto job = new QKeychain::ReadPasswordJob(kKeychainService, this);
    job->setKey(mWalletKey);
    connect(job, &QKeychain::Job::finished, this, [this, job] {
        job->deleteLater();
        const QString refreshToken = job->error() ? QString() : job->textData();
        if (refreshToken.isEmpty()) {
            startInteractive();
        } else {
            startSilentRefresh(refreshToken);
        }
    });
    job->start();
}

void GraphOAuth::startSilentRefresh(const QString &refreshToken)
{
    mInteractive = false;
    mFlow->setRefreshToken(refreshToken);
    mFlow->refreshTokens();
}

void GraphOAuth::startInteractive()
{
    mInteractive = true;

    if (!mReplyHandler) {
        mReplyHandler = new QOAuthHttpServerReplyHandler(this);
        mReplyHandler->setCallbackHost(QStringLiteral("localhost"));
        mReplyHandler->setCallbackPath(kCallbackPath);
    }
    if (!mReplyHandler->isListening() && !mReplyHandler->listen(QHostAddress::LocalHost, kCallbackPort)) {
        // Preferred port taken (e.g. another app of the same registration). Azure ignores
        // the port on http://localhost redirect URIs for desktop apps — any free one works.
        if (!mReplyHandler->listen(QHostAddress::LocalHost, 0)) {
            Q_EMIT failed(i18n("OAuth2: cannot listen on localhost (no free port?)"));
            return;
        }
    }
    mFlow->setReplyHandler(mReplyHandler);
    mFlow->grant();
}

void GraphOAuth::onGranted()
{
    if (mReplyHandler) {
        mReplyHandler->close();
    }
    persistRefreshToken();
    scheduleProactiveRefresh();
    if (!mWasReady) {
        mWasReady = true;
        Q_EMIT ready();
    }
}

void GraphOAuth::onRequestFailed()
{
    // A dead refresh token (revoked, >90 days idle) fails the silent path — retry
    // interactively once before giving up.
    if (!mInteractive) {
        startInteractive();
        return;
    }
    Q_EMIT failed(i18n("OAuth2 request failed"));
}

void GraphOAuth::persistRefreshToken()
{
    const QString refreshToken = mFlow->refreshToken();
    if (refreshToken.isEmpty()) {
        return;
    }
    auto job = new QKeychain::WritePasswordJob(kKeychainService, this);
    job->setKey(mWalletKey);
    job->setTextData(refreshToken);
    connect(job, &QKeychain::Job::finished, job, &QObject::deleteLater);
    job->start();
}

void GraphOAuth::forgetTokens()
{
    mWasReady = false;
    if (mFlow) {
        mFlow->setToken(QString());
        mFlow->setRefreshToken(QString());
    }
    auto job = new QKeychain::DeletePasswordJob(kKeychainService, this);
    job->setKey(mWalletKey);
    connect(job, &QKeychain::Job::finished, job, &QObject::deleteLater);
    job->start();
}

void GraphOAuth::scheduleProactiveRefresh()
{
    if (!mRefreshTimer) {
        mRefreshTimer = new QTimer(this);
        mRefreshTimer->setSingleShot(true);
        connect(mRefreshTimer, &QTimer::timeout, this, [this] {
            if (!mFlow->refreshToken().isEmpty()) {
                mFlow->refreshTokens(); // granted -> reschedule
            }
        });
    }
    // Renew 5 minutes before expiry (Graph tokens last 60-90 min).
    constexpr qint64 defaultDelayMsecs = 30 * 60 * 1000;
    constexpr qint64 minimumDelayMsecs = 60 * 1000;
    constexpr qint64 expiryMarginMsecs = 5 * 60 * 1000;
    qint64 msecs = defaultDelayMsecs;
    if (mFlow->expirationAt().isValid()) {
        msecs = qMax(minimumDelayMsecs, QDateTime::currentDateTime().msecsTo(mFlow->expirationAt()) - expiryMarginMsecs);
    }
    mRefreshTimer->start(int(qMin<qint64>(msecs, std::numeric_limits<int>::max())));
}

QString GraphOAuth::accessToken() const
{
    if (!mEnvToken.isEmpty()) {
        return mEnvToken;
    }
    return mFlow ? mFlow->token() : QString();
}

#include "moc_graphoauth.cpp"
