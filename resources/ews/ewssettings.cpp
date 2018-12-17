/*
    Copyright (C) 2017-2018 Krzysztof Nowicki <krissn@op.pl>

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

#include "ewssettings.h"

#include <KPasswordDialog>
#include <KWallet/KWallet>
#include <KLocalizedString>

#include "auth/ewspasswordauth.h"
#include "auth/ewsoauth.h"

#include "ewsresource_debug.h"

static const QString ewsWalletFolder = QStringLiteral("akonadi-ews");

// Allow unittest to override this to shorten test execution
#ifdef EWSSETTINGS_UNITTEST
constexpr int walletTimeout = 2000;
#else
constexpr int walletTimeout = 30000;
#endif

using namespace KWallet;

EwsSettings::EwsSettings(WId windowId)
    : EwsSettingsBase(), mWindowId(windowId), mPasswordReadPending(false), mPasswordWritePending(false),
      mMapReadPending(false),  mMapWritePending(false), mWalletTimer(this)
{
    mWalletTimer.setInterval(walletTimeout);
    mWalletTimer.setSingleShot(true);
    connect(&mWalletTimer, &QTimer::timeout, this, [this]() {
        qCWarning(EWSRES_LOG) << "Timeout waiting for wallet open";
        onWalletOpened(false);
    });
}

EwsSettings::~EwsSettings()
{
}

bool EwsSettings::requestWalletOpen()
{
    if (!mWallet) {
        mWallet = Wallet::openWallet(Wallet::NetworkWallet(),
                                     mWindowId, Wallet::Asynchronous);
        if (mWallet) {
            connect(mWallet.data(), &Wallet::walletOpened, this,
                    &EwsSettings::onWalletOpened);
            mWalletTimer.start();
            return true;
        } else {
            qCWarning(EWSRES_LOG) << "Failed to open wallet";
        }
    }
    return false;
}

void EwsSettings::requestPassword(bool ask)
{
    bool status = false;

    qCDebug(EWSRES_LOG) << "requestPassword: start";
    if (!mPassword.isNull()) {
        qCDebug(EWSRES_LOG) << "requestPassword: password already set";
        Q_EMIT passwordRequestFinished(mPassword);
        return;
    }

    if (requestWalletOpen()) {
        mPasswordReadPending = true;
        return;
    }

    if (mWallet && mWallet->isOpen()) {
        mPassword = readPassword();
        mWallet.clear();
        status = true;
    }

    if (!status) {
        if (!ask) {
            qCDebug(EWSRES_LOG) << "requestPassword: Not allowed to ask";
        } else {
            qCDebug(EWSRES_LOG) << "requestPassword: Requesting interactively";
            mPasswordDlg = new KPasswordDialog(nullptr);
            mPasswordDlg->setModal(true);
            mPasswordDlg->setPrompt(i18n("Please enter password for user '%1' and Exchange account '%2'.",
                                username(), email()));
            if (mPasswordDlg->exec() == QDialog::Accepted) {
                mPassword = mPasswordDlg->password();
                setPassword(mPassword);
            }
            delete mPasswordDlg;
        }
    }

    Q_EMIT passwordRequestFinished(mPassword);
}

void EwsSettings::requestMap()
{
    qCDebug(EWSRES_LOG) << "requestMap: start";
    if (!mMap.isEmpty()) {
        qCDebug(EWSRES_LOG) << "requestMap: already set";
        Q_EMIT mapRequestFinished(mMap);
        return;
    }

    if (requestWalletOpen()) {
        mMapReadPending = true;
        return;
    }

    if (mWallet && mWallet->isOpen()) {
        mMap = readMap();
        mWallet.clear();
    }

    Q_EMIT mapRequestFinished(mMap);
}

QString EwsSettings::readPassword() const
{
    QString password;
    if (mWallet->hasFolder(ewsWalletFolder)) {
        mWallet->setFolder(ewsWalletFolder);
        mWallet->readPassword(config()->name(), password);
    } else {
        mWallet->createFolder(ewsWalletFolder);
    }
    return password;
}

QMap<QString, QString> EwsSettings::readMap() const
{
    QMap<QString, QString> map;
    if (mWallet->hasFolder(ewsWalletFolder)) {
        mWallet->setFolder(ewsWalletFolder);
        mWallet->readMap(config()->name(), map);
    } else {
        mWallet->createFolder(ewsWalletFolder);
    }
    return map;
}

void EwsSettings::satisfyPasswordReadRequest(bool success)
{
    if (success) {
        if (mPassword.isNull()) {
            mPassword = readPassword();
        }
        qCDebug(EWSRES_LOG) << "satisfyPasswordReadRequest: got password";
        Q_EMIT passwordRequestFinished(mPassword);
    } else {
        qCDebug(EWSRES_LOG) << "satisfyPasswordReadRequest: failed to retrieve password";
        Q_EMIT passwordRequestFinished(QString());
    }
    mPasswordReadPending = false;
}

void EwsSettings::satisfyPasswordWriteRequest(bool success)
{
    if (success) {
        if (!mWallet->hasFolder(ewsWalletFolder)) {
            mWallet->createFolder(ewsWalletFolder);
        }
        mWallet->setFolder(ewsWalletFolder);
        mWallet->writePassword(config()->name(), mPassword);
    }
    mPasswordWritePending = false;
}

void EwsSettings::satisfyMapReadRequest(bool success)
{
    if (success) {
        if (mMap.isEmpty()) {
            mMap = readMap();
        }
        qCDebug(EWSRES_LOG) << "satisfyMapReadRequest: got map";
        Q_EMIT mapRequestFinished(mMap);
    } else {
        qCDebug(EWSRES_LOG) << "satisfyTokensReadRequest: failed to retrieve map";
        Q_EMIT mapRequestFinished(QMap<QString, QString>());
    }
    mMapReadPending = false;
}

void EwsSettings::satisfyMapWriteRequest(bool success)
{
    if (success) {
        if (!mWallet->hasFolder(ewsWalletFolder)) {
            mWallet->createFolder(ewsWalletFolder);
        }
        mWallet->setFolder(ewsWalletFolder);
        mWallet->writeMap(config()->name(), mMap);
    }
    mMapWritePending = false;
}

void EwsSettings::onWalletOpened(bool success)
{
    mWalletTimer.stop();
    if (mWallet) {
        if (mPasswordReadPending) {
            satisfyPasswordReadRequest(success);
        }
        if (mPasswordWritePending) {
            satisfyPasswordWriteRequest(success);
        }
        if (mMapReadPending) {
            satisfyMapReadRequest(success);
        }
        if (mMapWritePending) {
            satisfyMapWriteRequest(success);
        }
        mWallet.clear();
    }
}

void EwsSettings::setPassword(const QString &password)
{
    if (password.isNull()) {
        qCWarning(EWSRES_LOG) << "Trying to set a null password";
        return;
    }

    mPassword = password;

    /* If a pending password request is running, satisfy it. */
    if (mWallet && mPasswordReadPending) {
        satisfyPasswordReadRequest(true);
    }
    if (mPasswordDlg) {
        mPasswordDlg->reject();
    }

    if (requestWalletOpen()) {
        mPasswordWritePending = true;
    }
}

