/**
 * test.cpp
 *
 * Copyright (C)  2003  Zack Rusin <zack@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#include "core.h"

#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kuniqueapplication.h>

#include <QLabel>

static const char description[] =
    I18N_NOOP( "KDE mail editing manager" );

static const char version[] = "0.0.1 (SVN)";

int main(int argc, char **argv)
{
  KAboutData about( "komposertest", 0, ki18n( "KomposerTest" ), version, ki18n(description),
                    KAboutData::License_GPL, ki18n("(C) 2001-2003 The Kontact developers"), KLocalizedString(), "http://kontact.kde.org", "zack@kde.org" );
  about.addAuthor( ki18n("Zack Rusin"), KLocalizedString(), "zack@kde.org" );

  KCmdLineArgs::init( argc, argv, &about );
  KUniqueApplication app;

  // see if we are starting with session management
  if ( app.isSessionRestored() )
    RESTORE( Komposer::Core )
  else {
    // no session.. just start up normally
    Komposer::Core *mw = new Komposer::Core;
    mw->show();
  }

  return app.exec();
}
