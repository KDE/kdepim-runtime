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

#include <kglobal.h>
#include <kconfig.h>
#include <klocale.h>
#include <kdebug.h>
#include <kdirwatch.h>
#include <kstandarddirs.h>

#include "kpimprefs.h"

KPimPrefs::KPimPrefs( const QString &name )
  : QObject( 0 ), KConfigSkeleton( name )
{
  if ( !name.isEmpty() ) {
    KDirWatch *watch = new KDirWatch( this, "ConfigDirWatcher" );
    watch->addFile( locateLocal( "config", name ) );

    connect( watch, SIGNAL( dirty(const QString&) ), this, SLOT( reloadConfig() ) );

    watch->startScan();
  }
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


void KPimPrefs::usrWriteConfig()
{
  config()->setGroup("General");
  config()->writeEntry("Custom Categories",mCustomCategories);
}

void KPimPrefs::reloadConfig()
{
  config()->reparseConfiguration();
  readConfig();
  usrReadConfig();
}

#include "kpimprefs.moc"
