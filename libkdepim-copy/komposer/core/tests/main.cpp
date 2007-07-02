/**
 * main.cpp
 *
 * Copyright (C)  2004  Zack Rusin <zack@kde.org>
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
#include "testfactory.h"
#include <kaboutdata.h>
#include <kapplication.h>
#include <kdebug.h>
#include <klocale.h>
#include <kcmdlineargs.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main( int argc, char** argv )
{
    KAboutData aboutData( "tests", 0,ki18n("Test"),"0.1" );
    KCmdLineArgs::init( argc, argv, &aboutData );

    KCmdLineOptions options;
    options.add("verbose", ki18n("Verbose output"));
    KCmdLineArgs::addCmdLineOptions( options );

    KApplication app;

    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
    Q_UNUSED( args );

    TestFactory t;
    return t.runTests();
}
