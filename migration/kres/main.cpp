/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

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

#include "kabcmigrator.h"
#include "kcalmigrator.h"
#include "infodialog.h"

#include <akonadi/control.h>

#include <kaboutdata.h>
#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kglobal.h>

void connectMigrator( KResMigratorBase *m, InfoDialog *dlg )
{
  if ( !dlg || !m )
    return;
  dlg->migratorAdded();
  QObject::connect( m, SIGNAL(successMessage(QString)), dlg, SLOT(successMessage(QString)) );
  QObject::connect( m, SIGNAL(infoMessage(QString)), dlg, SLOT(infoMessage(QString)) );
  QObject::connect( m, SIGNAL(errorMessage(QString)), dlg, SLOT(errorMessage(QString)) );
  QObject::connect( m, SIGNAL(destroyed()), dlg, SLOT(migratorDone()) );
}

int main( int argc, char **argv )
{
  KAboutData aboutData( "kres-migrator", 0,
                        ki18n( "KResource Migration Tool" ),
                        "0.1",
                        ki18n( "Migration of KResource settings and application to Akonadi" ),
                        KAboutData::License_LGPL,
                        ki18n( "(c) 2008 the Akonadi developers" ),
                        KLocalizedString(),
                        "http://pim.kde.org/akonadi/" );
  aboutData.setProgramIconName( "akonadi" );
  aboutData.addAuthor( ki18n( "Volker Krause" ),  ki18n( "Author" ), "vkrause@kde.org" );

  KCmdLineArgs::init( argc, argv, &aboutData );
  KCmdLineOptions options;
  options.add( "bridge-only", ki18n("Only migrate to Akonadi KResource bridges") );
  options.add( "contacts-only", ki18n("Only migrate contact resources") );
  options.add( "calendar-only", ki18n("Only migrate calendar resources") );
  options.add( "interactive", ki18n( "Do not show reporting dialog") );
  KCmdLineArgs::addCmdLineOptions( options );
  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

  KApplication app;
  app.setQuitOnLastWindowClosed( false );

  KGlobal::setAllowQuit( true );

  Akonadi::Control::start();

  InfoDialog *infoDialog = 0;
  if ( args->isSet( "interactive" ) ) {
    infoDialog = new InfoDialog();
    infoDialog->show();
  }

  if ( !args->isSet( "calendar-only" ) ) {
    KABCMigrator *m = new KABCMigrator();
    m->setBridgingOnly( args->isSet( "bridge-only" ) );
    connectMigrator( m, infoDialog );
  }
  if ( !args->isSet( "contacts-only" ) ) {
    KCalMigrator *m = new KCalMigrator();
    m->setBridgingOnly( args->isSet( "bridge-only" ) );
    connectMigrator( m, infoDialog );
  }

  return app.exec();
}
