/*
   SPDX-FileCopyrightText: 2013 Daniel Vrátil <dvratil@redhat.com>
   SPDX-FileCopyrightText: 2020 Igor Poboiko <igor.poboiko@gmail.com>

   SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include "settingsbase.h"

#include <QPointer>
#include <qwindowdefs.h>

#include <KGAPI/Types>

namespace KWallet
{
class Wallet;
}

/**
 * @brief Settings object
 *
 * Provides read-only access to application clientId and
 * clientSecret and read-write access to accessToken and
 * refreshToken. Interacts with KWallet.
 */
class GoogleSettings : public SettingsBase
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.Akonadi.Google.ExtendedSettings")

public:
    GoogleSettings();
    void init();
    void setWindowId(WId id);
    void setResourceId(const QString &resourceIdentifier);

    QString appId() const;
    QString clientId() const;
    QString clientSecret() const;

    void addCalendar(const QString &calendar);
    void addTaskList(const QString &taskList);

    KGAPI2::AccountPtr accountPtr();
    // Wallet
    bool isReady() const;
    bool storeAccount(KGAPI2::AccountPtr account);
    void cleanup();
Q_SIGNALS:
    void accountReady(bool ready);
    void accountChanged();
private Q_SLOTS:
    void slotWalletOpened(bool success);

private:
    WId m_winId = 0;
    QString m_resourceId;
    bool m_isReady = false;
    KGAPI2::AccountPtr m_account;
    QPointer<KWallet::Wallet> m_wallet;

    KGAPI2::AccountPtr fetchAccountFromWallet(const QString &accountName);
};
