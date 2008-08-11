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
#include "kcalmigrator.h"

#include <akonadi/control.h>

#include <kaboutdata.h>
#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kglobal.h>

int main( int argc, char **argv )
{
  KAboutData aboutData( "kres-migrator", 0,
                        ki18n( "KResource Migration Tool" ),
                        "0.1",
                        ki18n( "Migration of KResource settings and application to Akonadi" ),
                        KAboutData::License_LGPL,
                        ki18n( "(c) 2008 the Akonadi developers" ),
                        KLocalizedString(),
                        "http://pim.kde.org/akonadi/" );
  aboutData.setProgramIconName( "akonadi" );
  aboutData.addAuthor( ki18n( "Volker Krause" ),  ki18n( "Author" ), "vkrause@kde.org" );

  KCmdLineArgs::init( argc, argv, &aboutData );
  KApplication app;
  app.setQuitOnLastWindowClosed( false );

  KGlobal::setAllowQuit( true );

  Akonadi::Control::start();

  new KABCMigrator();
  new KCalMigrator();

  return app.exec();
}
