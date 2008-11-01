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

#include "kabcmigrator.h"

#include "vcardsettings.h"

#include <akonadi/agentinstance.h>
#include <akonadi/agentinstancecreatejob.h>
#include <akonadi/agentmanager.h>
#include <akonadi/agenttype.h>

#include <KDebug>
#include <KGlobal>

using namespace Akonadi;

KABCMigrator::KABCMigrator() :
    KResMigrator<KABC::Resource>( "contact", "akonadi_kabc_resource" )
{
}

bool KABCMigrator::migrateResource( KABC::Resource* res)
{
  kDebug() << res->identifier() << res->type();
  if ( res->type() == "file" )
    migrateFileResource( res );
  else
    return false;
  return true;
}

void KABCMigrator::migrateFileResource(KABC::Resource * res)
{
  const KConfigGroup kresCfg = kresConfig( res );
  const QString format = kresCfg.readEntry( "FileFormat", "" );
  if ( format != "vcard" ) {
    migrationFailed( i18n("Unsupported file format found. "
        "The file format '%1' is no longer supported, please convert to another one.", format ) );
    return;
  }
  const AgentType type = AgentManager::self()->type( "akonadi_vcard_resource" );
  if ( !type.isValid() ) {
    migrationFailed( "Unable to obtain vcard resource type." );
    return;
  }
  AgentInstanceCreateJob *job = new AgentInstanceCreateJob( type, this );
  connect( job, SIGNAL(result(KJob*)), SLOT(fileResourceCreated(KJob*)) );
  job->start();
}

void KABCMigrator::fileResourceCreated(KJob * job)
{
  if ( job->error() ) {
    migrationFailed( i18n("Failed to create resource: %1", job->errorText()) );
    return;
  }
  KABC::Resource *res = currentResource();
  AgentInstance instance = static_cast<AgentInstanceCreateJob*>( job )->instance();
  const KConfigGroup kresCfg = kresConfig( res );
  instance.setName( kresCfg.readEntry( "ResourceName", "Migrated Addressbook" ) );

  OrgKdeAkonadiVCardSettingsInterface *iface = new OrgKdeAkonadiVCardSettingsInterface( "org.freedesktop.Akonadi.Resource." + instance.identifier(),
      "/Settings", QDBusConnection::sessionBus(), this );
  if ( !iface->isValid() ) {
    migrationFailed( "Failed to obtain D-Bus interface for remote configuration.", instance );
    return;
  }
  iface->setPath( kresCfg.readPathEntry( "FileName", KStandardDirs::locateLocal( "data", "kabc/std.vcf" ) ) );
  iface->setReadOnly( res->readOnly() );
  instance.reconfigure();
  migrationCompleted( instance );
}

#include "kabcmigrator.moc"
