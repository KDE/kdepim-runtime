/*
    Copyright (c) 2010 Laurent Montel <montel@kde.org>

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

#ifndef LDAP_H
#define LDAP_H

#include "setupobject.h"

class Ldap : public SetupObject
{
    Q_OBJECT
public:
    explicit Ldap(QObject *parent = 0);
    ~Ldap();
    void create();
    void destroy();
public slots:
    Q_SCRIPTABLE void setUser(const QString &name);
    Q_SCRIPTABLE void setServer(const QString &server);
    Q_SCRIPTABLE void setAuthenticationMethod(const QString &meth);
    Q_SCRIPTABLE void setBindDn(const QString &bindDn);
    Q_SCRIPTABLE void setPassword(const QString &password);
private:
    QString m_user;
    QString m_server;
    QString m_bindDn;
    QString m_authMethod;
    QString m_password;
};

#endif
