/*
  Copyright (c) 2013, 2014 Montel Laurent <montel@kde.org>

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "knotesmigratorconfig.h"
#include "knoteconfig.h"

#include <KGlobal>
#include <KStandardDirs>

#include <KCal/Journal>
using namespace KCal;

KNotesMigratorConfig::KNotesMigratorConfig(KCal::Journal *journal)
    : mJournal(journal),
      mConfig(0)
{
    const QString configPath = KGlobal::dirs()->saveLocation( "data", QLatin1String("knotes/") ) + QLatin1String("notes/") + journal->uid();
    if (!configPath.isEmpty()) {
        mConfig = new KNoteConfig( KSharedConfig::openConfig( configPath, KConfig::NoGlobals ) );
        mConfig->readConfig();
    }
}

KNotesMigratorConfig::~KNotesMigratorConfig()
{
    delete mConfig;
}

bool KNotesMigratorConfig::readOnly() const
{
    if (mConfig)
        return mConfig->readOnly();
    return false;
}

KNoteConfig *KNotesMigratorConfig::noteConfig()
{
    return mConfig;
}
