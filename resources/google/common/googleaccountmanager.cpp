/*
 * Copyright (C) 2013  Daniel Vrátil <dvratil@redhat.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include "googleaccountmanager.h"

#include <KWallet>
#include <QDebug>

#define WALLET_FOLDER QStringLiteral("Akonadi Google")

using namespace KGAPI2;

GoogleAccountManager::GoogleAccountManager(QObject *parent)
    : QObject(parent)
    , m_isReady(false)
{
    QMetaObject::invokeMethod(this, &GoogleAccountManager::initManager, Qt::QueuedConnection);
}

GoogleAccountManager::~GoogleAccountManager()
{
    delete m_wallet;
}

bool GoogleAccountManager::isReady() const
{
    return m_isReady;
}

void GoogleAccountManager::initManager()
{
    delete m_wallet;

    // FIXME: Don't use synchronous wallet
    // With asynchronous wallet however we are unable to read any data from it
    // in when slotWalletOpened() is called on walletOpened() signal
    m_wallet = KWallet::Wallet::openWallet(KWallet::Wallet::NetworkWallet(),
                                           0, KWallet::Wallet::Synchronous);
    slotWalletOpened(m_wallet != nullptr);
    if (m_wallet) {
        connect(m_wallet.data(), &KWallet::Wallet::folderUpdated, this, &GoogleAccountManager::slotFolderUpdated);
        connect(m_wallet.data(), &KWallet::Wallet::walletClosed, this, &GoogleAccountManager::slotWalletClosed);
    }
}

void GoogleAccountManager::slotWalletOpened(bool success)
{
    if (!success) {
        qWarning() << "Failed to open wallet";
        Q_EMIT managerReady(false);
        return;
    }

    if (!m_wallet->hasFolder(WALLET_FOLDER)) {
        if (!m_wallet->createFolder(WALLET_FOLDER)) {
            qWarning() << "Failed to create KWallet folder " << WALLET_FOLDER;
            Q_EMIT managerReady(false);
            return;
        }
    }

    if (!m_wallet->setFolder(WALLET_FOLDER)) {
        qWarning() << "Failed to open KWallet folder" << WALLET_FOLDER;
        Q_EMIT managerReady(false);
        return;
    }

    // Populate the cache now
    const QStringList accountNames = m_wallet->entryList();
    for (const QString &accountName : accountNames) {
        m_accounts[accountName] = findAccountInWallet(accountName);
    }

    m_isReady = true;
    Q_EMIT managerReady(true);
}

void GoogleAccountManager::slotWalletClosed()
{
    m_isReady = false;
    delete m_wallet;
}

void GoogleAccountManager::slotFolderUpdated(const QString &folder)
{
    // We are interested only in the "Akonadi Google" folder
    if (folder != WALLET_FOLDER) {
        return;
    }

    QStringList walletEntries = m_wallet->entryList();

    Q_FOREACH (const AccountPtr &account, m_accounts) { //no using foreach
        AccountPtr changedAccount = findAccountInWallet(account->accountName());
        if (changedAccount.isNull()) {
            walletEntries.removeOne(account->accountName());
            m_accounts.remove(account->accountName());
            Q_EMIT accountRemoved(account->accountName());
            continue;
        }

        if ((account->accessToken() != changedAccount->accessToken())
            || (account->refreshToken() != changedAccount->refreshToken())
            || (account->scopes() != changedAccount->scopes())) {
            walletEntries.removeOne(account->accountName());
            m_accounts[account->accountName()] = changedAccount;
            Q_EMIT accountChanged(changedAccount);
        }
    }

    for (const QString &accountName : qAsConst(walletEntries)) {
        const AccountPtr newAccount = findAccountInWallet(accountName);

        m_accounts[newAccount->accountName()] = newAccount;
        Q_EMIT accountAdded(newAccount);
    }
}

AccountPtr GoogleAccountManager::findAccount(const QString &accountName) const
{
    if (!m_isReady) {
        qWarning() << "Manager is not ready!";
        return AccountPtr();
    }

    if (m_accounts.contains(accountName)) {
        return m_accounts[accountName];
    }

    AccountPtr account = findAccountInWallet(accountName);
    if (account.isNull()) {
        return AccountPtr();
    }

    m_accounts[accountName] = account;
    return account;
}

AccountPtr GoogleAccountManager::findAccountInWallet(const QString &accountName) const
{
    if (!m_wallet->entryList().contains(accountName)) {
        qDebug() << "Account" << accountName << "not found in KWallet";
        return AccountPtr();
    }

    QMap<QString, QString> map;
    m_wallet->readMap(accountName, map);

#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
    const QStringList scopes = map[QStringLiteral("scopes")].split(QLatin1Char(','), QString::SkipEmptyParts);
#else
    const QStringList scopes = map[QStringLiteral("scopes")].split(QLatin1Char(','), Qt::SkipEmptyParts);
#endif
    QList<QUrl> scopeUrls;
    scopeUrls.reserve(scopes.count());
    for (const QString &scope : scopes) {
        scopeUrls << QUrl(scope);
    }
    AccountPtr account(new Account(accountName,
                                   map[QStringLiteral("accessToken")],
                                   map[QStringLiteral("refreshToken")],
                                   scopeUrls));

    return account;
}

bool GoogleAccountManager::storeAccount(const AccountPtr &account)
{
    if (!m_isReady) {
        qWarning() << "Manager is not ready!";
        return false;
    }

    QStringList scopes;
    const QList<QUrl> urlScopes = account->scopes();
    scopes.reserve(urlScopes.count());
    for (const QUrl &url : urlScopes) {
        scopes << url.toString();
    }

    QMap<QString, QString> map;
    map[QStringLiteral("accessToken")] = account->accessToken();
    map[QStringLiteral("refreshToken")] = account->refreshToken();
    map[QStringLiteral("scopes")] = scopes.join(QLatin1Char(','));

    if (m_wallet->writeMap(account->accountName(), map) == 0) {
        m_accounts[account->accountName()] = account;
        return true;
    }

    return false;
}

bool GoogleAccountManager::removeAccount(const QString &accountName)
{
    if (!m_isReady) {
        qWarning() << "Manager is not ready";
        return false;
    }

    if (!m_accounts.contains(accountName)) {
        return true;
    }

    if (m_wallet->removeEntry(accountName) != 0) {
        qWarning() << "Failed to remove account from KWallet";
        return false;
    }

    m_accounts.remove(accountName);
    return true;
}

AccountsList GoogleAccountManager::listAccounts() const
{
    if (!m_isReady) {
        qWarning() << "Manager is not ready";
        return AccountsList();
    }

    return m_accounts.values();
}

void GoogleAccountManager::cleanup(const QString &accountName)
{
    removeAccount(accountName);
}
