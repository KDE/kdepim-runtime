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

#include "setupautoconfigkolabldap.h"
#include "ldap.h"
#include "configfile.h"

#include <KLocalizedString>

SetupAutoconfigKolabLdap::SetupAutoconfigKolabLdap(QObject *parent)
    : SetupObject(parent)
{
    mIspdb = new AutoconfigKolabLdap(this);
    connect(mIspdb, SIGNAL(finished(bool)), SLOT(onIspdbFinished(bool)));
}

SetupAutoconfigKolabLdap::~SetupAutoconfigKolabLdap()
{
    delete mIspdb;
}

void SetupAutoconfigKolabLdap::fillLdapServer(int i, QObject *o) const
{
    ldapServer isp = mIspdb->ldapServers().values()[i];
    Ldap *ldapRes = qobject_cast<Ldap *>(o);

    //TODO: setting dn & filter

    ldapRes->setServer(isp.hostname);
    ldapRes->setPort(isp.port);
    ldapRes->setSecurity(isp.socketType);
    ldapRes->setVersion(isp.ldapVersion);
    ldapRes->setUser(isp.username);
    ldapRes->setPassword(isp.password);
    ldapRes->setBindDn(isp.bindDn);

    ldapRes->setRealm(isp.realm);
    ldapRes->setSaslMech(isp.saslMech);

    if (isp.pageSize != -1) {
        ldapRes->setPageSize(isp.pageSize);
    }

    if (isp.timeLimit != -1) {
        ldapRes->setPageSize(isp.timeLimit);
    }

    if (isp.sizeLimit != -1) {
        ldapRes->setPageSize(isp.sizeLimit);
    }

    //Anonymous is set by not setting the AuthenticationMethod
    if (isp.authentication == KLDAP::LdapServer::SASL) {
        ldapRes->setAuthenticationMethod(QLatin1String("SASL"));
    } else if (isp.authentication == KLDAP::LdapServer::Simple) {
        ldapRes->setAuthenticationMethod(QLatin1String("Simple"));
    }
}

int SetupAutoconfigKolabLdap::countLdapServers() const
{
    return mIspdb->ldapServers().count();
}

void SetupAutoconfigKolabLdap::start()
{
    mIspdb->start();
    emit info(i18n("Searching for autoconfiguration..."));
}

void SetupAutoconfigKolabLdap::setEmail(const QString &email)
{
    mIspdb->setEmail(email);
}

void SetupAutoconfigKolabLdap::setPassword(const QString &password)
{
    mIspdb->setPassword(password);
}

void SetupAutoconfigKolabLdap::create()
{
}

void SetupAutoconfigKolabLdap::destroy()
{
}

void SetupAutoconfigKolabLdap::onIspdbFinished(bool status)
{
    emit ispdbFinished(status);
    if (status) {
        emit info(i18n("Autoconfiguration found."));
    } else {
        emit info(i18n("Autoconfiguration failed."));
    }
}
