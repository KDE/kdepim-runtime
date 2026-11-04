/*
   SPDX-FileCopyrightText: 2010 Thomas McGuire <mcguire@kde.org>

   SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "settings.h"
#include "settingsadaptor.h"
#include <MailTransport/Transport>
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

KPOP::LoginJob::AuthenticationMethod Settings::mapTransportAuthToKpop(MailTransport::Transport::EnumAuthenticationType authType)
{
    switch (authType) {
    case MailTransport::Transport::APOP:
        return KPOP::LoginJob::APOP;
    case MailTransport::Transport::ANONYMOUS:
        return KPOP::LoginJob::Anonymous;
    case MailTransport::Transport::PLAIN:
        return KPOP::LoginJob::Plain;
    case MailTransport::Transport::NTLM:
        return KPOP::LoginJob::NTLM;
    case MailTransport::Transport::LOGIN:
        return KPOP::LoginJob::Login;
    case MailTransport::Transport::GSSAPI:
        return KPOP::LoginJob::GSSAPI;
    case MailTransport::Transport::DIGEST_MD5:
        return KPOP::LoginJob::DigestMD5;
    case MailTransport::Transport::CRAM_MD5:
        return KPOP::LoginJob::CramMD5;
    case MailTransport::Transport::CLEAR:
        return KPOP::LoginJob::Clear;
    case MailTransport::Transport::XOAUTH2:
        return KPOP::LoginJob::XOAuth2;
    default:
        qWarning() << "mapping from Transport::EnumAuthenticationType -> KPOP::LoginJob::AuthenticationMethod not possible" << authType;
        return KPOP::LoginJob::Clear; // dummy value, shouldn't get here.
    }
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
