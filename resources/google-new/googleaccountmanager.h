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

#ifndef GOOGLEACCOUNTMANAGER_H
#define GOOGLEACCOUNTMANAGER_H

#include <QObject>
#include <QPointer>
#include <QMap>

#include <KGAPI/Types>
#include <KGAPI/Account>

namespace KWallet {
class Wallet;
}

class GoogleAccountManager : public QObject
{
    Q_OBJECT

public:
    explicit GoogleAccountManager(QObject *parent = nullptr);
    ~GoogleAccountManager() override;

    bool isReady() const;

    bool storeAccount(const KGAPI2::AccountPtr &account);
    KGAPI2::AccountPtr findAccount(const QString &accountName) const;
    bool removeAccount(const QString &accountName);
    KGAPI2::AccountsList listAccounts() const;

    void cleanup(const QString &accountName);

Q_SIGNALS:
    void managerReady(bool ready);
    void accountAdded(const KGAPI2::AccountPtr &account);
    void accountChanged(const KGAPI2::AccountPtr &account);
    void accountRemoved(const QString &accountName);

private Q_SLOTS:
    void initManager();
    void slotWalletOpened(bool success);
    void slotWalletClosed();
    void slotFolderUpdated(const QString &folder);

private:
    KGAPI2::AccountPtr findAccountInWallet(const QString &accountName) const;
    bool m_isReady;
    QPointer<KWallet::Wallet> m_wallet;
    mutable QMap<QString, KGAPI2::AccountPtr> m_accounts;
};

#endif // GOOGLEACCOUNTMANAGER_H
