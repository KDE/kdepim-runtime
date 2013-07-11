/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

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

#ifndef RESOURCE_H
#define RESOURCE_H

#include "setupobject.h"
#include <akonadi/agentinstance.h>
#include <QtCore/QMap>

class KJob;

class Resource : public SetupObject
{
  Q_OBJECT
  public:
    explicit Resource( const QString &type, QObject *parent = 0 );
    void create();
    void destroy();

  public slots:
    Q_SCRIPTABLE void setName( const QString &name );
    Q_SCRIPTABLE void setOption( const QString &key, const QVariant &value );
    Q_SCRIPTABLE QString identifier();
    Q_SCRIPTABLE void reconfigure();

  private slots:
    void instanceCreateResult( KJob* job );

  private:
    QString m_typeIdentifier, m_name;
    QMap<QString, QVariant> m_settings;
    Akonadi::AgentInstance m_instance;
};
#endif
