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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307  USA
 */

#include "core.h"

#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kuniqueapplication.h>

#include <qlabel.h>

static const char *description =
    I18N_NOOP( "A KDE Mail Editing Manager" );

static const char *version = "0.0.1 (CVS)";

int main(int argc, char **argv)
{
  KAboutData about( "komposertest", I18N_NOOP( "KomposerTest" ), version, description,
                    KAboutData::License_GPL, "(C) 2001-2003 The Kontact developers", 0, "http://kontact.kde.org", "zack@kde.org" );
  about.addAuthor( "Zack Rusin", 0, "zack@kde.org" );

  KCmdLineArgs::init( argc, argv, &about );
  KUniqueApplication app;

  // see if we are starting with session management
  if ( app.isRestored() )
    RESTORE( Komposer::Core )
  else {
    // no session.. just start up normally
    Komposer::Core *mw = new Komposer::Core;
    mw->show();
  }

  return app.exec();
}
