/*
    This file is part of libkdepim.

    Copyright (c) 2002 Cornelius Schumacher <schumacher@kde.org>
    Copyright (c) 2005 Rafal Rzepecki <divide@users.sourceforge.net>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include <config.h>

#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>

#include <QString>

#include <kstandarddirs.h>
#include <kglobal.h>
#include <kconfig.h>
#include <klocale.h>
#include <kdebug.h>

#include "kpimprefs.h"

KPimPrefs::KPimPrefs( const QString &name )
  : KConfigSkeleton( name )
{
}

KPimPrefs::~KPimPrefs()
{
}

void KPimPrefs::usrSetDefaults()
{
  setCategoryDefaults();
}

void KPimPrefs::usrReadConfig()
{
  kDebug(5300) << "KPimPrefs::usrReadConfig()" << endl;

  config()->setGroup("General");
  mCustomCategories = config()->readEntry( "Custom Categories" , QStringList() );
  if ( mCustomCategories.isEmpty() ) setCategoryDefaults();
  mCustomCategories.sort();
}

KDateTime::Spec KPimPrefs::timeSpec()
{
  const KTimeZone *zone = 0;
	// FIXME: In KDE 4, use KSystemTimeZones::local instead of system and file system calls!

  // Read TimeZoneId from korganizerrc.
  KConfig korgcfg( KStandardDirs::locate( "config", "korganizerrc" ) );
  korgcfg.setGroup( "Time & Date" );
  QString tz( korgcfg.readEntry( "TimeZoneId" ) );
  if ( !tz.isEmpty() ) {
    zone = KSystemTimeZones::zone( tz );
    if ( zone )
      kDebug(5300) << "timezone from korganizerrc is " << tz << endl;
  }

  // If timezone not found in KOrg, use the system's default timezone.
  if ( !zone ) {
    zone = KSystemTimeZones::local();
    if ( zone )
      kDebug(5300) << "system timezone is " << zone->name() << endl;
  }

  return zone ? KDateTime::Spec( zone ) : KDateTime::ClockTime;
}

void KPimPrefs::usrWriteConfig()
{
  config()->setGroup( "General" );
  config()->writeEntry( "Custom Categories", mCustomCategories );
}

const QString KPimPrefs::categorySeparator = ":";
