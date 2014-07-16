/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2014  Daniel Vrátil <dvratil@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "gmailsettings.h"
//#include "settingsadaptor.h"

#include "imapaccount.h"

#include <kwallet.h>
using KWallet::Wallet;

#include <kglobal.h>
#include <klocale.h>
#include <kpassworddialog.h>

#include <QDBusConnection>

#include <KDE/Akonadi/Collection>
#include <KDE/Akonadi/CollectionFetchJob>
#include <KDE/Akonadi/CollectionModifyJob>

#include <LibKGAPI2/Account>
#include <LibKGAPI2/AuthJob>

GmailSettings::GmailSettings(WId winId)
    : Settings(winId)
    , mActiveAuthJob(0)
{
    // Try to initialize mAccount
    requestAccount(false);
    /*
    new SettingsAdaptor( this );
    QDBusConnection::sessionBus().registerObject( QLatin1String( "/GmailSettings" ), this,
                                                  QDBusConnection::ExportAdaptors | QDBusConnection::ExportScriptableContents );
    */
}

QString GmailSettings::apiKey() const
{
    return QLatin1String("554041944266.apps.googleusercontent.com");
}

QString GmailSettings::secretKey() const
{
    return QLatin1String("mdT1DjzohxN3npUUzkENT0gO");
}


void GmailSettings::clearCachedPassword()
{
    mAccount = KGAPI2::AccountPtr();
}

void GmailSettings::cleanup()
{
    Wallet* wallet = Wallet::openWallet(Wallet::NetworkWallet(), m_winId);
    if (wallet && wallet->isOpen()) {
        if (wallet->hasFolder(QLatin1String("gmail"))) {
            wallet->setFolder(QLatin1String("gmail"));
            wallet->removeEntry(config()->name());
        }
        delete wallet;
    }
}

void GmailSettings::requestPassword()
{
    requestAccount(false);
    if (mAccount) {
        Q_EMIT passwordRequestCompleted(mAccount->accessToken(), false);
    } else {
        Q_EMIT passwordRequestCompleted(QString(), false);
    }
}

void GmailSettings::requestAccount(bool authenticate)
{
    bool userRejected = false;
    loadAccountFromKWallet(&userRejected);
    if (userRejected) {
        Q_EMIT accountRequestCompleted(KGAPI2::AccountPtr(), true);
        return;
    }

    if (authenticate) {
        if (mActiveAuthJob) {
            return;
        }

        if (!mAccount) {
            mAccount = KGAPI2::AccountPtr(new KGAPI2::Account());
            mAccount->addScope(QUrl(QLatin1String("https://mail.google.com")));
        }

        KGAPI2::AuthJob *authJob = new KGAPI2::AuthJob(mAccount, apiKey(), secretKey(), this);
        connect(authJob, SIGNAL(finished(KGAPI2::Job*)),
                this, SLOT(onAuthFinished(KGAPI2::Job*)));
        mActiveAuthJob = authJob;
    } else {
        if (mAccount) {
            setImapServer(QLatin1String("imap.gmail.com"));
        } else {
            setImapServer(QString());
        }
        Q_EMIT accountRequestCompleted(mAccount, false);
    }
}

void GmailSettings::onAuthFinished(KGAPI2::Job *job)
{
    mActiveAuthJob = 0;

    if (job->error()) {
        Q_EMIT accountRequestCompleted(KGAPI2::AccountPtr(), job->error() == KGAPI2::AuthCancelled);
        return;
    }

    KGAPI2::AuthJob *auth = qobject_cast<KGAPI2::AuthJob*>(job);
    const KGAPI2::AccountPtr account = auth->account();
    storeAccount(account);
    setImapServer(QLatin1String("imap.gmail.com"));

    Q_EMIT accountRequestCompleted(account, false);
}


void GmailSettings::onWalletOpened(bool success)
{
    if (!success) {
        emit passwordRequestCompleted(QString(), true);
    } else {
        Wallet *wallet = qobject_cast<Wallet*>( sender() );
        bool passwordNotStoredInWallet = true;
        if (wallet && wallet->hasFolder(QLatin1String("gmail"))) {
            loadAccountFromKWallet();
            passwordNotStoredInWallet = false;
        }
        if (passwordNotStoredInWallet || !mAccount) {
            /* FIXME: Manual auth */
            //requestManualAuth();
        } else {
            emit passwordRequestCompleted(mAccount->accessToken(), passwordNotStoredInWallet);
        }

        if (wallet) {
            wallet->deleteLater();
        }
    }
}

