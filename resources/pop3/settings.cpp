/*
   SPDX-FileCopyrightText: 2010 Thomas McGuire <mcguire@kde.org>

   SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "settings.h"
#include "settingsadaptor.h"
#include <qt6keychain/keychain.h>
using namespace QKeychain;
#include "pop3resource_debug.h"

Settings::Settings(const KSharedConfigPtr &config, Options options)
    : SettingsBase(config)
{
    if (options & Option::ExportToDBus) {
        new SettingsAdaptor(this);
        QDBusConnection::sessionBus().registerObject(QStringLiteral("/Settings"),
                                                     this,
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

Pop3Settings Settings::toPop3Settings() const
{
    Pop3Settings result;
    result.setLogin(this->login());
    result.setHost(this->host());
    result.setPort(this->port());
    result.setAuthenticationMethod(this->authenticationMethod());
    result.setUseSSL(this->useSSL());
    result.setUseTLS(this->useTLS());
    result.setPipelining(this->pipelining());
    result.setUseProxy(this->useProxy());
    return result;
}

void Settings::setPassword(const QString &password)
{
    auto writeJob = new WritePasswordJob(QStringLiteral("pop3"));
    connect(writeJob, &QKeychain::Job::finished, this, [](QKeychain::Job *baseJob) {
        if (baseJob->error()) {
            qCWarning(POP3RESOURCE_LOG) << "Error writing password using QKeychain:" << baseJob->errorString();
        }
    });
    writeJob->setKey(mResourceId);
    writeJob->setTextData(password);
    writeJob->start();
}

#include "moc_settings.cpp"
