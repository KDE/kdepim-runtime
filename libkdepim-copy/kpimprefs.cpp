/*
    This file is part of libkdepim.
    Copyright (c) 2002 Cornelius Schumacher <schumacher@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    As a special exception, permission is given to link this program
    with any edition of Qt, and distribute the resulting executable,
    without including the source code for Qt in the source distribution.
*/

#include <kglobal.h>
#include <kconfig.h>
#include <klocale.h>
#include <kdebug.h>

#include "kpimprefs.h"

KPimPrefs::KPimPrefs() :
  KPrefs("")
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


void KPimPrefs::usrWriteConfig()
{
  config()->setGroup("General");
  config()->writeEntry("Custom Categories",mCustomCategories);
}
