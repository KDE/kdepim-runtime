/*
  Copyright (C) 2009 Allen Winter <winter@kde.org>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

#include <KApplication>
#include <KCmdLineArgs>

#include <kcal/event.h>

#include "../kincidencechooser.h"
using namespace KPIM;

int main( int argc, char **argv )
{
  KCmdLineArgs::init( argc, argv, "testkincidencechooser", 0,
                      ki18n( "KIncidenceChooserTest" ), "1.0",
                      ki18n( "kincidencechooser test app" ) );
  KApplication app;
  KIncidenceChooser *chooser = new KIncidenceChooser();

  Event event;
  event.setSummary( i18n( "Meeting" ) );
  event.setDescription( i18n( "Discuss foo" ) );
  chooser->setIncidence( &event, &event );
  chooser->resize( 600, 600 );
  chooser->show();
  return app.exec();
}