void EwsSettings::setMap(const QMap<QString, QString> &map)
{
    if (map.isEmpty()) {
        qCWarning(EWSRES_LOG) << "Trying to set null map";
        return;
    }

    for (auto it = map.begin(); it != map.end(); ++it) {
        qDebug() << "setMap:" << it.key();
        if (!it.value().isNull()) {
            mMap[it.key()] = it.value();
        } else {
            mMap.remove(it.key());
        }
    }

    /* Remote items with null value - this is a way to remove
       unwanted items from the map. */
    for (auto it = mMap.begin(); it != mMap.end(); ++it) {
        while (it->isNull()) {
            it = mMap.erase(it);
        }
    }

    /* If a pending password request is running, satisfy it. */
    if (mWallet && mMapReadPending) {
        satisfyMapReadRequest(true);
    }

    if (requestWalletOpen()) {
        mMapWritePending = true;
    }
}

void EwsSettings::setTestPassword(const QString &password)
{
    if (password.isNull()) {
        qCWarning(EWSRES_LOG) << "Trying to set a null password";
        return;
    }

    mPassword = password;
    if (mWallet) {
        satisfyPasswordReadRequest(true);
    }
    if (mPasswordDlg) {
        mPasswordDlg->reject();
    }
}

/* Not needed for unittests - exclude to avoid excess link-time dependencies. */
#ifndef EWSSETTINGS_UNITTEST
EwsAbstractAuth *EwsSettings::loadAuth(QObject *parent)
{
    qCDebugNC(EWSRES_LOG) << QStringLiteral("Setting up authentication");

    EwsAbstractAuth *auth = nullptr;
    const auto mode = authMode();
    if (mode == QStringLiteral("oauth2")) {
        qCDebugNC(EWSRES_LOG) << QStringLiteral("Using OAuth2 authentication");

        auth = new EwsOAuth(parent, email(), oAuth2AppId(), oAuth2ReturnUri());
    }
    if (mode == QStringLiteral("username-password")) {
        qCDebugNC(EWSRES_LOG) << QStringLiteral("Using password-based authentication");

        QString user;
        if (!hasDomain()) {
            user = username();
        } else {
            user = domain() + QLatin1Char('\\') + username();
        }
        auth = new EwsPasswordAuth(user, parent);
    }

#ifdef HAVE_QCA
    if (!pKeyCert().isNull() && !pKeyKey().isNull()) {
        auth->setPKeyAuthCertificateFiles(pKeyCert(), pKeyKey());
    }
#endif

    connect(auth, &EwsAbstractAuth::requestWalletPassword, this, &EwsSettings::requestPassword);
    connect(auth, &EwsAbstractAuth::requestWalletMap, this, &EwsSettings::requestMap);
    connect(this, &EwsSettings::passwordRequestFinished, auth, &EwsAbstractAuth::walletPasswordRequestFinished);
    connect(this, &EwsSettings::mapRequestFinished, auth, &EwsAbstractAuth::walletMapRequestFinished);
    connect(auth, &EwsAbstractAuth::setWalletPassword, this, &EwsSettings::setPassword);
    connect(auth, &EwsAbstractAuth::setWalletMap, this, &EwsSettings::setMap);

    return auth;
}
#endif /* EWSSETTINGS_UNITTEST */
