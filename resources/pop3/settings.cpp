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

Pop3Settings::AuthenticationMethod Settings::mapTransportAuthToKpop(MailTransport::Transport::EnumAuthenticationType authType)
{
    switch (authType) {
    case MailTransport::Transport::APOP:
        return Pop3Settings::APOP;
    case MailTransport::Transport::ANONYMOUS:
        return Pop3Settings::Anonymous;
    case MailTransport::Transport::PLAIN:
        return Pop3Settings::Plain;
    case MailTransport::Transport::NTLM:
        return Pop3Settings::NTLM;
    case MailTransport::Transport::LOGIN:
        return Pop3Settings::Login;
    case MailTransport::Transport::GSSAPI:
        return Pop3Settings::GSSAPI;
    case MailTransport::Transport::DIGEST_MD5:
        return Pop3Settings::DigestMD5;
    case MailTransport::Transport::CRAM_MD5:
        return Pop3Settings::CramMD5;
    case MailTransport::Transport::CLEAR:
        return Pop3Settings::Clear;
    case MailTransport::Transport::XOAUTH2:
        return Pop3Settings::XOAuth2;
    default:
        qWarning() << "mapping from Transport::EnumAuthenticationType -> KPOP::Pop3Settings::AuthenticationMethod not possible" << authType;
        return Pop3Settings::Clear; // dummy value, shouldn't get here.
    }
}

Pop3Settings Settings::toPop3Settings() const
{
    Pop3Settings result;
    result.setLogin(this->login());
    result.setHost(this->host());
    result.setPort(this->port());
    result.setAuthenticationMethod(mapTransportAuthToKpop(static_cast<MailTransport::Transport::EnumAuthenticationType>(this->authenticationMethod())));
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
