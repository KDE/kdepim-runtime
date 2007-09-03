/*
    This file is part of KContactManager.

    Copyright (c) 2007 Tobias Koenig <tokoe@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <klocale.h>
#include <kuniqueapplication.h>

#include "mainwindow.h"

int main( int argc, char **argv )
{
  KAboutData *about = new KAboutData( "kcontactmanager", 0, ki18n( "KContactManager" ),
                                      "0.1", ki18n( "The KDE Contact Management Application" ),
                                      KAboutData::License_GPL_V2,
                                      ki18n( "(c) 2007, The KDE PIM Team" ) );
  about->addAuthor( ki18n("Tobias Koenig"), ki18n( "Current maintainer" ), "tokoe@kde.org" );

  KCmdLineArgs::init( argc, argv, about );

  KCmdLineOptions options;
  KCmdLineArgs::addCmdLineOptions( options );
  KUniqueApplication::addCmdLineOptions();

  if ( !KUniqueApplication::start() )
    exit( 0 );

  KUniqueApplication app;

  MainWindow *window = new MainWindow;
  window->show();

  app.exec();
}
