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

#include "autoconfigkolabfreebusy.h"

#include <QDomDocument>

AutoconfigKolabFreebusy::AutoconfigKolabFreebusy(QObject *parent)
    : AutoconfigKolabMail(parent)
{

}

void AutoconfigKolabFreebusy::lookupInDb(bool auth, bool crypt)
{
    if (serverType() == DataBase) {
        setServerType(IspAutoConfig);
    }

    startJob(lookupUrl(QLatin1String("freebusy"), QLatin1String("1.0"), auth, crypt));
}

void AutoconfigKolabFreebusy::parseResult(const QDomDocument &document)
{
    const QDomElement docElem = document.documentElement();
    const QDomNodeList l = docElem.elementsByTagName(QLatin1String("freebusyProvider"));

    if (l.isEmpty()) {
        emit finished(false);
        return;
    }

    for (int i = 0; i < l.count(); ++i) {
        QDomElement e = l.at(i).toElement();
        freebusy s = createFreebusyServer(e);
        if (s.isValid()) {
            mFreebusyServer[e.attribute(QLatin1String("id"))] = s;
        }
    }

    emit finished(true);
}

freebusy AutoconfigKolabFreebusy::createFreebusyServer(const QDomElement &n)
{
    QDomNode o = n.firstChild();
    freebusy s;
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
                    s.socketType = None;
                } else if (type == QLatin1String("SSL")) {
                    s.socketType = SSL;
                }
                if (type == QLatin1String("TLS")) {
                    s.socketType = StartTLS;
                }
            } else if (tagName == QLatin1String("username")) {
                s.username = replacePlaceholders(f.text());
            } else if (tagName == QLatin1String("password")) {
                s.password = f.text();
            } else if (tagName == QLatin1String("authentication")) {
                const QString type(f.text());
                if (type == QLatin1String("none")
                        || type == QLatin1String("plain")) {
                    s.authentication = Plain;
                } else if (type == QLatin1String("basic")) {
                    s.authentication = Basic;
                }
            } else if (tagName == QLatin1String("path")) {
                s.path = f.text();
            }
        }
        o = o.nextSibling();
    }
    return s;
}

QHash< QString, freebusy > AutoconfigKolabFreebusy::freebusyServers() const
{
    return mFreebusyServer;
}
