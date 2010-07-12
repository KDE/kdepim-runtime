/*
 * Copyright (c) 2009  Igor Trindade Oliveira <igor_trindade@yahoo.com.br>
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
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "resource.h"
#include "script.h"
#include "xmloperations.h"
#include "global.h"
#include "test.h"
#include "collectiontest.h"
#include "itemtest.h"
#include "system.h"
#include "qemu.h"

#include <akonadi/control.h>

#include <KApplication>
#include <KAboutData>
#include <KCmdLineArgs>
#include <KDebug>

#include <QCoreApplication>
#include <QtCore/QTimer>
#include <QFileInfo>

#include <signal.h>

void sigHandler( int signal)
{
  Q_UNUSED( signal );
  QCoreApplication::quit();
}

int main(int argc, char *argv[])
{
  QString path;

  KAboutData aboutdata( "akonadi-RT", 0,
                        ki18n( "Akonadi Resource Tester" ),
                        "1.0",
                        ki18n( "Resource Tester" ),
                        KAboutData::License_GPL,
                        ki18n( "(c) 2009 Igor Trindade Oliveira" ) );

  KCmdLineArgs::init( argc, argv, &aboutdata );


  KCmdLineOptions options;
  options.add( "c" ).add( "config <configfile>", ki18n( "Configuration file to open" ), "script.js/py/rb" );
  KCmdLineArgs::addCmdLineOptions( options );

  KApplication app;

  const KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

  if ( args->isSet( "config" ) )
    path.append(args->getOption( "config" )) ;
  else
    return -1;
  Global::setBasePath( QFileInfo( path ).absolutePath() );

#ifdef Q_OS_UNIX
  signal( SIGINT, sigHandler );
  signal( SIGQUIT, sigHandler );
#endif

  if ( !Akonadi::Control::start() )
    qFatal( "Unable to start Akonadi!" );

  Script *script = new Script();

  script->configure(path);
  script->insertObject( new XmlOperations( Global::parent() ), "XmlOperations" );
  script->insertObject( new Resource( Global::parent() ), "Resource" );
  script->insertObject( Test::instance(), "Test" );
  script->insertObject( new CollectionTest( Global::parent() ), "CollectionTest" );
  script->insertObject( new ItemTest( Global::parent() ), "ItemTest" );
  script->insertObject( new System( Global::parent() ), "System" );
  script->insertObject( new QEmu( Global::parent() ), "QEmu" );
  QTimer::singleShot( 0, script, SLOT(start()) );

  const int result = app.exec();
  Global::cleanup();
  return result;
}
