/*
    Copyright (c) 2010 Stephen Kelly <steveire@gmail.com>

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


#include "kjotsmigrator.h"

#include "infodialog.h"

#include <AkonadiCore/control.h>

#include <KAboutData>
#include <QApplication>

#include <KGlobal>
#include <KLocalizedString>
#include <QCommandLineParser>
#include <QCommandLineOption>


int main( int argc, char **argv )
{
  KLocalizedString::setApplicationDomain("kjotsmigrator"); 
  KAboutData aboutData( QStringLiteral("kjotsmigrator"),
                        i18n( "KJots Migration Tool" ),
                        QStringLiteral("0.1"),
                        i18n( "Migration of KJots notes to Akonadi" ),
                        KAboutLicense::LGPL,
                        i18n( "(c) 2010 the Akonadi developers" ),
                        QStringLiteral("http://pim.kde.org/akonadi/") );
  aboutData.setProgramIconName( QLatin1String("akonadi") );
  aboutData.addAuthor( i18n( "Stephen Kelly" ),  i18n( "Author" ), QLatin1String("steveire@gmail.com") );

  QCommandLineParser parser;
  QApplication app(argc, argv);
  parser.addVersionOption();
  parser.addHelpOption();
  parser.addOption(QCommandLineOption(QStringList() << QLatin1String("interactive"), i18n( "Show reporting dialog" )));
  parser.addOption(QCommandLineOption(QStringList() << QLatin1String("interactive-on-change"), i18n( "Show report only if changes were made" )));

  aboutData.setupCommandLine(&parser);
  parser.process(app);
  aboutData.processCommandLine(&parser);

  app.setQuitOnLastWindowClosed( false );

  KGlobal::setAllowQuit( true );

  if ( !Akonadi::Control::start( 0 ) )
    return 2;

  InfoDialog *infoDialog = 0;
  if ( parser.isSet( QLatin1String("interactive") ) || parser.isSet( QLatin1String("interactive-on-change") ) ) {
    infoDialog = new InfoDialog( parser.isSet( QLatin1String("interactive-on-change") ) );
    infoDialog->show();
  }
  

  KJotsMigrator *migrator = new KJotsMigrator;
  if ( infoDialog && migrator ) {
    infoDialog->migratorAdded();
    QObject::connect( migrator, SIGNAL(message(KMigratorBase::MessageType,QString)),
                      infoDialog, SLOT(message(KMigratorBase::MessageType,QString)) );
    QObject::connect(migrator, &KJotsMigrator::destroyed, infoDialog, &InfoDialog::migratorDone);
  }

  const int result = app.exec();
  if ( InfoDialog::hasError() )
    return 3;
  return result;
}
