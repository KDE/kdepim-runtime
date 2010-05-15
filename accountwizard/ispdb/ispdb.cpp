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
#include <kdebug.h>
#include <kio/job.h>
#include <kio/jobclasses.h>

#include <kmime/kmime_header_parsing.h>
#include <QDomDocument>

Ispdb::Ispdb( QObject *parent ) : QObject( parent )
{
}

Ispdb::~Ispdb()
{
}

void Ispdb::setEmail( const QString& address )
{
    KMime::Types::Mailbox box;
    box.fromUnicodeString( address );
    mAddr = box.addrSpec();
}

void Ispdb::start()
{
    kDebug() << mAddr.asString();
    // we should do different things in here. But lets focus in the db first.
    lookupInDb();
}

void Ispdb::lookupInDb()
{
    const KUrl url( "https://live.mozillamessaging.com/autoconfig/v1.1/" + mAddr.domain );
    kDebug() << mAddr.domain << url;

    QMap< QString, QVariant > map;
    map["errorPage"] = false;

    KIO::TransferJob* job = KIO::get( url, KIO::NoReload, KIO::HideProgressInfo );
    job->setMetaData( map );
    connect( job, SIGNAL( result( KJob * ) ),
             this, SLOT( slotResult( KJob * ) ) );
    connect( job, SIGNAL( data( KIO::Job*, const QByteArray& ) ),
             this, SLOT( dataArrived( KIO::Job*, const QByteArray& ) ) );
}

void Ispdb::slotResult( KJob* job )
{
    if ( job->error() ) {
        kDebug() << "Fetching failed" << job->errorString();
        emit finished( false );
        return;
    }

    //kDebug() << mData;
    QDomDocument document;
    bool ok = document.setContent( mData );
    if ( !ok ) {
        kDebug() << "Could not parse xml" << mData;
        emit finished( false );
        return;
    }

    QDomElement docElem = document.documentElement();
    QDomNode m = docElem.firstChild(); // emailprovider
    QDomNode n = m.firstChild(); // emailprovider

    while ( !n.isNull() ) {
        QDomElement e = n.toElement();
        if ( !e.isNull() ) {
            //kDebug()  << qPrintable(e.tagName());
            if ( e.tagName() == "domain" )
                mDomains << e.text();
            else if ( e.tagName() == "displayName" )
                mDisplayName = e.text();
            else if ( e.tagName() == "displayShortName" )
                mDisplayShortName = e.text();
            else if ( e.tagName() == "incomingServer" && e.attribute( "type" ) == "imap" )
                mImapServers.append( createServer( e ) );
            else if ( e.tagName() == "incomingServer" && e.attribute( "type" ) == "pop3" )
                mPop3Servers.append( createServer( e ) );
            else if ( e.tagName() == "outgoingServer" && e.attribute( "type" ) == "smtp" )
                mSmtpServers.append( createServer( e ) );
        }
        n = n.nextSibling();
    }

    // comment this section out when you are tired of it...
    kDebug() << "------------------ summary --------------";
    kDebug() << "Domains" << mDomains;
    kDebug() << "Name" << mDisplayName << "(" << mDisplayShortName << ")";
    kDebug() << "Imap servers:";
    foreach( const server s, mImapServers ) {
        kDebug() << s.hostname << s.port << s.socketType << s.username << s.authentication;
    }
    kDebug() << "pop3 servers:";
    foreach( const server s, mPop3Servers ) {
        kDebug() << s.hostname << s.port << s.socketType << s.username << s.authentication;
    }
    kDebug() << "smtp servers:";
    foreach( const server s, mSmtpServers ) {
        kDebug() << s.hostname << s.port << s.socketType << s.username << s.authentication;
    }
    // end section.

    emit finished( true );
}

server Ispdb::createServer( const QDomElement& n )
{
    QDomNode o = n.firstChild();
    server s;
    while ( !o.isNull() ) {
        QDomElement f = o.toElement();
        if ( !f.isNull() ) {
            if ( f.tagName() == "hostname" )
                s.hostname = replacePlaceholders( f.text() );
            else if ( f.tagName() == "port" )
                s.port = f.text().toInt();
            else if ( f.tagName() == "socketType" ) {
                if ( f.text() == "plain" )
                    s.socketType = Plain;
                else if ( f.text() == "SSL" )
                    s.socketType = SSL;
                if ( f.text() == "STARTTLS" )
                    s.socketType = StartTLS;
            } else if ( f.tagName() == "username" ) {
                s.username = replacePlaceholders( f.text() );
            } else if ( f.tagName() == "authentication" ) {
                if ( f.text() == "password-cleartext" || f.text() == "plain" )
                    s.authentication = Cleartext;
                else if ( f.text() == "password-encrypted" || f.text() == "secure" )
                    s.authentication = Secure;
                else if ( f.text() == "NTLM" )
                    s.authentication = NTLM;
                else if ( f.text() == "GSSAPI" )
                    s.authentication = GSSAPI;
                else if ( f.text() == "client-ip-based" )
                    s.authentication = ClientIP;
                else if ( f.text() == "none" )
                    s.authentication = None;
            }
        }
        o = o.nextSibling();
    }
    return s;
}

QString Ispdb::replacePlaceholders( const QString& in )
{
    QString out( in );
    out.replace( "%EMAILLOCALPART%", mAddr.localPart );
    out.replace( "%EMAILADDRESS%", mAddr.asString() );
    out.replace( "%EMAILDOMAIN%", mAddr.domain );
    return out;
}

void Ispdb::dataArrived( KIO::Job*, const QByteArray& data )
{
    mData.append( data );
}

// The getters

QStringList Ispdb::relevantDomains() const
{
    return mDomains;
}

QString Ispdb::name( length l ) const
{
    if ( l == Long )
        return mDisplayName;
    else if ( l == Short )
        return  mDisplayShortName;
    else
        return QString(); //make compiler happy. Not me.
}

QList< server > Ispdb::imapServers() const
{
    return mImapServers;
}

QList< server > Ispdb::pop3Servers() const
{
    return mPop3Servers;
}

QList< server > Ispdb::smtpServers() const
{
    return mSmtpServers;
}

#include "ispdb.moc"
