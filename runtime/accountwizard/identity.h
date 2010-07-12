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

#ifndef IDENTITY_H
#define IDENTITY_H

#include "setupobject.h"

namespace KPIMIdentities {
  class Identity;
  class IdentityManager;
};

class Identity : public SetupObject
{
  Q_OBJECT
  public:
    explicit Identity( QObject *parent = 0 );
    ~Identity();
    void create();
    void destroy();

  public slots:
    Q_SCRIPTABLE void setRealName( const QString & name );
    Q_SCRIPTABLE void setEmail( const QString &email );
    Q_SCRIPTABLE void setOrganization( const QString &org );
    Q_SCRIPTABLE uint uoid() const;

  protected:
    QString identityName() const;

  private:
    QString m_realName;
    QString m_email;
    QString m_organization;
    KPIMIdentities::IdentityManager *m_manager;
    KPIMIdentities::Identity *m_identity;
};

#endif
