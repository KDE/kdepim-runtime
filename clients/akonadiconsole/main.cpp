/*
    This file is part of Akonadi.

    Copyright (c) 2006 Tobias Koenig <tokoe@kde.org>

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
    USA.
*/

#include <kaboutdata.h>
#include <kapplication.h>
#include <kcmdlineargs.h>

#include "mainwindow.h"

int main( int argc, char **argv )
{
  KAboutData aboutData( "akonadiconsole", 0,
                        ki18n( "Akonadi Console" ),
                        "0.99",
                        ki18n( "The Management and Debugging Console for Akonadi" ),
                        KAboutData::License_GPL,
                        ki18n( "(c) 2006-2008 the Akonadi developer" ),
                        KLocalizedString(),
                        "http://pim.kde.org/akonadi/" );
  aboutData.addAuthor( ki18n( "Tobias König" ), ki18n( "Author" ), "tokoe@kde.org" );
  aboutData.addAuthor( ki18n( "Volker Krause" ),  ki18n( "Author" ), "vkrause@kde.org" );

  KCmdLineArgs::init( argc, argv, &aboutData );
  KApplication app;

  MainWindow *window = new MainWindow;
  window->show();

  return app.exec();
}
