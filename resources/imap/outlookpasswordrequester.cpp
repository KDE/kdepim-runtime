/*
    SPDX-FileCopyrightText: 2023 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "outlookpasswordrequester.h"
#include "imapresource_debug.h"
#include "imapresourcebase.h"
#include "passwordrequesterinterface.h"
#include "settings.h"

#include <MailTransport/OutlookOAuthTokenRequester>

#include <KLocalizedString>
#include <KWallet>

#include <memory>

static const QString clientId = QStringLiteral("18da2bc3-146a-4581-8c92-27dc7b9954a0");
static const QString tenantId = QStringLiteral("common");
static const QStringList scopes = {QStringLiteral("https://outlook.office.com/IMAP.AccessAsUser.All"), QStringLiteral("offline_access")};

static const QString kwalletFolder = QStringLiteral("imap");

OutlookPasswordRequester::OutlookPasswordRequester(ImapResourceBase *resource, QObject *parent)
    : XOAuthPasswordRequester(parent)
    , mResource(resource)
{
}

OutlookPasswordRequester::~OutlookPasswordRequester() = default;

QString OutlookPasswordRequester::loadTokenFromKWallet(KWallet::Wallet *wallet, const QString &tokenType)
{
    if (!wallet->hasFolder(kwalletFolder)) {
        return {};
    }

    wallet->setFolder(kwalletFolder);
    QMap<QString, QString> result;
    wallet->readMap(mResource->settings()->config()->name(), result);
    auto token = result.constFind(tokenType);
    if (token == result.cend()) {
        return {};
    }

    return *token;
}

void OutlookPasswordRequester::requestPassword(RequestType request, const QString &serverError)
{
    Q_UNUSED(serverError) // we don't get anything useful from XOAUTH2 SASL

    if (mRequestInProgress) {
        qCDebug(IMAPRESOURCE_LOG) << "Outlook OAuth2 token request already in progress";
        return;
    }

    auto *wallet = KWallet::Wallet::openWallet(KWallet::Wallet::NetworkWallet(), 0, KWallet::Wallet::Asynchronous);
    connect(wallet, &KWallet::Wallet::walletOpened, this, [this, wallet, request](bool success) {
        if (!success) {
            mRequestInProgress = false;
            Q_EMIT done(UserRejected, i18nc("@status", "Failed to open KWallet"));
            return;
        }

        mTokenRequester = std::make_unique<MailTransport::OutlookOAuthTokenRequester>(clientId, tenantId, scopes);
        connect(mTokenRequester.get(), &MailTransport::OutlookOAuthTokenRequester::finished, this, [this, wallet](const auto &result) {
            onTokenRequestFinished(wallet, result);
        });

        if (request == WrongPasswordRequest) {
            const auto refreshToken = loadTokenFromKWallet(wallet, QStringLiteral("refreshToken"));
            if (!refreshToken.isEmpty()) {
                qCDebug(IMAPRESOURCE_LOG) << "Found an Outlook OAuth2 refresh token in KWallet, refreshing access token...";
                mTokenRequester->refreshToken(refreshToken);
            } else {
                qCDebug(IMAPRESOURCE_LOG) << "No Outlook OAuth2 refresh token found in KWallet, requesting new token...";
                mTokenRequester->requestToken(mResource->settings()->userName());
            }
        } else {
            const auto accessToken = loadTokenFromKWallet(wallet, QStringLiteral("accessToken"));
            if (accessToken.isEmpty()) {
                qCDebug(IMAPRESOURCE_LOG) << "No Outlook OAuth2 access token found in KWallet, requesting new token...";
                mTokenRequester->requestToken(mResource->settings()->userName());
            } else {
                qCDebug(IMAPRESOURCE_LOG) << "Found an Outlook OAuth2 access token in KWallet, using it...";
                mRequestInProgress = false;
                Q_EMIT done(PasswordRetrieved, accessToken);
            }
        }
    });
}

void OutlookPasswordRequester::cancelPasswordRequests()
{
    if (mTokenRequester) {
        mTokenRequester->disconnect(this);
        mTokenRequester.release()->deleteLater();
        qCDebug(IMAPRESOURCE_LOG) << "Canceled Outlook OAuth2 token request";
    }
}

void OutlookPasswordRequester::storeResultToWallet(KWallet::Wallet *wallet, const MailTransport::TokenResult &result)
{
    const auto name = mResource->settings()->config()->name();
    qCDebug(IMAPRESOURCE_LOG).nospace().noquote() << "Storing Outlook OAuth2 token to KWallet (" << kwalletFolder << "/" << name << ")";
    if (!wallet->hasFolder(kwalletFolder)) {
        wallet->createFolder(kwalletFolder);
    }
    wallet->setFolder(kwalletFolder);
    const int ok = wallet->writeMap(name, {{QStringLiteral("accessToken"), result.accessToken()}, {QStringLiteral("refreshToken"), result.refreshToken()}});
    if (ok != 0) {
        qCWarning(IMAPRESOURCE_LOG) << "Failed to store Outlook OAuth2 token to KWallet:" << ok;
    }
}

void OutlookPasswordRequester::onTokenRequestFinished(KWallet::Wallet *wallet, const MailTransport::TokenResult &result)
{
    mRequestInProgress = false;
    mTokenRequester.release()->deleteLater();
    if (result.hasError()) {
        qCDebug(IMAPRESOURCE_LOG) << "Outlook OAuth2 token request failed:" << result.errorText();
        Q_EMIT done(UserRejected, result.errorText());
    } else {
        storeResultToWallet(wallet, result);
        Q_EMIT done(PasswordRetrieved, result.accessToken());
    }
}
