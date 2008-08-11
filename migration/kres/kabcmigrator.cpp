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

KABCMigrator::KABCMigrator()
{
  migrateType( "contact" );
}

KABCMigrator::~KABCMigrator()
{
}

void KABCMigrator::migrateResource( KABC::Resource* res)
{
  kDebug() << res->identifier() << res->type();
  if ( res->type() == "file" )
    migrateFileResource( res );
  else
    migrateNext();
}

void KABCMigrator::migrateFileResource(KABC::Resource * res)
{
  const KConfigGroup kresCfg = kresConfig( res );
  if ( kresCfg.readEntry( "FileFormat", "" ) != "vcard" ) {
    kDebug() << "Unsupported file format found!";
    return;
  }
  const AgentType type = AgentManager::self()->type( "akonadi_vcard_resource" );
  if ( !type.isValid() ) {
    kDebug() << "Unable to obtain vcard resource type!";
    return;
  }
  AgentInstanceCreateJob *job = new AgentInstanceCreateJob( type, this );
  connect( job, SIGNAL(result(KJob*)), SLOT(fileResourceCreated(KJob*)) );
  mJobMap.insert( job, res );
  job->start();
}

void KABCMigrator::fileResourceCreated(KJob * job)
{
  if ( job->error() ) {
    kDebug() << "Failed to create vcard resource!";
    return;
  }
  Q_ASSERT( mJobMap.contains( job ) );
  KABC::Resource *res = mJobMap.take( job );
  AgentInstance instance = static_cast<AgentInstanceCreateJob*>( job )->instance();
  const KConfigGroup kresCfg = kresConfig( res );
  instance.setName( kresCfg.readEntry( "ResourceName", "Migrated Addressbook" ) );

  OrgKdeAkonadiVCardSettingsInterface *iface = new OrgKdeAkonadiVCardSettingsInterface( "org.freedesktop.Akonadi.Resource." + instance.identifier(),
      "/Settings", QDBusConnection::sessionBus(), this );
  if ( !iface->isValid() ) {
    kDebug() << "Failed to obtain dbus interface for resource configuration!";
    return;
  }
  iface->setPath( kresCfg.readPathEntry( "FileName", "" ) );
  iface->setReadOnly( res->readOnly() );
  instance.reconfigure();
  resourceMigrated( res );
  migrateNext();
}

#include "kabcmigrator.moc"
