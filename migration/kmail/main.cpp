/*
    Copyright (c) 2009 Jonathan Armond <jon.armond@gmail.com>
    Copyright (c) 2010 Volker Krause <vkrause@kde.org>

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

#include "kmailmigrator.h"

#include "infodialog.h"

#include <AkonadiCore/control.h>

#include <KAboutData>
#include <QApplication>
#include <KGlobal>
#include <QDebug>
#include <KSharedConfig>
#include <KLocale>
#include <QCommandLineParser>
#include <QCommandLineOption>


using namespace KMail;

int main( int argc, char **argv )
{
  KLocalizedString::setApplicationDomain("kmail-migrator");
  KAboutData aboutData( QStringLiteral("kmail-migrator"),
                        i18n( "KMail Migration Tool" ),
                        QStringLiteral("0.1"),
                        i18n( "Migration of KMail accounts to Akonadi" ),
                        KAboutLicense::LGPL,
                        i18n( "(c) 2009-2010 the Akonadi developers" ),
                        QStringLiteral("http://pim.kde.org/akonadi/") );
  aboutData.setProgramIconName( QLatin1String("akonadi") );
  aboutData.addAuthor( i18n( "Jonathan Armond" ),  i18n( "Author" ), QStringLiteral("jon.armond@gmail.com") );

  QCommandLineParser parser;
  QApplication app(argc, argv);
  parser.addVersionOption();
  parser.addHelpOption();
  parser.addOption(QCommandLineOption(QStringList() << QLatin1String("interactive"), i18n( "Show reporting dialog" )));
  parser.addOption(QCommandLineOption(QStringList() << QLatin1String("interactive-on-change"), i18n( "Show report only if changes were made" )));

  //PORTING SCRIPT: adapt aboutdata variable if necessary
  aboutData.setupCommandLine(&parser);
  parser.process(app);
  aboutData.processCommandLine(&parser);

  app.setQuitOnLastWindowClosed( false );

  KGlobal::setAllowQuit( true );
  if ( !Akonadi::Control::start( 0 ) )
    return 2;

  InfoDialog *infoDialog = 0;
  if ( parser.isSet( QStringLiteral("interactive") ) || parser.isSet( QStringLiteral("interactive-on-change") ) ) {
    infoDialog = new InfoDialog( parser.isSet( QStringLiteral("interactive-on-change") ) );
    infoDialog->show();
  }

  // Don't run the migration twice
  // The second time, it would only copy kmailrc over kmail2rc and
  // not migrate the accounts and folders again...
  KConfigGroup migrationCfg( KSharedConfig::openConfig(), "Migration" );
  const bool enabled = migrationCfg.readEntry( "Enabled", false );
  const int currentVersion = migrationCfg.readEntry( "Version", 0 );
  const int targetVersion = migrationCfg.readEntry( "TargetVersion", 1 );
  if ( !enabled || currentVersion >= targetVersion ) {
    qWarning() << "Migration of kmailrc has already run, not running it again";
    return 4;
  }

  KMailMigrator *migrator = new KMailMigrator;
  if ( infoDialog && migrator ) {
    infoDialog->migratorAdded();
    QObject::connect( migrator, SIGNAL(message(KMigratorBase::MessageType,QString)),
                      infoDialog, SLOT(message(KMigratorBase::MessageType,QString)) );
    QObject::connect( migrator, SIGNAL(destroyed()), infoDialog, SLOT(migratorDone()) );
    QObject::connect( migrator, SIGNAL(status(QString)), infoDialog, SLOT(status(QString)) );
    QObject::connect( migrator, SIGNAL(progress(int)), infoDialog, SLOT(progress(int)) );
    QObject::connect( migrator, SIGNAL(progress(int,int,int)),
                      infoDialog, SLOT(progress(int,int,int)) );
  }
  
  const int result = app.exec();
  if ( InfoDialog::hasError() )
    return 3;

  // if we have succeeded, update version information
  migrationCfg.writeEntry( "Version", targetVersion );
  migrationCfg.sync();

  return result;
}
