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
    : EwsSettingsBase(), mWindowId(windowId), mWalletTimer(this)
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
        return;
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
