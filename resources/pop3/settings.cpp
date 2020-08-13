/* 
   SPDX-FileCopyrightText: 2010 Thomas McGuire <mcguire@kde.org>

   SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "settings.h"
#include "settingsadaptor.h"

#include <KWallet>
#include "pop3resource_debug.h"

Settings::Settings(const KSharedConfigPtr &config, Options options)
    : SettingsBase(config)
{
    if (options & Option::ExportToDBus) {
        new SettingsAdaptor(this);
        QDBusConnection::sessionBus().registerObject(QStringLiteral("/Settings"), this,
                                                     QDBusConnection::ExportAdaptors | QDBusConnection::ExportScriptableContents);
    }
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
