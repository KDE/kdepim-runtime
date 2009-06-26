/*
    Copyright (c) 2009 Jonathan Armond <jon.armond@gmail.com>

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

#include <akonadi/control.h>

#include <KAboutData>
#include <KApplication>
#include <KCmdLineArgs>
#include <KGlobal>
#include <KDebug>

#include <QDBusConnection>

using namespace KMail;

int main( int argc, char **argv )
{
  KAboutData aboutData( "kmail-migrator", 0,
                        ki18n( "KMail Migration Tool" ),
                        "0.1",
                        ki18n( "Migration of KMail accounts to Akonadi" ),
                        KAboutData::License_LGPL,
                        ki18n( "(c) 2009 the Akonadi developers" ),
                        KLocalizedString(),
                        "http://pim.kde.org/akonadi/" );
  aboutData.setProgramIconName( "akonadi" );
  aboutData.addAuthor( ki18n( "Jonathan Armond" ),  ki18n( "Author" ), "jon.armond@gmail.com" ); 

  const QStringList supportedTypes = QStringList() << "imap" << "mbox" << "maildir" << "dimap"
                                                   << "local" << "pop";

  KCmdLineArgs::init( argc, argv, &aboutData );
  KCmdLineOptions options;
  options.add( "type <type>", ki18n("Only migrate the specified types (supported: imap, mbox, "
                                    "maildir, dimap, local, pop)" ),
               supportedTypes.join( "," ).toLatin1() );
  options.add( "interactive", ki18n( "Show reporting dialog") );
  options.add( "interactive-on-change", ki18n("Show report only if changes were made") );
  KCmdLineArgs::addCmdLineOptions( options );
  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

  QStringList typesToMigrate;
  foreach ( const QString &type, args->getOption( "type" ).split( ',' ) ) {
    if ( !supportedTypes.contains( type ) )
      kWarning() << "Unknown resource type: " << type;
    else if ( !QDBusConnection::sessionBus().registerService( "org.kde.Akonadi.KMailMigrator." + type ) )
      kWarning() << "Migrator instance already running for type " << type;
    else
      typesToMigrate << type;
  }
  if ( typesToMigrate.isEmpty() )
    return 1;

  KApplication app;
  app.setQuitOnLastWindowClosed( false );

  KGlobal::setAllowQuit( true );
  KGlobal::locale()->insertCatalog( "libakonadi" );
  if ( !Akonadi::Control::start( 0 ) )
    return 2;

  InfoDialog *infoDialog = 0;
  if ( args->isSet( "interactive" ) || args->isSet( "interactive-on-change" ) ) {
    infoDialog = new InfoDialog( args->isSet( "interactive-on-change" ) );
    infoDialog->show();
  }

  KMailMigrator *migrator = new KMailMigrator( typesToMigrate );
  if ( infoDialog && migrator ) {
    infoDialog->migratorAdded();
    QObject::connect( migrator, SIGNAL( message( KMigratorBase::MessageType, QString ) ),
                      infoDialog, SLOT( message( KMigratorBase::MessageType, QString ) ) );
    QObject::connect( migrator, SIGNAL( destroyed() ), infoDialog, SLOT( migratorDone() ) );
  }

  const int result = app.exec();
  if ( infoDialog && infoDialog->hasError() )
    return 3;
  return result;
}
