/*
    Copyright (c) 2014 Sandro Knauß <knauss@kolabsys.com>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "setupispdb.h"
#include "identity.h"
#include "resource.h"
#include "transport.h"
#include "ldap.h"
#include "configfile.h"

#include <KLocalizedString>

SetupIspdb::SetupIspdb(QObject *parent)
    : SetupObject(parent)
{
    mIspdb = new Ispdb(this);
    connect(mIspdb, SIGNAL(finished(bool)), SLOT(onIspdbFinished(bool)));
}

SetupIspdb::~SetupIspdb()
{
    delete mIspdb;
}

QStringList SetupIspdb::relevantDomains() const
{
    return mIspdb->relevantDomains();
}

QString SetupIspdb::name(int l) const
{
    return mIspdb->name((Ispdb::length)l);
}

int SetupIspdb::defaultIdentity() const
{
    return mIspdb->defaultIdentity();
}

int SetupIspdb::countIdentities() const
{
    return mIspdb->identities().count();
}

void SetupIspdb::fillIdentitiy(int i, QObject *o) const
{
    identity isp = mIspdb->identities()[i];

    Identity *id = qobject_cast<Identity *>(o);

    id->setIdentityName(isp.name);
    id->setRealName(isp.name);
    id->setEmail(isp.email);
    id->setOrganization(isp.organization);
    id->setSignature(isp.signature);
}

void SetupIspdb::fillImapServer(int i, QObject *o) const
{
    server isp = mIspdb->imapServers()[i];
    Resource *imapRes = qobject_cast<Resource *>(o);

    imapRes->setName(isp.hostname);
    imapRes->setOption(QLatin1String("ImapServer"), isp.hostname);
    imapRes->setOption(QLatin1String("UserName"), isp.username);
    imapRes->setOption(QLatin1String("ImapPort"), isp.port);
    imapRes->setOption(QLatin1String("Authentication"), isp.authentication);  //TODO: setup with right authentification
    if (isp.socketType == Ispdb::None) {
        imapRes->setOption(QLatin1String("Safety"), QLatin1String("NONE"));
    } else if (isp.socketType == Ispdb::SSL) {
        imapRes->setOption(QLatin1String("Safety"), QLatin1String("SSL"));
    } else {
        imapRes->setOption(QLatin1String("Safety"), QLatin1String("STARTTLS"));
    }
}

int SetupIspdb::countImapServers() const
{
    return mIspdb->imapServers().count();
}

void SetupIspdb::fillSmtpServer(int i, QObject *o) const
{
    server isp = mIspdb->smtpServers()[i];
    Transport *smtpRes = qobject_cast<Transport *>(o);

    smtpRes->setName(isp.hostname);
    smtpRes->setHost(isp.hostname);
    smtpRes->setPort(isp.port);
    smtpRes->setUsername(isp.username);

    switch (isp.authentication) {
    case Ispdb::Plain:
        smtpRes->setAuthenticationType(QLatin1String("plain"));
        break;
    case Ispdb::CramMD5:
        smtpRes->setAuthenticationType(QLatin1String("cram-md5"));
        break;
    case Ispdb::NTLM:
        smtpRes->setAuthenticationType(QLatin1String("ntlm"));
    break;
    case Ispdb::GSSAPI:
        smtpRes->setAuthenticationType(QLatin1String("gssapi"));
        break;
    case Ispdb::ClientIP:
    case Ispdb::NoAuth:
    default:
        break;
    }
    switch (isp.socketType) {
    case Ispdb::None:
          smtpRes->setEncryption(QLatin1String("none"));
          break;
    case Ispdb::SSL:
          smtpRes->setEncryption(QLatin1String("ssl"));
          break;
    case Ispdb::StartTLS:
          smtpRes->setEncryption(QLatin1String("tls"));
          break;
    default:
          break;
    }
}

int SetupIspdb::countSmtpServers() const
{
    return mIspdb->smtpServers().count();
}

void SetupIspdb::start()
{
    mIspdb->start();
    emit info(i18n("Searching for autoconfiguration..."));
}

void SetupIspdb::setEmail(const QString &email)
{
    mIspdb->setEmail(email);
}

void SetupIspdb::setPassword(const QString &password)
{
    mIspdb->setPassword(password);
}

void SetupIspdb::create()
{
}

void SetupIspdb::destroy()
{
}

void SetupIspdb::onIspdbFinished(bool status)
{
    emit ispdbFinished(status);
    if (status) {
        emit info(i18n("Autoconfiguration found."));
    } else {
        emit info(i18n("Autoconfiguration failed."));
    }
}
