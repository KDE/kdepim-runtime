/*
    Copyright (C) 2009 Omat Holding B.V. <info@omat.nl>

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

#include "communication.h"

#include <kdebug.h>
#include <klocale.h>

#include <QDomDocument>
#include <QObject>

#include <kio/job.h>
using namespace KIO;

Communication::Communication( QObject* parent )
        : QObject( parent )
{
}

Communication::~Communication()
{
}

void Communication::setService( int service )
{
    m_service = service;
}

void Communication::setCredentials( const QString &username, const QString &password )
{
    m_username = username;
    m_password = password;
}

KUrl Communication::getBaseUrl()
{
    KUrl url( serviceToApi( m_service ) );
    url.setUser( m_username );
    url.setPass( m_password );
    return url;
}

void Communication::checkAuth()
{
    KUrl url = getBaseUrl();
    url.addPath( "account/verify_credentials.xml" );
    KIO::StoredTransferJob *job = KIO::storedGet( url, Reload, HideProgressInfo ) ;
    connect( job, SIGNAL( result( KJob* ) ), this, SLOT( slotCheckAuthData( KJob* ) ) );
}

void Communication::slotCheckAuthData( KJob *job )
{
    if ( job->error() ) {
        kDebug() << "Job error, " << job->errorString();
        emit authFailed( i18n( "Login failed" ) );
        return;
    }

    StoredTransferJob* transJob = static_cast<StoredTransferJob*>( job );
    if ( transJob->isErrorPage() ) {
        kDebug() << "Login failed";
        emit authFailed( i18n( "Login failed" ) );
        return;
    }

    QByteArray data = transJob->data();
    kDebug() << "Received: " << data;

    QDomDocument dom;
    dom.setContent( data );
    QDomNodeList nodeList = dom.elementsByTagName( "authorized" );
    for ( int i=0; i< nodeList.count(); ++i ) {
        QDomNode node = nodeList.at( i );
        if ( node.toElement().text() == "true" ) {
            kDebug() << "Authorisation is OK";
            emit authOk();
            return;
        }
    }

    QString error;
    nodeList = dom.elementsByTagName( "error" );
    for ( int i=0; i< nodeList.count(); ++i ) {
        QDomNode node = nodeList.at( i );
        error.append( node.toElement().text() );
    }

    emit authFailed( error );
}

void Communication::retrieveFolder( const QString &folder )
{
    KUrl url = getBaseUrl();
    if ( folder ==  "home" ) {
        url.addPath( "statuses/friends_timeline.xml" );
    } else if ( folder == "replies" ) {
        url.addPath( "statuses/replies.xml" );
    } else if ( folder == "favorites" ) {
        url.addPath( "favorites.xml" );
    } else if ( folder == "inbox" ) {
        url.addPath( "direct_messages.xml" );
    } else if ( folder == "outbox" ) {
        url.addPath( "direct_messages/sent.xml" );
    }

    url.addQueryItem( "page", "1" ); // temp;

    KIO::StoredTransferJob *job = KIO::storedGet( url, Reload, HideProgressInfo ) ;
    connect( job, SIGNAL( result( KJob* ) ), this, SLOT( slotStatusListReceived( KJob* ) ) );
}

void Communication::slotStatusListReceived( KJob* job )
{
    if ( job->error() ) {
        kDebug() << "Job error, " << job->errorString();
        // TODO: EMIT SOMETHING
        return;
    }

    StoredTransferJob* transJob = static_cast<StoredTransferJob*>( job );

    QByteArray data = transJob->data();
    kDebug() << "Received: " << data;
}

QString Communication::serviceToApi( int service )
{
    if ( service == 0 )
        return QString( "http://identi.ca/api/" );

    return QString();
}

#include "communication.moc"


