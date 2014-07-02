/*
    Copyright (c) 2010 Omat Holding B.V. <info@omat.nl>

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

#include "ispdb.h"

#include <kaboutdata.h>
#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kglobal.h>

#include <KDebug>

QString socketTypeToStr( Ispdb::socketType socketType )
{
    QString enc = QLatin1String("None");
    if ( socketType == Ispdb::SSL ) {
        enc = QLatin1String("SSL");
    } else if ( socketType == Ispdb::StartTLS ) {
        enc = QLatin1String("TLS");
    }
    return enc;
}

QString authTypeToStr( Ispdb::authType authType )
{
    QString auth = QLatin1String("unknown");
    switch ( authType ) {
      case Ispdb::Plain:
        auth = QLatin1String("plain");
        break;
      case Ispdb::CramMD5:
        auth = QLatin1String("CramMD5");
        break;
      case Ispdb::NTLM:
        auth = QLatin1String("NTLM");
        break;
      case Ispdb::GSSAPI:
        auth = QLatin1String("GSSAPI");
        break;
      case Ispdb::ClientIP:
        auth = QLatin1String("ClientIP");
        break;
      case Ispdb::NoAuth:
        auth = QLatin1String("NoAuth");
        break;
    }
    return auth;
}
int main( int argc, char **argv )
{
    KAboutData aboutData( "ispdb", 0,
                          ki18n( "ISPDB Assistant" ),
                          "0.1",
                          ki18n( "Helps setting up PIM accounts" ),
                          KAboutData::License_LGPL,
                          ki18n( "(c) 2010 Omat Holding B.V." ),
                          KLocalizedString(),
                          "http://pim.kde.org/akonadi/" );
    aboutData.setProgramIconName( QLatin1String("akonadi") );
    aboutData.addAuthor( ki18n( "Tom Albers" ),  ki18n( "Author" ), "toma@kde.org" );

    KCmdLineArgs::init( argc, argv, &aboutData );
    KCmdLineOptions options;
    options.add( "email <emailaddress>", ki18n( "Tries to fetch the settings for that email address" ) );
    KCmdLineArgs::addCmdLineOptions( options );
    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

    QString email( QLatin1String("blablabla@gmail.com") );
    if ( !args->getOption( "email" ).isEmpty() ) {
        email = args->getOption( "email" );
    }

    KApplication app;

    QEventLoop loop;
    Ispdb ispdb;
    ispdb.setEmail( email );

    loop.connect( &ispdb, SIGNAL(finished(bool)), SLOT(quit()) );

    ispdb.start();

    loop.exec();
    kDebug() << "Domains" << ispdb.relevantDomains();
    kDebug() << "Name" << ispdb.name( Ispdb::Long ) << "(" << ispdb.name( Ispdb::Short ) << ")";
    kDebug() << "Imap servers:";
    foreach ( const server &s, ispdb.imapServers() ) {
        kDebug() << "\thostname:" << s.hostname
                 << "- port:" << s.port
                 << "- encryption:" << socketTypeToStr(s.socketType)
                 << "- username:" << s.username
                 << "- authentication:" << authTypeToStr(s.authentication);
    }
    kDebug() << "pop3 servers:";
    foreach ( const server &s, ispdb.pop3Servers() ) {
        kDebug() << "\thostname:" << s.hostname
                 << "- port:" << s.port
                 << "- encryption:" << socketTypeToStr(s.socketType)
                 << "- username:" << s.username
                 << "- authentication:" << authTypeToStr(s.authentication);
    }
    kDebug() << "smtp servers:";
    foreach ( const server &s, ispdb.smtpServers() ) {
        kDebug() << "\thostname:" << s.hostname
                 << "- port:" << s.port
                 << "- encryption:" << socketTypeToStr(s.socketType)
                 << "- username:" << s.username
                 << "- authentication:" << authTypeToStr(s.authentication);
    }

    return true;
}
