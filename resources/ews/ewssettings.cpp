/*
    Copyright (C) 2017 Krzysztof Nowicki <krissn@op.pl>

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
#include <QPointer>

#include <KPasswordDialog>
#include <KWallet/KWallet>
#include <KLocalizedString>

#include "ewsresource_debug.h"

static const QString ewsWalletFolder = QStringLiteral("akonadi-ews");

// Allow unittest to override this to shorten test execution
#ifndef WALLET_TIMEOUT
#define WALLET_TIMEOUT 30000
#endif

EwsSettings::EwsSettings(WId windowId)
    : EwsSettingsBase(), mWindowId(windowId), mWalletReadTimer(this), mWalletWriteTimer(this)
{
    mWalletReadTimer.setInterval(WALLET_TIMEOUT);
    mWalletReadTimer.setSingleShot(true);
    connect(&mWalletReadTimer, &QTimer::timeout, this, [this]() {
        qCWarning(EWSRES_LOG) << "Timeout waiting for wallet open for read";
        onWalletOpenedForRead(false);
    });
    mWalletWriteTimer.setInterval(WALLET_TIMEOUT);
    mWalletWriteTimer.setSingleShot(true);
    connect(&mWalletWriteTimer, &QTimer::timeout, this, [this]() {
        qCWarning(EWSRES_LOG) << "Timeout waiting for wallet open for write";
        onWalletOpenedForWrite(false);
    });
}

EwsSettings::~EwsSettings()
{
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

    if (!mWallet) {
        mWallet = KWallet::Wallet::openWallet(KWallet::Wallet::NetworkWallet(),
                                              mWindowId, KWallet::Wallet::Asynchronous);
        if (mWallet) {
            connect(mWallet.data(), &KWallet::Wallet::walletOpened, this,
                    &EwsSettings::onWalletOpenedForRead);
            mWalletReadTimer.start();
            return;
        } else {
            qCWarning(EWSRES_LOG) << "Failed to open wallet";
        }
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
            QPointer<KPasswordDialog> dlg = new KPasswordDialog(nullptr);
            dlg->setModal(true);
            dlg->setPrompt(i18n("Please enter password for user '%1' and Exchange account '%2'.",
                                username(), email()));
            if (dlg->exec() == QDialog::Accepted) {
                mPassword = dlg->password();
                setPassword(mPassword);
            }
            delete dlg;
        }
    }

    Q_EMIT passwordRequestFinished(mPassword);
    return;
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

void EwsSettings::onWalletOpenedForRead(bool success)
{
    qCDebug(EWSRES_LOG) << "onWalletOpenedForRead: start" << success;
    mWalletReadTimer.stop();
    if (mWallet) {
        if (success) {
            if (mPassword.isNull()) {
                mPassword = readPassword();
            }
            qCDebug(EWSRES_LOG) << "onWalletOpenedForRead: got password";
            Q_EMIT passwordRequestFinished(mPassword);
        } else {
            qCDebug(EWSRES_LOG) << "onWalletOpenedForRead: failed to retrieve password";
            Q_EMIT passwordRequestFinished(QString());
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
    if (mWallet) {
        onWalletOpenedForRead(true);
    }

    mWallet = KWallet::Wallet::openWallet(KWallet::Wallet::NetworkWallet(),
                                          mWindowId, KWallet::Wallet::Asynchronous);
    if (mWallet) {
        connect(mWallet.data(), &KWallet::Wallet::walletOpened, this, &EwsSettings::onWalletOpenedForWrite);
        mWalletWriteTimer.start();
    } else {
        qCWarning(EWSRES_LOG) << "Failed to open wallet";
    }
}

void EwsSettings::onWalletOpenedForWrite(bool success)
{
    mWalletWriteTimer.stop();
    if (success) {
        if (!mWallet->hasFolder(ewsWalletFolder)) {
            mWallet->createFolder(ewsWalletFolder);
        }
        mWallet->setFolder(ewsWalletFolder);
        mWallet->writePassword(config()->name(), mPassword);
    }
    mWallet.clear();
}

void EwsSettings::setTestPassword(const QString &password)
{
    if (password.isNull()) {
        qCWarning(EWSRES_LOG) << "Trying to set a null password";
        return;
    }

    mPassword = password;
    if (mWallet) {
        onWalletOpenedForRead(true);
    }
}

