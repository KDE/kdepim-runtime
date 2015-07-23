/* Copyright 2010 Thomas McGuire <mcguire@kde.org>

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU Library General Public License as published
   by the Free Software Foundation; either version 2 of the License or
   ( at your option ) version 3 or, at the discretion of KDE e.V.
   ( which shall act as a proxy as in section 14 of the GPLv3 ), any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/
#include "settings.h"
#include "settingsadaptor.h"

#include <kwallet.h>
#include "pop3resource_debug.h"

class SettingsHelper
{
public:
    SettingsHelper() : q(Q_NULLPTR) {}
    ~SettingsHelper()
    {
        qCWarning(POP3RESOURCE_LOG) << q;
        delete q;
        q = Q_NULLPTR;
    }
    Settings *q;
};

Q_GLOBAL_STATIC(SettingsHelper, s_globalSettings)

Settings *Settings::self()
{
    if (!s_globalSettings->q) {
        new Settings;
        s_globalSettings->q->load();
    }
    return s_globalSettings->q;
}

Settings::Settings()
{
    Q_ASSERT(!s_globalSettings->q);
    s_globalSettings->q = this;

    new SettingsAdaptor(this);
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/Settings"), this,
            QDBusConnection::ExportAdaptors | QDBusConnection::ExportScriptableContents);
}

void Settings::setWindowId(WId id)
{
    mWinId = id;
}

void Settings::setResourceId(const QString &resourceIdentifier)
{
    mResourceId = resourceIdentifier;
}

void Settings::setPassword(const QString &password)
{
    using namespace KWallet;
    Wallet *wallet = Wallet::openWallet(Wallet::NetworkWallet(), mWinId,
                                        Wallet::Synchronous);
    if (wallet && wallet->isOpen()) {
        if (!wallet->hasFolder(QStringLiteral("pop3"))) {
            wallet->createFolder(QStringLiteral("pop3"));
        }
        wallet->setFolder(QStringLiteral("pop3"));
        wallet->writePassword(mResourceId, password);
    } else {
        qCWarning(POP3RESOURCE_LOG) << "Unable to open wallet!";
    }
    delete wallet;
}
