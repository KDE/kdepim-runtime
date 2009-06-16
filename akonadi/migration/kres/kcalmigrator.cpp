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

#include "kcalmigrator.h"

#include "icalsettings.h"
#include "birthdayssettings.h"

#include <akonadi/agentinstance.h>
#include <akonadi/agentinstancecreatejob.h>
#include <akonadi/agentmanager.h>
#include <akonadi/agenttype.h>

#include <KDebug>

using namespace Akonadi;

KCalMigrator::KCalMigrator() :
    KResMigrator<KCal::ResourceCalendar>( "calendar", "akonadi_kcal_resource" )
{
}

bool KCalMigrator::migrateResource( KCal::ResourceCalendar* res)
{
  kDebug() << res->identifier() << res->type();
  if ( res->type() == "file" )
    createAgentInstance( "akonadi_ical_resource", this, SLOT(fileResourceCreated(KJob*)) );
  else if ( res->type() == "birthdays" )
    createAgentInstance( "akonadi_birthdays_resource", this, SLOT(birthdaysResourceCreated(KJob*)) );
  else
    return false;
  return true;
}

void KCalMigrator::fileResourceCreated(KJob * job)
{
  if ( job->error() ) {
    migrationFailed( i18n("Failed to create resource: %1", job->errorText()) );
    return;
  }
  KCal::ResourceCalendar *res = currentResource();
  AgentInstance instance = static_cast<AgentInstanceCreateJob*>( job )->instance();
  const KConfigGroup kresCfg = kresConfig( res );
  instance.setName( kresCfg.readEntry( "ResourceName", "Migrated Calendar" ) );

  OrgKdeAkonadiICalSettingsInterface *iface = new OrgKdeAkonadiICalSettingsInterface( "org.freedesktop.Akonadi.Resource." + instance.identifier(),
      "/Settings", QDBusConnection::sessionBus(), this );
  if ( !iface->isValid() ) {
    migrationFailed( "Failed to obtain D-Bus interface for remote configuration.", instance );
    return;
  }
  iface->setPath( kresCfg.readPathEntry( "CalendarURL", "" ) );
  iface->setReadOnly( res->readOnly() );
  instance.reconfigure();
  migrationCompleted( instance );
}

void KCalMigrator::birthdaysResourceCreated(KJob* job)
{
  if ( job->error() ) {
    migrationFailed( i18n("Failed to create birthdays resource: %1", job->errorText() ) );
    return;
  }
  KCal::ResourceCalendar *res = currentResource();
  AgentInstance instance = static_cast<AgentInstanceCreateJob*>( job )->instance();
  const KConfigGroup kresCfg = kresConfig( res );
  instance.setName( kresCfg.readEntry( "ResourceName", "Migrated Birthdays" ) );

  OrgKdeAkonadiBirthdaysSettingsInterface *iface =
    new OrgKdeAkonadiBirthdaysSettingsInterface( "org.freedesktop.Akonadi.Resource." + instance.identifier(),
                                                 "/Settings", QDBusConnection::sessionBus(), this );
  if ( !iface->isValid() ) {
    migrationFailed( "Failed to obtain D-Bus interface for remote configuration.", instance );
    return;
  }
  iface->setEnableAlarm( kresCfg.readEntry( "Alarm", true ) );
  iface->setAlarmDays( kresCfg.readEntry( "AlarmDays", 1 ) );
  iface->setFilterOnCategories( kresCfg.readEntry( "UseCategories", false ) );
  iface->setFilterCategories( kresCfg.readEntry( "Categories", QStringList() ) );
  instance.reconfigure();
  migrationCompleted( instance );
}


#include "kcalmigrator.moc"
