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

#ifndef SETUPMANAGER_H
#define SETUPMANAGER_H

#include <QtCore/QObject>

class SetupObject;
class SetupPage;

class SetupManager : public QObject
{
  Q_OBJECT
  public:
    explicit SetupManager( QObject *parent );
    void setSetupPage( SetupPage* page );

  public slots:
    Q_SCRIPTABLE QObject* createResource( const QString &type );
    Q_SCRIPTABLE QObject* createTransport( const QString &type );
    Q_SCRIPTABLE QObject* createConfigFile( const QString &configName );
    Q_SCRIPTABLE void execute();

  private:
    void setupNext();
    void rollback();

  private slots:
    void setupSucceeded( const QString &msg );
    void setupFailed( const QString &msg );
    void setupInfo( const QString &msg );

  private:
    QList<SetupObject*> m_objectToSetup;
    QList<SetupObject*> m_setupObjects;
    SetupObject* m_currentSetupObject;
    SetupPage* m_page;
};

#endif
