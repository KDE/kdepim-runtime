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

#include "kmigratorbase.h"

#include <KConfigGroup>

namespace KRES {
  class Resource;
}

class KJob;

/**
 * Non-template QObject part of KResMigrator.
 */
class KResMigratorBase : public KMigratorBase
{
  Q_OBJECT

  public:
    KResMigratorBase( const QString &type, const QString &bridgeType );
    virtual ~KResMigratorBase() {}

    void migrateToBridge( KRES::Resource* res, const QString &typeId );
    virtual void migratedToBridge(const Akonadi::AgentInstance & instance) = 0;

    virtual KConfigGroup kresConfig( KRES::Resource* res ) const = 0;

    void setBridgingOnly( bool b );
    void setOmitClientBridge( bool b );

    void migrationFailed( const QString &errorMsg, const Akonadi::AgentInstance &instance = Akonadi::AgentInstance() );

  protected:
    QString mType;
    QString mBridgeType;
    QStringList mPendingBridgedResources;
    bool mBridgeOnly;
    bool mOmitClientBridge;
    KRES::Resource *mCurrentKResource;
    bool mBridgingInProgress;

  private slots:
    void resourceBridgeCreated( KJob *job );
};

#endif
