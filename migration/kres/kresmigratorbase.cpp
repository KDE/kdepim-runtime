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

#include "kresmigratorbase.h"

#include <akonadi/agentinstance.h>
#include <akonadi/agentinstancecreatejob.h>
#include <akonadi/agentmanager.h>
#include <akonadi/agenttype.h>

#include <kresources/manager.h>
#include <kresources/resource.h>

#include <KConfigGroup>
#include <KGlobal>
#include <KStandardDirs>

#include <QMetaEnum>
#include <QTimer>

using namespace Akonadi;

KResMigratorBase::KResMigratorBase(const QString & type, const QString &bridgeType) :
    mType( type ),
    mBridgeType( bridgeType ),
    mCurrentKResource( 0 ),
    mBridgingInProgress( false )
{
  KGlobal::ref();

  KConfigGroup cfg( KGlobal::config(), "Bridged" );
  mPendingBridgedResources = cfg.readEntry( mType + "Resources", QStringList() );

  // load the vtable before we continue
  QTimer::singleShot( 0, this, SLOT(migrate()) );
}

KResMigratorBase::~ KResMigratorBase()
{
  KGlobal::deref();
}

KResMigratorBase::MigrationState KResMigratorBase::migrationState(KRES::Resource * res) const
{
  KConfigGroup cfg( KGlobal::config(), "Resource " + res->identifier() );
  QMetaEnum e = metaObject()->enumerator( metaObject()->indexOfEnumerator( "MigrationState" ) );
  const QString s = cfg.readEntry( "MigrationState", e.valueToKey( None ) );
  MigrationState state = (MigrationState)e.keyToValue( s.toLatin1() );

  if ( state != None ) {
    const QString resId = cfg.readEntry( "ResourceIdentifier", "" );
    // previously migrated but removed again
    if ( !AgentManager::self()->instance( resId ).isValid() )
      state = None;
  }

  return state;
}

void KResMigratorBase::setMigrationState( KRES::Resource * res, MigrationState state, const QString &resId )
{
  KConfigGroup cfg( KGlobal::config(), "Resource " + res->identifier() );
  QMetaEnum e = metaObject()->enumerator( metaObject()->indexOfEnumerator( "MigrationState" ) );
  QString stateStr = e.valueToKey( state );
  cfg.writeEntry( "MigrationState", stateStr );
  cfg.writeEntry( "ResourceIdentifier", resId );
  cfg.sync();

  cfg = KConfigGroup( KGlobal::config(), "Bridged" );
  QStringList bridgedResources = cfg.readEntry( mType + "Resources", QStringList() );
  if ( state == Bridged ) {
    if ( !bridgedResources.contains( res->identifier() ) )
      bridgedResources << res->identifier();
  } else {
    bridgedResources.removeAll( res->identifier() );
  }
  cfg.writeEntry( mType + "Resources", bridgedResources );
  cfg.sync();
}

void KResMigratorBase::migrateToBridge( KRES::Resource *res, const QString & typeId)
{
  kDebug() << res->type() << res->identifier() << typeId;

  if ( migrationState( res ) != None ) {
    migrateNext();
    return;
  }

  emit message( Info, i18n( "Trying to migragte '%1' to compatibility bridge...", res->resourceName() ) );
  mBridgingInProgress = true;
  const AgentType type = AgentManager::self()->type( typeId );
  if ( !type.isValid() ) {
    migrationFailed( i18n( "Unable to obtain bridge type '%1'.", mBridgeType ) );
    return;
  }
  AgentInstanceCreateJob *job = new AgentInstanceCreateJob( type, this );
  connect( job, SIGNAL(result(KJob*)), SLOT(resourceBridgeCreated(KJob*)) );
  job->start();
}

void KResMigratorBase::resourceBridgeCreated(KJob * job)
{
  kDebug();
  if ( job->error() ) {
    migrationFailed( i18n( "Unable to create compatibility bridge: %1", job->errorText() ) );
    return;
  }
  KRES::Resource *res = mCurrentKResource;
  Q_ASSERT( res );
  AgentInstance instance = static_cast<AgentInstanceCreateJob*>( job )->instance();
  const KConfigGroup kresCfg = kresConfig( res );
  instance.setName( kresCfg.readEntry( "ResourceName", "Bridged KResource" ) );

  const QString akoResCfgFile = KStandardDirs::locateLocal( "config", QString( "%1rc" ).arg( instance.identifier() ) );
  KConfig *akoResConfig = new KConfig( akoResCfgFile );
  KConfigGroup bridgeResCfg( akoResConfig, kresCfg.name() );
  kresCfg.copyTo( &bridgeResCfg );
  bridgeResCfg.writeEntry( "ResourceIsActive", true );
  bridgeResCfg.sync();

  KConfigGroup generalCfg( akoResConfig, "General" );
  generalCfg.writeEntry( "ResourceKeys", res->identifier() );
  generalCfg.writeEntry( "Standard", res->identifier() );
  generalCfg.sync();

  akoResConfig->sync();
  delete akoResConfig;

  instance.reconfigure();
  migratedToBridge( instance );
}

void KResMigratorBase::setBridgingOnly(bool b)
{
  mBridgeOnly = b;
}

void KResMigratorBase::migrationFailed(const QString & errorMsg, const Akonadi::AgentInstance & instance)
{
  if ( mBridgingInProgress ) {
    emit message( Error, i18n( "Migration of '%1' to compatibility bridge failed: %2",
                       mCurrentKResource->resourceName(), errorMsg ) );
  } else {
    emit message( Error, i18n( "Migration of '%1' to native backend failed: %2",
                       mCurrentKResource->resourceName(), errorMsg ) );
  }

  if ( instance.isValid() ) {
    AgentManager::self()->removeInstance( instance );
  }

  // native backend failed, try the bridge instead
  if ( !mBridgingInProgress && mCurrentKResource ) {
    migrateToBridge( mCurrentKResource, mBridgeType );
    return;
  }

  mBridgingInProgress = false;
  mCurrentKResource = 0;
  migrateNext();
}

#include "kresmigratorbase.moc"
