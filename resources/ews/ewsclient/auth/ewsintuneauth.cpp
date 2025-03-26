/*
    SPDX-FileCopyrightText: 2025 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

/*
    This module implements OAuth2 authentication using Microsoft InTune for Linux.
    The authentication is performed by calling the Microsoft Identity Broker over
    it's DBus API. The broker offers methods to acquire an access token in the same
    way as if OAuth2 was performed directly, but with additional use of device
    credentials in order to satisfy MFA requirements enforced by organizations.
    Internally the broker holds a Primary Refresh Token (PRT), which is obtained
    using the device credentials. This token serves as proof of authentication of
    a given user on an organization-managed device. The broker uses the PRT as a
    special refresh token to obtain an access token to the requested service.
    Two types of token request methods are supported - interactive and
    non-interactive. The first one guarantees no user interaction, but may fail if
    one is needed. The second one uses a browser window hosted by the broker itself
    to possibly request additional credentials from the user (such as username,
    password or a code from the Microsoft Authenticator smartphone app).
*/

#include "broker1.h"
#include "ewsclient_debug.h"
#include <KLocalizedString>
#include <QByteArrayView>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QJsonDocument>
#include <QJsonObject>
#include <QScopedPointer>
#include <QUuid>
#include <optional>

#include "ewsintuneauth.h"

using std::make_optional;
using std::nullopt;
using std::optional;

using namespace Qt::StringLiterals;

namespace
{
constexpr auto brokerIfaceVersion = "1.0"_L1;
constexpr auto accessTokenMapKey = "access-token"_L1;
}

class EwsInTuneAuthPrivate final : public QObject
{
public:
    EwsInTuneAuthPrivate(EwsInTuneAuth *parent, const QString &email, const QString &appId, const QString &redirectUri);
    optional<const QJsonObject> findAccount(const QJsonArray &accounts);
    bool authenticate(bool interactive);
    void error(const QString &error, const QString &errorDescription, const QUrl &uri);
    void resetPendingAuth();
    void acquireAuthToken();
public Q_SLOTS:
    void getAccountsFinished(QDBusPendingCallWatcher *call);
    void getAuthTokenFinished(QDBusPendingCallWatcher *call);

public:
    const QString mEmail;
    const QString mAppId;
    const QString mRedirectUri;
    bool mInteractiveAuth;
    QString mAuthToken;
    QJsonObject mInTuneAccount;
    QScopedPointer<ComMicrosoftIdentityBroker1Interface> mBrokerInterface;
    QScopedPointer<QDBusPendingCallWatcher> mPendingCallWatcher;

    EwsInTuneAuth *q_ptr = nullptr;
    Q_DECLARE_PUBLIC(EwsInTuneAuth)
};

EwsInTuneAuthPrivate::EwsInTuneAuthPrivate(EwsInTuneAuth *parent, const QString &email, const QString &appId, const QString &redirectUri)
    : mEmail(email)
    , mAppId(appId)
    , mRedirectUri(redirectUri)
    , mInteractiveAuth(false)
    , q_ptr(parent)
{
}

void EwsInTuneAuthPrivate::error(const QString &error, const QString &errorDescription, const QUrl &uri)
{
    Q_Q(EwsInTuneAuth);

    Q_UNUSED(uri)

    mAuthToken.clear();

    qCInfoNC(EWSCLI_LOG) << "Authentication failed: "_L1 << error << ' ' << errorDescription;

    resetPendingAuth();

    Q_EMIT q->authFailed(error);
}

void EwsInTuneAuthPrivate::resetPendingAuth()
{
    mPendingCallWatcher.reset();
    mBrokerInterface.reset();
}

bool EwsInTuneAuthPrivate::authenticate(bool interactive)
{
    if (mBrokerInterface) {
        qCWarningNC(EWSCLI_LOG) << "InTune authentication already in progress"_L1;
        return false;
    }

    mInteractiveAuth = interactive;
    mBrokerInterface.reset(new ComMicrosoftIdentityBroker1Interface("com.microsoft.identity.broker1"_L1,
                                                                    "/com/microsoft/identity/broker1"_L1,
                                                                    QDBusConnection::sessionBus(),
                                                                    this));
    if (!mBrokerInterface->isValid()) {
        qCWarningNC(EWSCLI_LOG) << "Failed to connect to InTune broker DBus interface"_L1;
        return false;
    }

    if (mInTuneAccount.isEmpty()) {
        const auto request(QJsonDocument(QJsonObject({{"clientId"_L1, mAppId}, {"redirectUri"_L1, mRedirectUri}})));
        auto resp = mBrokerInterface->getAccounts(brokerIfaceVersion, QUuid::createUuid().toString(QUuid::WithoutBraces), QString::fromUtf8(request.toJson()));
        mPendingCallWatcher.reset(new QDBusPendingCallWatcher(resp, this));

        connect(mPendingCallWatcher.get(), &QDBusPendingCallWatcher::finished, this, &EwsInTuneAuthPrivate::getAccountsFinished);
    } else {
        acquireAuthToken();
    }

    return true;
}

