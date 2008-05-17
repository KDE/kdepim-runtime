/* This file is part of the KDE project
   Copyright (C) 2008 Omat Holding B.V. <info@omat.nl>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include <QDBusInterface>
#include <kuniqueapplication.h>
#include <kstartupinfo.h>
#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <stdio.h>
#include <stdlib.h>

#include "dock.h"

/**
 * @class AkonadiTrayApplication
 * @author Tom Albers <tomalbers@kde.nl>
 * This class is a simple inheritance from KUniqueApplication
 * the reason that it is reimplemented is that when AkonadiTray
 * is launched a second time it would in the orinal implementation
 * make the show(). which we do not want. This
 * class misses that call.
 */
class AkonadiTrayApplication : public KUniqueApplication
{
public:
    /**
     * Similar to KUniqueApplication::newInstance, only without
     * the call to raise the widget when a second instance is started.
     */
    int newInstance() {
        return 0;
    }
};

int main( int argc, char *argv[] )
{
    KAboutData aboutData( "akonaditray", 0,
                          ki18n( "AkonadiTray" ),
                          "0.1",
                          ki18n( "Systemtray application to control basic Akonadi functions" ),
                          KAboutData::License_GPL,
                          ki18n( "(c) 2008 Omat Holding B.V." ),
                          KLocalizedString() );
    aboutData.addAuthor( ki18n( "Tom Albers" ), ki18n( "Maintainer and Author" ),
                         "tomalbers@kde.nl", "http://www.omat.nl" );
    aboutData.setProgramIconName( "akonadi" );

    KCmdLineArgs::init( argc, argv, &aboutData );

    if ( !KUniqueApplication::start() ) {
        fprintf( stderr, "akonaditray is already running!\n" );
        exit( 0 );
    }

    AkonadiTrayApplication a;
    a.setQuitOnLastWindowClosed( false );

    Tray tray;

    return a.exec();
}
