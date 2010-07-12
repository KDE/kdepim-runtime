/*
    Copyright (c) 2010 Omat Holding B.V. <info@omat.nl>

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

#include "ispdb.h"

#include <kaboutdata.h>
#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kglobal.h>

int main( int argc, char **argv )
{
    KAboutData aboutData( "ispdb", 0,
                          ki18n( "Ispdb Assistant" ),
                          "0.1",
                          ki18n( "Helps setting up PIM accounts" ),
                          KAboutData::License_LGPL,
                          ki18n( "(c) 2010 Omat Holding B.V." ),
                          KLocalizedString(),
                          "http://pim.kde.org/akonadi/" );
    aboutData.setProgramIconName( "akonadi" );
    aboutData.addAuthor( ki18n( "Tom Albers" ),  ki18n( "Author" ), "toma@kde.org" );

    KCmdLineArgs::init( argc, argv, &aboutData );
    KCmdLineOptions options;
    options.add( "email <emailaddress>", ki18n( "Tries to fetch the settings for that email address" ) );
    KCmdLineArgs::addCmdLineOptions( options );
    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

    QString email;
    if ( !args->getOption( "email" ).isEmpty() )
        email = args->getOption( "email" );
    else
        email = "blablabla@gmail.com";


    KApplication app;

    Ispdb ispdb;
    ispdb.setEmail( email );
    ispdb.start();

    return app.exec();

}
