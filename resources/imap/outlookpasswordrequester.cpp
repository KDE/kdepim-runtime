/*
    SPDX-FileCopyrightText: 2023 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "outlookpasswordrequester.h"
#include "imapresource_debug.h"
#include "imapresourcebase.h"
#include "settings.h"

#include <MailTransport/OutlookOAuthTokenRequester>

#include <KLocalizedString>
#include <qt6keychain/keychain.h>

#include <memory>

using namespace QKeychain;

static constexpr QLatin1StringView clientId{"18da2bc3-146a-4581-8c92-27dc7b9954a0"};
static constexpr QLatin1StringView tenantId{"common"};
static const QStringList scopes{
    QLatin1StringView("https://outlook.office.com/IMAP.AccessAsUser.All"),
    QLatin1StringView("offline_access"),
};

static constexpr QLatin1StringView walletFolder = QLatin1StringView("imap");

OutlookPasswordRequester::OutlookPasswordRequester(ImapResourceBase *resource, QObject *parent)
    : XOAuthPasswordRequester(parent)
    , mResource(resource)
{
}

OutlookPasswordRequester::~OutlookPasswordRequester() = default;

void OutlookPasswordRequester::requestPassword(RequestType request, const QString &serverError)
{
    Q_UNUSED(serverError) // we don't get anything useful from XOAUTH2 SASL

    if (mRequestInProgress) {
        qCDebug(IMAPRESOURCE_LOG) << "Outlook OAuth2 token request already in progress";
        return;
    }

    mTokenRequester = std::make_unique<MailTransport::OutlookOAuthTokenRequester>(clientId, tenantId, scopes);
    connect(mTokenRequester.get(), &MailTransport::OutlookOAuthTokenRequester::finished, this, [this](const auto &result) {
        onTokenRequestFinished(result);
    });

    auto readJob = new ReadPasswordJob(walletFolder);
    readJob->setKey(mResource->settings()->config()->name());
    connect(readJob, &ReadPasswordJob::finished, this, [this, readJob, request]() {
        if (!mTokenRequester) {
            qCDebug(IMAPRESOURCE_LOG) << "The pasword request was cancelled.";
            return;
        }

        if (readJob->error() == QKeychain::Error::EntryNotFound) {
            qCDebug(IMAPRESOURCE_LOG) << "No Outlook OAuth2 tokens found in KWallet, requesting new token...";
            mTokenRequester->requestToken(mResource->settings()->userName());
            return;
        } else if (readJob->error() != QKeychain::Error::NoError) {
            mRequestInProgress = false;
            qCWarning(IMAPRESOURCE_LOG) << "Failed to read password from keychain.";
            Q_EMIT done(UserRejected, i18nc("@status", "Failed to read password from keychain."));
            return;
        }

        QMap<QString, QString> map;
        auto value = readJob->binaryData();
        if (value.isEmpty()) {
            mRequestInProgress = false;
            qCWarning(IMAPRESOURCE_LOG) << "Failed to read password from keychain.";
            Q_EMIT done(UserRejected, i18nc("@status", "Failed to read password from keychain."));
            return;
        }

        QDataStream ds(value);
        ds >> map;

        if (request == WrongPasswordRequest) {
            const auto refreshToken = map[QStringLiteral("refreshToken")];

            if (!refreshToken.isEmpty()) {
                qCDebug(IMAPRESOURCE_LOG) << "Found an Outlook OAuth2 refresh token in KWallet, refreshing access token...";
                mTokenRequester->refreshToken(refreshToken);
            } else {
                qCDebug(IMAPRESOURCE_LOG) << "No Outlook OAuth2 refresh token found in KWallet, requesting new token...";
                mTokenRequester->requestToken(mResource->settings()->userName());
            }
        } else {
            const auto accessToken = map[QStringLiteral("accessToken")];
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
    readJob->start();
}

void OutlookPasswordRequester::cancelPasswordRequests()
{
    if (mTokenRequester) {
        mTokenRequester->disconnect(this);
        mTokenRequester.release()->deleteLater();
        qCDebug(IMAPRESOURCE_LOG) << "Canceled Outlook OAuth2 token request";
    }
}

void OutlookPasswordRequester::storeResultToWallet(const MailTransport::TokenResult &result)
{
    const auto name = mResource->settings()->config()->name();
    QByteArray mapData;
    QDataStream ds(&mapData, QIODeviceBase::WriteOnly);
    ds << QMap<QString, QString>{{QStringLiteral("accessToken"), result.accessToken()}, {QStringLiteral("refreshToken"), result.refreshToken()}};

    auto writeJob = new WritePasswordJob(walletFolder);
    writeJob->setKey(mResource->settings()->config()->name());
    writeJob->setBinaryData(mapData);
    connect(writeJob, &WritePasswordJob::finished, this, [this, writeJob, name]() {
        if (writeJob->error() != QKeychain::Error::NoError) {
            qCWarning(IMAPRESOURCE_LOG) << "Failed to store Outlook OAuth2 token to KWallet.";
            return;
        }
        qCDebug(IMAPRESOURCE_LOG).nospace().noquote() << "Storing Outlook OAuth2 token to KWallet (" << walletFolder << "/" << name << ")";
    });
    writeJob->start();
}

void OutlookPasswordRequester::onTokenRequestFinished(const MailTransport::TokenResult &result)
{
    mRequestInProgress = false;
    mTokenRequester.release()->deleteLater();
    if (result.hasError()) {
        qCDebug(IMAPRESOURCE_LOG) << "Outlook OAuth2 token request failed:" << result.errorText();
        Q_EMIT done(UserRejected, result.errorText());
    } else {
        storeResultToWallet(result);
        Q_EMIT done(PasswordRetrieved, result.accessToken());
    }
}

#include "moc_outlookpasswordrequester.cpp"
