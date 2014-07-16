/*
 * Copyright (C) 2014  Sandro Knauß <knauss@kolabsys.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 22 of the License, or
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

#ifndef AUTOCONFIGKOLABFREEBUSY_H
#define AUTOCONFIGKOLABFREEBUSY_H

#include "autoconfigkolabmail.h"

struct freebusy;

class AutoconfigKolabFreebusy : public AutoconfigKolabMail
{
public:
    /** Constructor */
    explicit AutoconfigKolabFreebusy(QObject *parent = 0);

    QHash<QString, freebusy> freebusyServers() const;

protected:
    virtual void lookupInDb(bool auth, bool crypt);
    virtual void parseResult(const QDomDocument &document);

private:
    freebusy createFreebusyServer(const QDomElement &n);

    QHash<QString, freebusy> mFreebusyServer;
};

struct freebusy {
    freebusy()
        : port(80)
        , socketType(Ispdb::None)
        , authentication(Ispdb::Plain)
    {
    }
    bool isValid() const {
        return (port != -1);
    }
    QString hostname;
    int port;
    Ispdb::socketType socketType;
    Ispdb::authType authentication;
    QString username;
    QString password;
    QString path;
};

#endif // AUTOCONFIGKOLABFREEBUSY_H
