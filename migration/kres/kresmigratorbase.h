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

#include <KConfigGroup>

#include <QHash>
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

    KResMigratorBase( const QString &type );
    ~KResMigratorBase();

    MigrationState migrationState( KRES::Resource *res ) const;
    void setMigrationState( KRES::Resource *res, MigrationState state );

    void setResourceForJob( KJob* job, KRES::Resource *res );

    virtual void migrateNext() = 0;
    void migrateToBridge( KRES::Resource* res, const QString &typeId );

    virtual KConfigGroup kresConfig( KRES::Resource* res ) const = 0;

  protected slots:
    virtual void migrate() = 0;

  protected:
    QString mType;
    QHash<KJob*, KRES::Resource*> mJobResMap;

  private slots:
    void resourceBridgeCreated( KJob *job );

};

#endif