optional<const QJsonObject> EwsInTuneAuthPrivate::findAccount(const QJsonArray &accounts)
{
    for (const auto &account : accounts) {
        if (account.isObject()) {
            const auto accountObj = account.toObject();
            const auto username = accountObj["username"_L1];
            const auto realm = accountObj["realm"_L1];
            if (realm.isString() && username.isString() && username.toString() == mEmail) {
                return make_optional(accountObj);
            }
        }
    }

    return nullopt;
}

void EwsInTuneAuthPrivate::getAccountsFinished(QDBusPendingCallWatcher *call)
{
    QDBusPendingReply<QString> reply = *call;
    if (reply.isError()) {
        error("InTune call failed"_L1, "Failed to retrieve InTune accounts: "_L1 + "DBus call failed"_L1, QUrl());
    } else {
        QJsonParseError jsonParseError;
        const auto replyJson = QJsonDocument::fromJson(reply.argumentAt<0>().toUtf8(), &jsonParseError);
        if (!replyJson.isObject()) {
            error("InTune call failed"_L1, "Failed to retrieve InTune accounts: "_L1 + "Incorrect JSON response"_L1, QUrl());
            return;
        }

        qCDebugNC(EWSCLI_LOG) << "Got InTune accounts response: "_L1 << replyJson;

        const auto replyObj = replyJson.object();
        const auto errorObj = replyObj["error"_L1];
        if (errorObj.isObject()) {
            error("InTune call failed"_L1,
                  "Failed to retrieve InTune accounts: "_L1 + "Broker error: "_L1 + errorObj.toObject()["context"_L1].toString(),
                  QUrl());
            return;
        }
        const auto accounts = replyObj["accounts"_L1];
        if (!accounts.isArray()) {
            error("InTune call failed"_L1, "Failed to retrieve InTune accounts: "_L1 + "No accounts found"_L1, QUrl());
        } else {
            const auto account = findAccount(accounts.toArray());
            if (!account) {
                error("InTune call failed"_L1, "Cannot find suitable InTune account"_L1, QUrl());
            } else {
                mInTuneAccount = *account;
                acquireAuthToken();
            }
        }
    }
}

void EwsInTuneAuthPrivate::acquireAuthToken()
{
    const auto request =
        QJsonDocument(QJsonObject({{"authParameters"_L1,
                                    QJsonObject({{"accessTokenToRenew"_L1, QString()},
                                                 {"account"_L1, mInTuneAccount},
                                                 {"authority"_L1, QString("https://login.microsoftonline.com/"_L1 + mInTuneAccount["realm"_L1].toString())},
                                                 {"clientId"_L1, mAppId},
                                                 {"decodedClaims"_L1, QString()},
                                                 {"enrollmentId"_L1, QString()},
                                                 {"password"_L1, QString()},
                                                 {"popParams"_L1, QJsonValue::Null},
                                                 {"redirectUri"_L1, mRedirectUri},
                                                 {"requestedScopes"_L1, QJsonArray({"https://outlook.office365.com/.default"_L1})},
                                                 {"username"_L1, mEmail}})}}));

    const auto authFunc =
        mInteractiveAuth ? &ComMicrosoftIdentityBroker1Interface::acquireTokenInteractively : &ComMicrosoftIdentityBroker1Interface::acquireTokenSilently;
    auto resp =
        (mBrokerInterface.get()->*authFunc)(brokerIfaceVersion, QUuid::createUuid().toString(QUuid::WithoutBraces), QString::fromUtf8(request.toJson()));
    mPendingCallWatcher.reset(new QDBusPendingCallWatcher(resp, this));

    connect(mPendingCallWatcher.get(), &QDBusPendingCallWatcher::finished, this, &EwsInTuneAuthPrivate::getAuthTokenFinished);
}

