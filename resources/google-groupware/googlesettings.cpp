/*
    Copyright (C) 2011-2013  Dan Vratil <dan@progdan.cz>
                  2020  Igor Poboiko <igor.poboiko@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "googlesettings.h"
#include "settingsbase.h"
#include "googleresource_debug.h"

#include <QGlobalStatic>
#include <KGAPI/Account>
#include <KWallet>

using namespace KWallet;
using namespace KGAPI2;

static const QString googleWalletFolder = QStringLiteral("Akonadi Google");

GoogleSettings::GoogleSettings()
    : m_winId(0)
{
    m_wallet = Wallet::openWallet(Wallet::NetworkWallet(),
                                  m_winId, Wallet::Asynchronous);
    if (m_wallet) {
        connect(m_wallet.data(), &Wallet::walletOpened, this, &GoogleSettings::slotWalletOpened);
    } else {
        qCWarning(GOOGLE_LOG) << "Failed to open wallet!";
    }
}

void GoogleSettings::slotWalletOpened(bool success)
{
    if (!success) {
        qCWarning(GOOGLE_LOG) << "Failed to open wallet!";
        Q_EMIT accountReady(false);
        return;
    }

    if (!m_wallet->hasFolder(googleWalletFolder)
        && !m_wallet->createFolder(googleWalletFolder)) {
        qCWarning(GOOGLE_LOG) << "Failed to create wallet folder" << googleWalletFolder;
        Q_EMIT accountReady(false);
        return;
    }

    if (!m_wallet->setFolder(googleWalletFolder)) {
        qWarning() << "Failed to open wallet folder" << googleWalletFolder;
        Q_EMIT accountReady(false);
        return;
    }
    qCDebug(GOOGLE_LOG) << "Wallet opened, reading" << account();
    if (!account().isEmpty()) {
        m_account = fetchAccountFromWallet(account());
    }
    m_isReady = true;
    Q_EMIT accountReady(true);
}

KGAPI2::AccountPtr GoogleSettings::fetchAccountFromWallet(const QString &accountName)
{
    if (!m_wallet->entryList().contains(accountName)) {
        qCDebug(GOOGLE_LOG) << "Account" << accountName << "not found in KWallet";
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

bool GoogleSettings::storeAccount(AccountPtr account)
{
    // Removing the old one (if present)
    if (m_account && (account->accountName() != m_account->accountName())) {
        cleanup();
    }
    // Populating the new one
    m_account = account;

    QStringList scopes;
    const QList<QUrl> urlScopes = m_account->scopes();
    scopes.reserve(urlScopes.count());
    for (const QUrl &url : urlScopes) {
        scopes << url.toString();
    }

    QMap<QString, QString> map;
    map[QStringLiteral("accessToken")] = m_account->accessToken();
    map[QStringLiteral("refreshToken")] = m_account->refreshToken();
    map[QStringLiteral("scopes")] = scopes.join(QLatin1Char(','));
    // Removing previous junk (if present)
    cleanup();
    if (m_wallet->writeMap(m_account->accountName(), map) != 0) {
        qCWarning(GOOGLE_LOG) << "Failed to write new account entry to wallet";
        return false;
    }
    SettingsBase::setAccount(m_account->accountName());
    m_isReady = true;
    return true;
}

void GoogleSettings::cleanup()
{
    if (m_account && m_wallet) {
        m_wallet->removeEntry(m_account->accountName());
    }
}

void GoogleSettings::addCalendar(const QString &calendar)
{
    if (calendars().isEmpty() || calendars().contains(calendar)) {
        return;
    }
    setCalendars(calendars() << calendar);
    save();
}

void GoogleSettings::addTaskList(const QString &taskList)
{
    if (calendars().isEmpty() || taskLists().contains(taskList)) {
        return;
    }
    setTaskLists(taskLists() << taskList);
    save();
}

QString GoogleSettings::clientId() const
{
    return QStringLiteral("554041944266.apps.googleusercontent.com");
}

QString GoogleSettings::clientSecret() const
{
    return QStringLiteral("mdT1DjzohxN3npUUzkENT0gO");
}

bool GoogleSettings::isReady() const
{
    return m_isReady;
}

AccountPtr GoogleSettings::accountPtr()
{
    return m_account;
}

void GoogleSettings::setWindowId(WId id)
{
    m_winId = id;
}

void GoogleSettings::setResourceId(const QString &resourceIdentificator)
{
    m_resourceId = resourceIdentificator;
}
