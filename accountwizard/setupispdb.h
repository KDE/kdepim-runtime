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

#ifndef SETUPISPDB_H
#define SETUPISPDB_H

#include "setupobject.h"
#include "ispdb/ispdb.h"

class Identity;

class SetupIspdb : public SetupObject
{
    Q_OBJECT
public:
    /** Constructor */
    explicit SetupIspdb(QObject *parent = 0);
    SetupIspdb(QObject *parent, Ispdb *ispdb);
    ~SetupIspdb();

    virtual void create();
    virtual void destroy();

public slots:
    Q_SCRIPTABLE QStringList relevantDomains() const;
    Q_SCRIPTABLE QString name(int l) const;

    Q_SCRIPTABLE void fillImapServer(int i, QObject *) const;
    Q_SCRIPTABLE int countImapServers() const;

    Q_SCRIPTABLE void fillSmtpServer(int i, QObject *) const;
    Q_SCRIPTABLE int countSmtpServers() const;

    Q_SCRIPTABLE void fillIdentitiy(int i, QObject *) const;
    Q_SCRIPTABLE int countIdentities() const;
    Q_SCRIPTABLE int defaultIdentity() const;

    Q_SCRIPTABLE void start();

    Q_SCRIPTABLE void setEmail(const QString &);
    Q_SCRIPTABLE void setPassword(const QString &);

signals:
    void ispdbFinished(bool);

protected slots:
    void onIspdbFinished(bool);

protected :
    Ispdb *mIspdb;

};

#endif
