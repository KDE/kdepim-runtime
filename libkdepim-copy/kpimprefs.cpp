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

#include <config.h>

#include <time.h>
#include <unistd.h>
#include <stdlib.h>

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
  mCustomCategories = config()->readListEntry( "Custom Categories" );
  if ( mCustomCategories.isEmpty() ) setCategoryDefaults();
  mCustomCategories.sort();
}

const QString KPimPrefs::timezone()
{
  QString zone = "";

  // Read TimeZoneId from korganizerrc.
  KConfig korgcfg( locate( "config", "korganizerrc" ) );
  korgcfg.setGroup( "Time & Date" );
  QString tz( korgcfg.readEntry( "TimeZoneId" ) );
  if ( !tz.isEmpty() ) {
    zone = tz;
    kdDebug(5300) << "timezone from korganizerrc is " << zone << endl;
  }

  // If timezone not found in KOrg, use the system's default timezone.
  if ( zone.isEmpty() ) {
    char zonefilebuf[ PATH_MAX ];

    int len = readlink( "/etc/localtime", zonefilebuf, PATH_MAX );
    if ( len > 0 && len < PATH_MAX ) {
      zone = QString::fromLocal8Bit( zonefilebuf, len );
      zone = zone.mid( zone.find( "zoneinfo/" ) + 9 );
      kdDebug(5300) << "system timezone from /etc/localtime is " << zone
                    << endl;
    } else {
      tzset();
      zone = tzname[ 0 ];
      kdDebug(5300) << "system timezone from tzset() is " << zone << endl;
    }
  }

  return( zone );
}

QDateTime KPimPrefs::utcToLocalTime( const QDateTime &_dt,
                                     const QString &timeZoneId )
{
  QDateTime dt(_dt);
//  kdDebug() << "---   UTC: " << dt.toString() << endl;

  int yearCorrection = 0;
  // The timezone conversion only works for dates > 1970
  // For dates < 1970 we adjust the date to be in 1970, 
  // do the correction there and then re-adjust back.
  // Actually, we use 1971 to prevent errors around
  // January 1, 1970
  int year = dt.date().year();
  if (year < 1971)
  {
    yearCorrection = 1971 - year;
    dt = dt.addYears(yearCorrection);
//    kdDebug() << "---   Adjusted UTC: " << dt.toString() << endl;
  }
  
  QCString origTz = getenv("TZ");

  setenv( "TZ", "UTC", 1 );
  time_t utcTime = dt.toTime_t();

  setenv( "TZ", timeZoneId.local8Bit(), 1 );
  struct tm *local = localtime( &utcTime );

  if ( origTz.isNull() ) {
    unsetenv( "TZ" );
  } else {
    setenv( "TZ", origTz, 1 );
  }
  tzset();

  QDateTime result( QDate( local->tm_year + 1900 - yearCorrection,
                           local->tm_mon + 1, local->tm_mday ),
                    QTime( local->tm_hour, local->tm_min, local->tm_sec ) );

//  kdDebug() << "--- LOCAL: " << result.toString() << endl;
  return result;
}

QDateTime KPimPrefs::localTimeToUtc( const QDateTime &_dt,
                                     const QString &timeZoneId )
{
  QDateTime dt(_dt);
//  kdDebug() << "--- LOCAL: " << dt.toString() << endl;

  int yearCorrection = 0;
  // The timezone conversion only works for dates > 1970
  // For dates < 1970 we adjust the date to be in 1970, 
  // do the correction there and then re-adjust back.
  // Actually, we use 1971 to prevent errors around
  // January 1, 1970
  
  int year = dt.date().year();
  if (year < 1971)
  {
    yearCorrection = 1971 - year;
    dt = dt.addYears(yearCorrection);
//    kdDebug() << "---   Adjusted LOCAL: " << dt.toString() << endl;
  }

  QCString origTz = getenv("TZ");

  setenv( "TZ", timeZoneId.local8Bit(), 1 );
  time_t localTime = dt.toTime_t();

  setenv( "TZ", "UTC", 1 );
  struct tm *utc = gmtime( &localTime );

  if ( origTz.isNull() ) {
    unsetenv( "TZ" );
  } else {
    setenv( "TZ", origTz, 1 );
  }
  tzset();

  QDateTime result( QDate( utc->tm_year + 1900 - yearCorrection,
                           utc->tm_mon + 1, utc->tm_mday ),
                    QTime( utc->tm_hour, utc->tm_min, utc->tm_sec ) );

//  kdDebug() << "---   UTC: " << result.toString() << endl;

  return result;
}

void KPimPrefs::usrWriteConfig()
{
  config()->setGroup( "General" );
  config()->writeEntry( "Custom Categories", mCustomCategories );
}
