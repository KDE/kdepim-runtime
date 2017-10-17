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

#ifndef EWSSETTINGS_H
#define EWSSETTINGS_H

#ifdef EWSSETTINGS_UNITTEST
#include "ewssettings_ut_mock.h"
#else
#include "ewssettingsbase.h"
#endif

#include <QTimer>
#include <QPointer>

namespace KWallet {
class Wallet;
}
class KPasswordDialog;

class EwsSettings : public EwsSettingsBase
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.Akonadi.Ews.Wallet")
public:
    explicit EwsSettings(WId windowId);
    ~EwsSettings() override;

    void requestPassword(bool ask);
public Q_SLOTS:
    Q_SCRIPTABLE void setPassword(const QString &password);
    Q_SCRIPTABLE void setTestPassword(const QString &password);
Q_SIGNALS:
    void passwordRequestFinished(const QString &password);
private Q_SLOTS:
    void onWalletOpenedForRead(bool success);
    void onWalletOpenedForWrite(bool success);
private:
    QString readPassword() const;
    WId mWindowId;
    QString mPassword;
    QPointer<KWallet::Wallet> mWallet;
    QTimer mWalletReadTimer;
    QTimer mWalletWriteTimer;
    QPointer<KPasswordDialog> mPasswordDlg;
};

#endif

