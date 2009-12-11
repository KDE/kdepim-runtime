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

#include "kmigratorbase.h"

#include <akonadi/agentinstancecreatejob.h>
#include <akonadi/agentmanager.h>
#include <akonadi/agenttype.h>

#include <KSharedConfig>
#include <KConfigGroup>
#include <KGlobal>
#include <KLocalizedString>

#include <QMetaEnum>
#include <QTimer>

using namespace Akonadi;

KMigratorBase::KMigratorBase()
{
  KGlobal::ref();

  // load the vtable before we continue
  QTimer::singleShot( 0, this, SLOT(migrate()) );
}

KMigratorBase::~KMigratorBase()
{
  KGlobal::deref();
}

KMigratorBase::MigrationState KMigratorBase::migrationState( const QString &identifier ) const
{
  KConfigGroup cfg( KGlobal::config(), "Resource " + identifier );
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

void KMigratorBase::setMigrationState( const QString &identifier, MigrationState state,
                        const QString &resId, const QString &type )
{
  KConfigGroup cfg( KGlobal::config(), "Resource " + identifier );
  QMetaEnum e = metaObject()->enumerator( metaObject()->indexOfEnumerator( "MigrationState" ) );
  const QString stateStr = e.valueToKey( state );
  cfg.writeEntry( "MigrationState", stateStr );
  cfg.writeEntry( "ResourceIdentifier", resId );
  cfg.sync();

  cfg = KConfigGroup( KGlobal::config(), "Bridged" );
  QStringList bridgedResources = cfg.readEntry( type + "Resources", QStringList() );
  if ( state == Bridged ) {
    if ( !bridgedResources.contains( identifier ) )
      bridgedResources << identifier;
  } else {
    bridgedResources.removeAll( identifier );
  }
  cfg.writeEntry( type + "Resources", bridgedResources );
  cfg.sync();
}

void KMigratorBase::createAgentInstance(const QString& typeId, QObject* receiver, const char* slot)
{
  const AgentType type = AgentManager::self()->type( typeId );
  if ( !type.isValid() ) {
    migrationFailed( i18n("Unable to obtain resource type '%1'.", typeId) );
    return;
  }
  AgentInstanceCreateJob *job = new AgentInstanceCreateJob( type, this );
  connect( job, SIGNAL( result( KJob* ) ), receiver, slot );
  job->start();
}

#include "kmigratorbase.moc"