void EwsInTuneAuthPrivate::getAuthTokenFinished(QDBusPendingCallWatcher *call)
{
    Q_Q(EwsInTuneAuth);

    QDBusPendingReply<QString> reply = *call;
    if (reply.isError()) {
        error("InTune call failed"_L1, "Failed to retrieve InTune access token: "_L1 + "DBus call failed"_L1, QUrl());
    } else {
        QJsonParseError jsonParseError;
        const auto replyJson = QJsonDocument::fromJson(reply.argumentAt<0>().toUtf8(), &jsonParseError);
        if (!replyJson.isObject()) {
            error("InTune call failed"_L1, "Failed to retrieve InTune access token"_L1 + "Incorrect JSON response"_L1, QUrl());
            return;
        }

        qCDebugNC(EWSCLI_LOG) << "Got InTune access token response: "_L1 << replyJson;

        const auto replyObj = replyJson.object();

        const auto brokerResponse = replyObj["brokerTokenResponse"_L1];
        if (brokerResponse.isObject()) {
            const auto responseObj = brokerResponse.toObject();
            const auto errorObj = responseObj["error"_L1];
            if (errorObj.isObject()) {
                error("InTune call failed"_L1,
                      "Failed to retrieve InTune access token: "_L1 + "Broker error: "_L1 + errorObj.toObject()["context"_L1].toString(),
                      QUrl());
                return;
            }
            const auto accessToken = responseObj["accessToken"_L1];
            if (accessToken.isString()) {
                mAuthToken = accessToken.toString();

                const QMap<QString, QString> map{{accessTokenMapKey, mAuthToken}};
                Q_EMIT q->setWalletMap(map);

                Q_EMIT q->authSucceeded();
                return;
            }
        }

        error("InTune call failed"_L1, "Failed to retrieve InTune access token: "_L1 + "Unknown error"_L1, QUrl());
    }
}

EwsInTuneAuth::~EwsInTuneAuth() = default;

void EwsInTuneAuth::init()
{
    Q_EMIT requestWalletMap();
}

bool EwsInTuneAuth::getAuthData(QString &username, QString &password, QHash<QByteArray, QByteArray> &customHeaders)
{
    Q_UNUSED(username);
    Q_UNUSED(password);

    Q_D(EwsInTuneAuth);

    if (!d->mAuthToken.isNull()) {
        customHeaders["Authorization"_ba] = "Bearer "_ba + d->mAuthToken.toUtf8();
        return true;
    } else {
        return false;
    }
}

void EwsInTuneAuth::notifyRequestAuthFailed()
{
    Q_D(EwsInTuneAuth);

    d->mAuthToken.clear();

    EwsAbstractAuth::notifyRequestAuthFailed();
}

bool EwsInTuneAuth::authenticate(bool interactive)
{
    Q_D(EwsInTuneAuth);

    return d->authenticate(interactive);
}

const QString &EwsInTuneAuth::reauthPrompt() const
{
    static const QString prompt =
        xi18nc("@info",
               "InTune credentials for the Microsoft Exchange account <b>%1</b> are no longer valid. You need to authenticate in order to continue using it.",
               QStringLiteral("%1"));
    return prompt;
}

const QString &EwsInTuneAuth::authFailedPrompt() const
{
    static const QString prompt =
        xi18nc("@info",
               "Failed to obtain InTune credentials for Microsoft Exchange account <b>%1</b>. Please update it in the account settings page.",
               QStringLiteral("%1"));
    return prompt;
}

void EwsInTuneAuth::walletPasswordRequestFinished(const QString &password)
{
    Q_UNUSED(password);
}

void EwsInTuneAuth::walletMapRequestFinished(const QMap<QString, QString> &map)
{
    Q_D(EwsInTuneAuth);
    qCWarning(EWSCLI_LOG) << "walletMapRequestFinished: "_L1 << map;

    if (map.contains(accessTokenMapKey)) {
        d->mAuthToken = map[accessTokenMapKey];
        Q_EMIT authSucceeded();
    } else {
        Q_EMIT authFailed("Access token request failed"_L1);
    }
}

EwsInTuneAuth::EwsInTuneAuth(QObject *parent, const QString &email, const QString &appId, const QString &redirectUri)
    : EwsAbstractAuth(parent)
    , d_ptr(new EwsInTuneAuthPrivate(this, email, appId, redirectUri))
{
}