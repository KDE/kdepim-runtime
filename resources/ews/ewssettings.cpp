/*
    SPDX-FileCopyrightText: 2017-2018 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "ewssettings.h"

#include <KLocalizedString>
#include <KWallet>

#include "auth/ewsoauth.h"
#include "auth/ewspasswordauth.h"

#include "ewsresource_debug.h"
#include <KAuthorized>
#include <qt6keychain/keychain.h>

using namespace QKeychain;
using namespace Qt::StringLiterals;

static const QString ewsWalletFolder = QStringLiteral("akonadi-ews");

EwsSettings::EwsSettings(QObject *parent)
    : EwsSettingsBase(parent)
{
}

EwsSettings::~EwsSettings() = default;

void EwsSettings::requestPassword()
{
    if (!mPassword.isEmpty()) {
        Q_EMIT passwordRequestFinished(mPassword);
    }

    auto readJob = new ReadPasswordJob(ewsWalletFolder);
    readJob->setKey(config()->name());
    connect(readJob, &ReadPasswordJob::finished, this, [this, readJob]() {
        if (readJob->error() != QKeychain::Error::NoError) {
            qCDebug(EWSRES_LOG) << "requestPassword: Failed to read password";
            return;
        }
        Q_EMIT passwordRequestFinished(readJob->textData());
    });

    readJob->start();
}

void EwsSettings::requestMap()
{
    qCDebug(EWSRES_LOG) << "requestMap: start";

    auto readJob = new ReadPasswordJob(ewsWalletFolder);
    readJob->setKey(config()->name() + "_map"_L1);

    connect(readJob, &ReadPasswordJob::finished, this, [this, readJob]() {
        QMap<QString, QString> map;
        if (readJob->error() != QKeychain::Error::NoError) {
            qCDebug(EWSRES_LOG) << "requestPassword: Failed to read map" << readJob->error();

            // TODO remove me in 2025, this is a fallback to read password stored
            // in KWallet and store it back in QtKeychain.
            auto wallet = KWallet::Wallet::openWallet(KWallet::Wallet::NetworkWallet(), 0, KWallet::Wallet::Synchronous);
            if (wallet && wallet->hasFolder(ewsWalletFolder)) {
                wallet->setFolder(ewsWalletFolder);
                QMap<QString, QString> map;
                wallet->readMap(config()->name(), map);
                wallet->removeEntry(config()->name());
                Q_EMIT mapRequestFinished(map);

                setMap(map);
                return;
            }
        }

        const auto value = readJob->binaryData();
        if (value.isEmpty()) {
            Q_EMIT mapRequestFinished(map);
            return;
        }
        QDataStream ds(value);
        ds >> map;
        Q_EMIT mapRequestFinished(map);
    });
    readJob->start();
}

void EwsSettings::setPassword(const QString &password)
{
    if (password.isNull()) {
        qCWarning(EWSRES_LOG) << "Trying to set a null password";
        return;
    }

    mPassword = password;

    auto writeJob = new WritePasswordJob(ewsWalletFolder);
    writeJob->setKey(config()->name());
    writeJob->setTextData(mPassword);

    connect(writeJob, &WritePasswordJob::finished, this, [writeJob]() {
        if (writeJob->error() != QKeychain::Error::NoError) {
            qCDebug(EWSRES_LOG) << "requestPassword: Failed to write password";
            return;
        }
    });
    writeJob->start();
}

void EwsSettings::setMap(const QMap<QString, QString> &map)
{
    if (map.isEmpty()) {
        qCWarning(EWSRES_LOG) << "Trying to set null map";
        return;
    }

    QByteArray mapData;
    QDataStream ds(&mapData, QIODevice::WriteOnly);
    ds << map;

    auto writeJob = new WritePasswordJob(ewsWalletFolder);
    writeJob->setKey(config()->name() + "_map"_L1);
    writeJob->setBinaryData(mapData);

    connect(writeJob, &WritePasswordJob::finished, this, [writeJob]() {
        if (writeJob->error() != QKeychain::Error::NoError) {
            qCDebug(EWSRES_LOG) << "requestPassword: Failed to write map";
            return;
        }
    });
    writeJob->start();
}

void EwsSettings::setTestPassword(const QString &password)
{
    if (password.isNull()) {
        qCWarning(EWSRES_LOG) << "Trying to set a null password";
        return;
    }

    mPassword = password;
    Q_EMIT passwordRequestFinished(mPassword);
}

/* Not needed for unittests - exclude to avoid excess link-time dependencies. */
#ifndef EWSSETTINGS_UNITTEST
EwsAbstractAuth *EwsSettings::loadAuth(QObject *parent)
{
    qCDebugNC(EWSRES_LOG) << QStringLiteral("Setting up authentication");

    EwsAbstractAuth *auth = nullptr;
    const auto mode = authMode();
    if (mode == QLatin1StringView("oauth2")) {
        qCDebugNC(EWSRES_LOG) << QStringLiteral("Using OAuth2 authentication");

        auth = new EwsOAuth(parent, email(), oAuth2AppId(), oAuth2ReturnUri());
    }
    if (mode == QLatin1StringView("username-password")) {
        qCDebugNC(EWSRES_LOG) << QStringLiteral("Using password-based authentication");

        QString user;
        if (!hasDomain()) {
            user = username();
        } else {
            user = domain() + QLatin1Char('\\') + username();
        }
        auth = new EwsPasswordAuth(user, parent);
    }

    if (!pKeyCert().isNull() && !pKeyKey().isNull()) {
        auth->setPKeyAuthCertificateFiles(pKeyCert(), pKeyKey());
    }

    connect(auth, &EwsAbstractAuth::requestWalletPassword, this, &EwsSettings::requestPassword);
    connect(auth, &EwsAbstractAuth::requestWalletMap, this, &EwsSettings::requestMap);
    connect(this, &EwsSettings::passwordRequestFinished, auth, &EwsAbstractAuth::walletPasswordRequestFinished);
    connect(this, &EwsSettings::mapRequestFinished, auth, &EwsAbstractAuth::walletMapRequestFinished);
    connect(auth, &EwsAbstractAuth::setWalletPassword, this, &EwsSettings::setPassword);
    connect(auth, &EwsAbstractAuth::setWalletMap, this, &EwsSettings::setMap);

    return auth;
}

#endif /* EWSSETTINGS_UNITTEST */

#include "moc_ewssettings.cpp"
