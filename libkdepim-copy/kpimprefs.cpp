/*
    This file is part of libkdepim.

    Copyright (c) 2002 Cornelius Schumacher <schumacher@kde.org>

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
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

#include <time.h>
#include <unistd.h>

#include <qstring.h>

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
  kdDebug(5300) << "KPimPrefs::usrReadConfig()" << endl;

  config()->setGroup("General");
  mCustomCategories = config()->readListEntry("Custom Categories");
  if (mCustomCategories.isEmpty()) setCategoryDefaults();
}

const QString KPimPrefs::timezone()
{
  QString zone = "";

  // Read TimeZoneId from korganizerrc.
  KConfig korgcfg( locate( "config", QString::fromLatin1("korganizerrc") ) );
  korgcfg.setGroup( "Time & Date" );
  QString tz(korgcfg.readEntry( "TimeZoneId" ) );
  if ( ! tz.isEmpty() ) 
  {
    zone = tz;
    kdDebug(5300) << "timezone from korganizerrc is " << zone << endl;
  }

  // If timezone not found in KOrg, use the system's default timezone.
  if ( zone.isEmpty() )
  {
    char zonefilebuf[PATH_MAX];

    // readlink does not return a null-terminated string.
    memset(zonefilebuf, '\0', PATH_MAX);
    int len = readlink("/etc/localtime", zonefilebuf, PATH_MAX);
    if ( len > 0 && len < PATH_MAX ) 
    {
        zone = QString::fromLocal8Bit(zonefilebuf);
        zone = zone.mid(zone.find("zoneinfo/") + 9);
        kdDebug(5300) << "system timezone from /etc/localtime is " << zone << endl;
    } 
    else 
    {
        tzset();
        zone = tzname[0];
        kdDebug(5300)  << "system timezone from tzset() is " << zone << endl;
    }
  }

  return( zone );

}

void KPimPrefs::usrWriteConfig()
{
  config()->setGroup("General");
  config()->writeEntry("Custom Categories",mCustomCategories);
}
