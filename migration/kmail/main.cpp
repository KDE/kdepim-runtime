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

#include <akonadi/control.h>

#include <KAboutData>
#include <KApplication>
#include <KCmdLineArgs>
#include <KGlobal>
#include <KDebug>

#include <QDBusConnection>
#include <KMessageBox>

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

  KCmdLineArgs::init( argc, argv, &aboutData );
  KCmdLineOptions options;
  options.add( "interactive", ki18n( "Show reporting dialog") );
  options.add( "interactive-on-change", ki18n("Show report only if changes were made") );
  KCmdLineArgs::addCmdLineOptions( options );
  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

  KApplication *app = new KApplication();
  app->setQuitOnLastWindowClosed( false );

  KGlobal::setAllowQuit( true );
  KGlobal::locale()->insertCatalog( "libakonadi" );

  const QString warningMsg = i18n(
    "<p>You are using a development version of KMail 2. Migration of existing settings, data and meta-data is not yet completely implemented. "
    "The following data will currently be migrated:<ul>"
    "<li>Account settings</li>"
    "<li>Appearance settings</li>"
    "</ul>The following will not happen yet and <b>cannot be done at a later point</b> anymore if you continue now:<ul>"
    "<li>Locally cached IMAP data (will be re-downloaded), pending IMAP changes (synchronize with KMail 1 first)</li>"
    "<li>Flags and tags on local mail folders</li>"
    "<li>Folder settings</li>"
    "</ul></p>"
    "<p>Selecting cancel here will not alter your data in any kind but will also prevent KMail 2 from starting. Selecting continue will perform "
    "the migration procedure as far as currently implemented and then start KMail 2. "
    "Unless you are fully aware of the implications noted above choose cancel and use KMail 1 until the migration tool has been completed.</p>"
  );
  int doYouReallyWantToBreakThings = KMessageBox::warningContinueCancel( 0, warningMsg, i18n( "KMail 2 Migration" ), KStandardGuiItem::cont(), KStandardGuiItem::cancel(), QString(), KMessageBox::Dangerous );
  if ( doYouReallyWantToBreakThings != KMessageBox::Continue )
    return 1;
  
  if ( !Akonadi::Control::start( 0 ) )
    return 2;

  InfoDialog *infoDialog = 0;
  if ( args->isSet( "interactive" ) || args->isSet( "interactive-on-change" ) ) {
    infoDialog = new InfoDialog( args->isSet( "interactive-on-change" ) );
    infoDialog->show();
  }

  KMailMigrator *migrator = new KMailMigrator;
  if ( infoDialog && migrator ) {
    infoDialog->migratorAdded();
    QObject::connect( migrator, SIGNAL( message( KMigratorBase::MessageType, QString ) ),
                      infoDialog, SLOT( message( KMigratorBase::MessageType, QString ) ) );
    QObject::connect( migrator, SIGNAL( destroyed() ), infoDialog, SLOT( migratorDone() ) );
  }

  const int result = app->exec();
  if ( infoDialog && infoDialog->hasError() )
    return 3;
  return result;
}
