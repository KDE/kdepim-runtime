/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

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

#include "dialog.h"
#include "global.h"

#include <akonadi/control.h>

#include <kaboutdata.h>
#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kglobal.h>

#include <QtDBus/QDBusConnection>

int main( int argc, char **argv )
{
  KAboutData aboutData( "accountwizard", 0,
                        ki18n( "Account Assistant" ),
                        "0.1",
                        ki18n( "Helps setting up PIM accounts" ),
                        KAboutData::License_LGPL,
                        ki18n( "(c) 2009 the Akonadi developers" ),
                        KLocalizedString(),
                        "http://pim.kde.org/akonadi/" );
  aboutData.setProgramIconName( "akonadi" );
  aboutData.addAuthor( ki18n( "Volker Krause" ),  ki18n( "Author" ), "vkrause@kde.org" );
  aboutData.addAuthor( ki18n( "Laurent Montel" ), KLocalizedString() , "montel@kde.org" );

  KCmdLineArgs::init( argc, argv, &aboutData );
  KCmdLineOptions options;
  options.add( "type <type>", ki18n("Only offer accounts that support the given type.") );
  options.add( "assistant <assistant>", ki18n("Run the specified assistant.") );
  options.add( "package <fullpath>", ki18n("unpack fullpath on startup and launch that assistant") );
  KCmdLineArgs::addCmdLineOptions( options );
  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

  KApplication app;
  KGlobal::locale()->insertCatalog( "libakonadi" );

  Akonadi::Control::start( 0 );

  if ( !args->getOption( "package" ).isEmpty() ) {
    Global::setAssistant( Global::unpackAssistant( args->getOption( "package" ) ) );
  } else
    Global::setAssistant( args->getOption( "assistant" ) );

  QString typesIdentifier = QLatin1String( "defaulttype" );
  if ( !args->getOption("type").isEmpty() ) {
    typesIdentifier = args->getOption( "type" );
    Global::setTypeFilter( args->getOption( "type" ).split( ',' ) );
  }
  typesIdentifier.remove( QLatin1Char( ' ' ) );
  typesIdentifier.replace( QLatin1Char( ',' ), QLatin1Char( '_' ) );
  typesIdentifier.replace( QLatin1Char( '/' ), QLatin1Char( '_' ) );

  args->clear();

  if ( !QDBusConnection::sessionBus().registerService( QString::fromLatin1( "org.kde.pim.AccountWizard.lock.%1" ).arg( typesIdentifier ) ) ) {
    // another acccount wizard instance is holding this lock -> quit this instance
    return 0;
  }

  Dialog dlg;
  dlg.show();
  dlg.resize(300,350);

  return app.exec();
}
