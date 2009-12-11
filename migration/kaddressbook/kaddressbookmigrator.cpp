/*
    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

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

#include <QtCore/QByteArray>
#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFile>

#include <kabc/vcardconverter.h>
#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <kconfig.h>
#include <kconfiggroup.h>
#include <kglobal.h>
#include <klocale.h>
#include <kstandarddirs.h>

KABC::Addressee::List readContacts( bool *ok )
{
  const QString fileName = KStandardDirs::locateLocal( "data", "kabc/std.vcf" );
  QFile file( fileName );
  if ( !file.open( QIODevice::ReadOnly ) ) {
    qDebug( "Unable to open file %s for reading", qPrintable( fileName ) );
    *ok = false;
    return KABC::Addressee::List();
  }

  const QByteArray content = file.readAll();
  file.close();

  KABC::VCardConverter converter;

  *ok = true;
  return converter.parseVCards( content );
}

bool writeContacts( const KABC::Addressee::List &contacts )
{
  const QString path = QDir::home().absolutePath() + "/.local/share/contacts/";
  if ( !QDir::root().mkpath( path ) )
    return false;

  KABC::VCardConverter converter;
  for ( int i = 0; i < contacts.count(); ++i ) {
    const KABC::Addressee contact = contacts.at( i );
    const QByteArray content = converter.createVCard( contact );

    const QString fileName = path + QDir::separator() + contact.uid() + ".vcf";
    QFile file( fileName );
    if ( !file.open( QIODevice::WriteOnly ) ) {
      qDebug( "Unable to open file %s for writing", qPrintable( fileName ) );
      return false;
    }

    file.write( content );
    file.close();
  }

  return true;
}

void convertAddressBook()
{
  /* The conversion is done by reading the file based default kresource address book
   * $HOME/.kde/share/apps/kabc/std.vcf and creating a directory based address book
   * under $HOME/.local/share/contacts.
   */

  bool ok = false;
  const KABC::Addressee::List contacts = readContacts( &ok );
  if ( !ok )
    return;

  writeContacts( contacts );
}

int main( int argc, char **argv )
{
  KAboutData aboutData( "kaddressbookmigrator", QByteArray(), ki18n( "Migration tool for the KDE address book" ), "0.1" );
  aboutData.addAuthor( ki18n( "Tobias Koenig" ), ki18n( "Author" ), "tokoe@kde.org" );

  KCmdLineArgs::init( argc, argv, &aboutData );
  KCmdLineOptions options;
  options.add( "disable-autostart", ki18n( "Disable automatic startup on login" ) );
  KCmdLineArgs::addCmdLineOptions( options );

  QCoreApplication app( KCmdLineArgs::qtArgc(), KCmdLineArgs::qtArgv() );

  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

  if ( args->isSet( "disable-autostart" ) ) {
    KSharedConfigPtr config = KGlobal::config();
    KConfigGroup group( config, "Startup" );
    group.writeEntry( "EnableAutostart", false );
  }

  convertAddressBook();

  return 0;
}
