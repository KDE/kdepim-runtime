/*
    Copyright (c) 2010 Omat Holding B.V. <info@omat.nl>
    Copyright (c) 2014  Sandro Knauß <knauss@kolabsys.com>

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
#include <KLocalizedString>

#include <kmime/kmime_header_parsing.h>
#include <QDomDocument>

Ispdb::Ispdb( QObject *parent )
    : QObject( parent )
    , mServerType( DataBase )
    , mStart( true )
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

void Ispdb::setPassword( const QString &password )
{
    mPassword = password;
}


void Ispdb::start()
{
    // we should do different things in here. But lets focus in the db first.
    lookupInDb(false, false);
}

void Ispdb::lookupInDb( bool auth, bool crypt )
{
    setServerType(mServerType);
    startJob( lookupUrl(QLatin1String("mail"), QLatin1String("1.1"), auth, crypt) );
}

void Ispdb::startJob( const KUrl&url )
{
    mData.clear();
    QMap< QString, QVariant > map;
    map[QLatin1String("errorPage")] = false;

    KIO::TransferJob* job = KIO::get( url, KIO::NoReload, KIO::HideProgressInfo );
    job->setMetaData( map );
    connect( job, SIGNAL(result(KJob*)),
             this, SLOT(slotResult(KJob*)) );
    connect( job, SIGNAL(data(KIO::Job*,QByteArray)),
             this, SLOT(dataArrived(KIO::Job*,QByteArray)) );
}

KUrl Ispdb::lookupUrl( const QString &type, const QString &version, bool auth, bool crypt )
{
    KUrl url;
    const QString path = type + QLatin1String("/config-v") + version + QLatin1String(".xml");

    switch( mServerType )
    {
    case IspAutoConfig:
    {
        url = KUrl( QLatin1String("http://autoconfig.") + mAddr.domain.toLower() + QLatin1Char('/') + path );
        break;
    }
    case IspWellKnow:
    {
        url = KUrl( QLatin1String("http://") + mAddr.domain.toLower() + QLatin1String("/.well-known/autoconfig/") + path );
        break;
    }
    case DataBase:
    {
        url = KUrl( QLatin1String("https://autoconfig.thunderbird.net/v1.1/") + mAddr.domain.toLower() );
        break;
    }
    }

    if ( mServerType != DataBase ) {
        if (crypt) {
            url.setProtocol(QLatin1String("https"));
        }

        if(auth) {
            url.setUser(mAddr.asString());
            url.setPass(mPassword);
        }
    }
    return url;
}


void Ispdb::slotResult( KJob* job )
{
    if ( job->error() ) {
      kDebug() << "Fetching failed" << job->errorString();
      bool lookupFinished = false;

      switch( mServerType ) {
      case IspAutoConfig: {
        mServerType = IspWellKnow;
        break;
      }
      case IspWellKnow: {
        lookupFinished = true;
        break;
      }
      case DataBase: {
        mServerType = IspAutoConfig;
        break;
      }
      }

      if ( lookupFinished )
      {
        emit finished( false );
        return;
      }
      lookupInDb( false, false );
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

    parseResult( document );

}

void Ispdb::parseResult( const QDomDocument &document )
{
    QDomElement docElem = document.documentElement();
    QDomNodeList l = docElem.elementsByTagName(QLatin1String("emailProvider"));

    if ( l.isEmpty() ) {
        emit finished(false);
        return;
    }

    //only handle the first emailProvider entry
    QDomNode firstProvider = l.at(0);
    QDomNode n = firstProvider.firstChild();
    while( !n.isNull() ) {
        QDomElement e = n.toElement();
        if ( !e.isNull() ) {
            //kDebug()  << qPrintable( e.tagName() );
          const QString tagName( e.tagName() );
            if ( tagName == QLatin1String( "domain" ) )
                mDomains << e.text();
            else if ( tagName == QLatin1String( "displayName" ) )
                mDisplayName = e.text();
            else if ( tagName == QLatin1String( "displayShortName" ) )
                mDisplayShortName = e.text();
            else if ( tagName == QLatin1String( "incomingServer" )
                      && e.attribute( QLatin1String("type") ) == QLatin1String( "imap" ) ) {
                server s = createServer( e );
                if (s.isValid())
                   mImapServers.append( s );
            } else if ( tagName == QLatin1String( "incomingServer" )
                      && e.attribute( QLatin1String("type") ) == QLatin1String( "pop3" ) ) {
                server s = createServer( e );
                if (s.isValid())
                   mPop3Servers.append( s );
            } else if ( tagName == QLatin1String( "outgoingServer" )
                      && e.attribute( QLatin1String("type") ) == QLatin1String( "smtp" ) ) {
                server s = createServer( e );
                if (s.isValid())
                   mSmtpServers.append( s );
            } else if ( tagName == QLatin1String( "identity" ) ) {
                identity i = createIdentity( e );
                if ( i.isValid() ) {
                    mIdentities.append( i );
                    if ( i.isDefault() ) {
                        mDefaultIdentity = mIdentities.count()-1;
                    }
                }
            }
        }
        n = n.nextSibling();
    }

    emit finished( true );
}


server Ispdb::createServer( const QDomElement& n )
{
    QDomNode o = n.firstChild();
    server s;
    while ( !o.isNull() ) {
        QDomElement f = o.toElement();
        if ( !f.isNull() ) {
          const QString tagName( f.tagName() );
            if ( tagName == QLatin1String( "hostname" ) )
                s.hostname = replacePlaceholders( f.text() );
            else if ( tagName == QLatin1String( "port" ) )
                s.port = f.text().toInt();
            else if ( tagName == QLatin1String( "socketType" ) ) {
              const QString type( f.text() );
                if ( type == QLatin1String( "plain" ) )
                    s.socketType = None;
                else if ( type == QLatin1String( "SSL" ) )
                    s.socketType = SSL;
                if ( type == QLatin1String( "STARTTLS" ) )
                    s.socketType = StartTLS;
            } else if ( tagName == QLatin1String( "username" ) ) {
                s.username = replacePlaceholders( f.text() );
            } else if ( tagName == QLatin1String( "authentication" ) ) {
              const QString type( f.text() );
                if ( type == QLatin1String( "password-cleartext" )
                     || type == QLatin1String( "plain" ) )
                  s.authentication = Plain;
                else if ( type == QLatin1String( "password-encrypted" )
                          || type == QLatin1String( "secure" ) )
                  s.authentication = CramMD5;
                else if ( type == QLatin1String( "NTLM" ) )
                    s.authentication = NTLM;
                else if ( type == QLatin1String( "GSSAPI" ) )
                    s.authentication = GSSAPI;
                else if ( type == QLatin1String( "client-ip-based" ) )
                    s.authentication = ClientIP;
                else if ( type == QLatin1String( "none" ) )
                    s.authentication = NoAuth;
            }
        }
        o = o.nextSibling();
    }
    return s;
}

identity Ispdb::createIdentity( const QDomElement& n )
{
    QDomNode o = n.firstChild();
    identity i;

    //type="kolab" version="1.0" is the only identity that is defined
    if ( n.attribute(QLatin1String("type")) != QLatin1String("kolab")
            || n.attribute(QLatin1String("version")) != QLatin1String("1.0") ) {
        kDebug() << "unknown type of identity element.";
    }

    while ( !o.isNull() ) {
        QDomElement f = o.toElement();
        if ( !f.isNull() ) {
          const QString tagName( f.tagName() );
          if ( tagName == QLatin1String( "default" ) ) {
              i.mDefault = f.text().toLower() == QLatin1String( "true" );
          } else if ( tagName == QLatin1String( "email" ) ) {
              i.email = f.text();
          } else if ( tagName == QLatin1String( "name" ) ) {
              i.name = f.text();
          } else if ( tagName == QLatin1String( "organization" ) ) {
              i.organization = f.text();
          } else if ( tagName == QLatin1String( "signature" ) ) {
              QTextStream stream(&i.signature);
              f.save(stream, 0);
              i.signature = i.signature.trimmed();
              if ( i.signature.startsWith(QLatin1String("<signature>")) ) {
                  i.signature = i.signature.mid(11,i.signature.length()-23);
                  i.signature = i.signature.trimmed();
              }
          }
        }
        o = o.nextSibling();
    }

    return i;
}

QString Ispdb::replacePlaceholders( const QString& in )
{
    QString out( in );
    out.replace( QLatin1String("%EMAILLOCALPART%"), mAddr.localPart );
    out.replace( QLatin1String("%EMAILADDRESS%"), mAddr.asString() );
    out.replace( QLatin1String("%EMAILDOMAIN%"), mAddr.domain );
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

int Ispdb::defaultIdentity() const
{
    return mDefaultIdentity;
}

QList< identity > Ispdb::identities() const
{
    return mIdentities;
}

void Ispdb::setServerType( Ispdb::searchServerType type )
{
    if ( type != mServerType || mStart ) {
        mServerType = type;
        mStart  = false;
        switch( mServerType )
        {
        case IspAutoConfig:
        {
            Q_EMIT searchType(i18n("Lookup configuration: Email provider"));
            break;
        }
        case IspWellKnow:
        {
            Q_EMIT searchType(i18n("Lookup configuration: Trying common server name"));
            break;
        }
        case DataBase:
        {
            Q_EMIT searchType(i18n("Lookup configuration: Mozilla database"));
            break;
        }
        }
    }
}

Ispdb::searchServerType Ispdb::serverType() const
{
    return mServerType;
}
