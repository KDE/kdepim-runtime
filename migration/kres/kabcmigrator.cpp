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

using namespace Akonadi;

KABCMigrator::KABCMigrator() :
    KResMigrator<KABC::Resource>( QLatin1String("contact"), QLatin1String("akonadi_kabc_resource") )
{
}

bool KABCMigrator::migrateResource( KABC::Resource* res)
{
  kDebug() << res->identifier() << res->type();
  if ( res->type() == QLatin1String("file") )
    return migrateFileResource( res );
  else
    return false;
  //return true;                  this line will never been reached
}

bool KABCMigrator::migrateFileResource(KABC::Resource * res)
{
  const KConfigGroup kresCfg = kresConfig( res );
  const QString format = kresCfg.readEntry( "FileFormat", "" );
  if ( format == QLatin1String("binary") ) {
    emit message( Warning, i18n( "Unsupported file format found. "
        "The file format '%1' is no longer supported, please convert to another one.", format ) );
    return false;
  } else if ( format != QLatin1String("vcard") ) {
    emit message( Skip, i18n( "File format '%1' is not yet supported natively.", format ) );
    return false;
  }
  createAgentInstance( QLatin1String("akonadi_vcard_resource"), this, SLOT(fileResourceCreated(KJob*)) );
  return true;
}

void KABCMigrator::fileResourceCreated(KJob * job)
{
  if ( job->error() ) {
    migrationFailed( i18n( "Failed to create resource: %1", job->errorText() ) );
    return;
  }
  KABC::Resource *res = currentResource();
  AgentInstance instance = static_cast<AgentInstanceCreateJob*>( job )->instance();
  const KConfigGroup kresCfg = kresConfig( res );
  instance.setName( kresCfg.readEntry( "ResourceName", i18n("Migrated Addressbook") ) );

  OrgKdeAkonadiVCardSettingsInterface *iface = new OrgKdeAkonadiVCardSettingsInterface( QLatin1String("org.freedesktop.Akonadi.Resource.") + instance.identifier(),
      QLatin1String("/Settings"), QDBusConnection::sessionBus(), this );
  if ( !iface->isValid() ) {
    migrationFailed( i18n( "Failed to obtain D-Bus interface for remote configuration." ), instance );
    delete iface;
    return;
  }
  iface->setPath( kresCfg.readPathEntry( "FileName", KStandardDirs::locateLocal( "data", QLatin1String("kabc/std.vcf") ) ) );
  iface->setReadOnly( res->readOnly() );

  // make sure the config is saved
  iface->writeConfig();

  instance.reconfigure();
  migrationCompleted( instance );
}

