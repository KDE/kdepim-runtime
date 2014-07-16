/*
 * Copyright (C) 2014  Sandro Knauß <knauss@kolabsys.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef AUTOCONFIGKOLABLDAP_H
#define AUTOCONFIGKOLABLDAP_H

#include "autoconfigkolabmail.h"
#include <kldap/ldapserver.h>

struct ldapServer;

class AutoconfigKolabLdap : public AutoconfigKolabMail
{
public:
    /** Constructor */
    explicit AutoconfigKolabLdap(QObject *parent = 0);

    QHash<QString, ldapServer> ldapServers() const;

protected:
    virtual void lookupInDb(bool auth, bool crypt);
    virtual void parseResult(const QDomDocument &document);

private:
    ldapServer createLdapServer(const QDomElement &n);

    QHash<QString, ldapServer> mLdapServers;

};

struct ldapServer {
    ldapServer()
        : port(-1)
        , socketType(KLDAP::LdapServer::None)
        , authentication(KLDAP::LdapServer::Anonymous)
        , ldapVersion(3)
        , pageSize(-1)
        , timeLimit(-1)
        , sizeLimit(-1)
    {
    }
    bool isValid() const {
        return (port != -1);
    }
    QString hostname;
    int port;
    KLDAP::LdapServer::Security socketType;
    KLDAP::LdapServer::Auth authentication;
    int ldapVersion;
    int pageSize;
    int timeLimit;
    int sizeLimit;
    QString bindDn;
    QString password;
    QString saslMech;
    QString username;
    QString realm;
    QString dn;
    QString filter;
};

#endif // AUTOCONFIGKOLABLDAP_H
