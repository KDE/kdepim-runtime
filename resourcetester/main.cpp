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

#include <AkonadiCore/control.h>


#include <KAboutData>

#include <KDebug>

#include <QCoreApplication>
#include <QtCore/QTimer>
#include <QFileInfo>

#include <signal.h>
#include <QApplication>
#include <KLocalizedString>
#include <QCommandLineParser>
#include <QCommandLineOption>

void sigHandler( int signal)
{
  Q_UNUSED( signal );
  QCoreApplication::quit();
}

int main(int argc, char *argv[])
{
  QString path;

  KAboutData aboutData( QLatin1String("akonadi-RT"),
                        i18n( "Akonadi Resource Tester" ),
                        QLatin1String("1.0"),
                        i18n( "Resource Tester" ),
                        KAboutLicense::GPL,
                        i18n( "(c) 2009 Igor Trindade Oliveira" ) );

    QApplication app(argc, argv);
    QCommandLineParser parser;
    KAboutData::setApplicationData(aboutData);
    parser.addVersionOption();
    parser.addHelpOption();
  parser.addOption(QCommandLineOption(QStringList() << QLatin1String("c") << QLatin1String("config"), i18n( "Configuration file to open" ), QLatin1String("configfile"), QLatin1String("script.js/py/rb" )));

    //PORTING SCRIPT: adapt aboutdata variable if necessary
    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);

  if ( parser.isSet( QLatin1String("config") ) )
    path.append(parser.value( QLatin1String("config") )) ;
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
