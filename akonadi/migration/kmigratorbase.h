/*
    Copyright (c) 2009 Jonathan Armond <jon.armond@gmail.com>

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

#ifndef KMIGRATORBASE_H
#define KMIGRATORBASE_H

#include <akonadi/agentinstance.h>

#include <QObject>

/**
 * Base class for akonadi migrators.
 */
class KMigratorBase : public QObject
{
  Q_OBJECT
  public:
    enum MigrationState
    {
      None,
      Bridged,
      Complete
    };

    enum MessageType
    {
      Success,
      Skip,
      Info,
      Warning,
      Error
    };

    Q_ENUMS( MigrationState )

    KMigratorBase();
    virtual ~KMigratorBase();

    MigrationState migrationState( const QString &identifier ) const;
    void setMigrationState( const QString &identifier, MigrationState state,
                            const QString &resId, const QString &type );

    virtual void migrateNext() = 0;

  protected:
    void createAgentInstance( const QString &typeId, QObject *reciver, const char* slot );
    virtual void migrationFailed( const QString &errorMsg, const Akonadi::AgentInstance &instance
                                  = Akonadi::AgentInstance() ) = 0;

  signals:
    void message( KMigratorBase::MessageType type, const QString &msg );

  protected slots:
    virtual void migrate() = 0;

};

#endif
