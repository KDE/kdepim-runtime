/*
 * Copyright (C) 2014  Sandro Knauß <knauss@kolabsys.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version cense as published by
 * the Free Software Foundation, either version 2: of the License, or
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

#include "autoconfigkolabldap.h"

#include <QDomDocument>

AutoconfigKolabLdap::AutoconfigKolabLdap(QObject *parent)
    : AutoconfigKolabMail(parent)
{

}

void AutoconfigKolabLdap::lookupInDb(bool auth, bool crypt)
{
    if (serverType() == DataBase) {
        setServerType(IspAutoConfig);
    }

    startJob(lookupUrl(QLatin1String("ldap"), QLatin1String("1.0"), auth, crypt));
}

void AutoconfigKolabLdap::parseResult(const QDomDocument &document)
{
    const QDomElement docElem = document.documentElement();
    const QDomNodeList l = docElem.elementsByTagName(QLatin1String("ldapProvider"));

    if (l.isEmpty()) {
        emit finished(false);
        return;
    }

    for (int i = 0; i < l.count(); ++i) {
        QDomElement e = l.at(i).toElement();
        ldapServer s = createLdapServer(e);
        if (s.isValid()) {
            mLdapServers[e.attribute(QLatin1String("id"))] = s;
        }
    }

    emit finished(true);
}

ldapServer AutoconfigKolabLdap::createLdapServer(const QDomElement &n)
{
    QDomNode o = n.firstChild();
    ldapServer s;
    while (!o.isNull()) {
        QDomElement f = o.toElement();
        if (!f.isNull()) {
            const QString tagName(f.tagName());
            if (tagName == QLatin1String("hostname")) {
                s.hostname = replacePlaceholders(f.text());
            } else if (tagName == QLatin1String("port")) {
                s.port = f.text().toInt();
            } else if (tagName == QLatin1String("socketType")) {
                const QString type(f.text());
                if (type == QLatin1String("plain")) {
                    s.socketType = KLDAP::LdapServer::None;
                } else if (type == QLatin1String("SSL")) {
                    s.socketType =  KLDAP::LdapServer::SSL;
                } else if (type == QLatin1String("TLS")) {
                    s.socketType = KLDAP::LdapServer::TLS;
                }
            } else if (tagName == QLatin1String("authentication")) {
                const QString type(f.text());
                if (type == QLatin1String("anonyoum")) {
                    s.authentication = KLDAP::LdapServer::Anonymous;
                } else if (type == QLatin1String("simple")) {
                    s.authentication = KLDAP::LdapServer::Simple;
                } else if (type == QLatin1String("sasl")) {
                    s.authentication = KLDAP::LdapServer::SASL;
                }
            } else if (tagName == QLatin1String("bindDn")) {
                s.bindDn = f.text();
            } else if (tagName == QLatin1String("sasl-mech")) {
                s.saslMech = f.text();
            } else if (tagName == QLatin1String("username")) {
                s.username = f.text();
            } else if (tagName == QLatin1String("password")) {
                s.password = f.text();
            } else if (tagName == QLatin1String("realm")) {
                s.realm = f.text();
            } else if (tagName == QLatin1String("dn")) {
                s.dn = f.text();
            } else if (tagName == QLatin1String("ldapVersion")) {
                s.ldapVersion = f.text().toInt();
            } else if (tagName == QLatin1String("filter")) {
                s.filter = f.text();
            } else if (tagName == QLatin1String("pagesize")) {
                s.pageSize = f.text().toInt();
                if (s.pageSize < 1 || s.pageSize > 9999999) {
                    s.pageSize = -1;
                }
            } else if (tagName == QLatin1String("timelimit")) {
                s.timeLimit = f.text().toInt();
                if (s.timeLimit < 1 || s.timeLimit > 9999999) {
                    s.timeLimit = -1;
                }
            } else if (tagName == QLatin1String("sizelimit")) {
                s.sizeLimit = f.text().toInt();
                if (s.sizeLimit < 1 || s.sizeLimit > 9999999) {
                    s.sizeLimit = -1;
                }
            }
        }
        o = o.nextSibling();
    }
    return s;
}

QHash< QString, ldapServer > AutoconfigKolabLdap::ldapServers() const
{
    return mLdapServers;
}
