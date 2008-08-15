/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

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

#ifndef KRESMIGRATORBASE_H
#define KRESMIGRATORBASE_H

#include <akonadi/agentinstance.h>

#include <KConfigGroup>

#include <QObject>

namespace KRES {
  class Resource;
}

class KJob;

/**
 * Non-template QObject part of KResMigrator.
 */
class KResMigratorBase : public QObject
{
  Q_OBJECT
  public:
    enum MigrationState
    {
      None,
      Bridged,
      Complete
    };

    Q_ENUMS( MigrationState )

    KResMigratorBase( const QString &type, const QString &bridgeType );
    ~KResMigratorBase();

    MigrationState migrationState( KRES::Resource *res ) const;

    virtual void migrateNext() = 0;
    void migrateToBridge( KRES::Resource* res, const QString &typeId );

    virtual KConfigGroup kresConfig( KRES::Resource* res ) const = 0;

    void setBridgingOnly( bool b );

    void migrationCompleted( const Akonadi::AgentInstance &instance );
    void migratedToBridge( const Akonadi::AgentInstance &instance );
    void migrationFailed( const QString &errorMsg, const Akonadi::AgentInstance &instance = Akonadi::AgentInstance() );

  signals:
    void successMessage( const QString &msg );
    void infoMessage( const QString &msg );
    void errorMessage( const QString &msg );

  protected slots:
    virtual void migrate() = 0;

  protected:
    QString mType;
    QString mBridgeType;
    QStringList mPendingBridgedResources;
    bool mBridgeOnly;
    KRES::Resource *mCurrentKResource;

  private:
    void setMigrationState( KRES::Resource *res, MigrationState state, const QString &resId );

  private slots:
    void resourceBridgeCreated( KJob *job );

  private:
    bool mBridgingInProgress;

};

#endif