void GmailSettings::loadAccountFromKWallet(bool *userRejected) const
{
    if (userRejected != 0) {
        *userRejected = false;
    }

    Wallet* wallet = Wallet::openWallet(Wallet::NetworkWallet(), m_winId);
    if (wallet && wallet->isOpen()) {
        if (wallet->hasFolder(QLatin1String("gmail"))) {
            wallet->setFolder(QLatin1String("gmail"));
            QMap<QString, QString> map;
            wallet->readMap(config()->name(), map);
            mAccount = KGAPI2::AccountPtr(new KGAPI2::Account(map[QLatin1String("accountName")],
                                                              map[QLatin1String("accessToken")],
                                                              map[QLatin1String("refreshToken")],
                                                              QList<QUrl>() << QUrl(QLatin1String("https://mail.google.com"))
                                                                            << KGAPI2::Account::accountInfoScopeUrl()
                                                                            << KGAPI2::Account::accountInfoEmailScopeUrl()));
        } else {
            wallet->createFolder(QLatin1String("gmail"));
            mAccount = KGAPI2::AccountPtr();
        }
    } else {
        mAccount = KGAPI2::AccountPtr();
        if (userRejected != 0) {
            *userRejected = true;
        }
    }
}

void GmailSettings::saveAccountToKWallet()
{
    Wallet* wallet = Wallet::openWallet(Wallet::NetworkWallet(), m_winId);
    if (wallet && wallet->isOpen()) {
        if (!wallet->hasFolder(QLatin1String("gmail"))) {
            wallet->createFolder(QLatin1String("gmail"));
        }
        wallet->setFolder(QLatin1String("gmail"));
        QMap<QString, QString> map;
        map[QLatin1String("accountName")] = mAccount->accountName();
        map[QLatin1String("accessToken")] = mAccount->accessToken();
        map[QLatin1String("refreshToken")] = mAccount->refreshToken();
        wallet->writeMap(config()->name(), map);
        kDebug() << "Wallet save: " << wallet->sync();
    }
    delete wallet;
}


QString GmailSettings::accountName(bool *userRejected) const
{
    if (!mAccount) {
        loadAccountFromKWallet(userRejected);
    }

    return mAccount->accountName();
}

void GmailSettings::setAccountName(const QString &accountName)
{
    if (accountName == mAccount->accountName()) {
        return;
    }

    mAccount->setAccountName(accountName);
    saveAccountToKWallet();
}

QString GmailSettings::password(bool *userRejected) const
{
    if (!mAccount) {
        loadAccountFromKWallet(userRejected);
    }
    if (mAccount)
        return mAccount->accessToken();
    return QString();
}

void GmailSettings::setPassword(const QString &accessToken)
{
    if (accessToken == mAccount->accessToken()) {
        return;
    }

    mAccount->setAccessToken(accessToken);
    saveAccountToKWallet();
}

QString GmailSettings::refreshToken(bool *userRejected) const
{
    if (!mAccount) {
        loadAccountFromKWallet(userRejected);
    }

    return mAccount->refreshToken();
}

void GmailSettings::setRefreshToken(const QString &refreshToken)
{
    if (refreshToken == mAccount->refreshToken()) {
        return;
    }

    mAccount->setRefreshToken(refreshToken);
    saveAccountToKWallet();
}


void GmailSettings::loadAccount(ImapAccount *account) const
{
    kDebug() << userName();
    account->setServer(QLatin1String("imap.gmail.com"));
    account->setPort(993);

    account->setUserName(userName());
    account->setSubscriptionEnabled(subscriptionEnabled());

    account->setEncryptionMode(KIMAP::LoginJob::SslV3);
    account->setAuthenticationMode(KIMAP::LoginJob::XOAuth2);

    account->setTimeout( sessionTimeout() );
}

void GmailSettings::storeAccount(const KGAPI2::AccountPtr &account)
{
    if (!account) {
        cleanup();
        return;
    }

    mAccount = account;
    saveAccountToKWallet();
}

QString GmailSettings::rootRemoteId() const
{
    return QLatin1String("imap://") + userName() + QLatin1Char('@') + imapServer() + QLatin1Char('/');
}
