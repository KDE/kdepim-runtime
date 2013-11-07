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

#include <QBuffer>
#include <QDomDocument>
#include <QObject>

#include <kio/job.h>
using namespace KIO;

Communication::Communication( QObject* parent )
        : QObject( parent ), m_service(-1)
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
    url.addPath( QLatin1String("account/verify_credentials.xml") );
    KIO::StoredTransferJob *job = KIO::storedGet( url, Reload, HideProgressInfo ) ;
    connect( job, SIGNAL(result(KJob*)), this, SLOT(slotCheckAuthData(KJob*)) );
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
    QDomNodeList nodeList = dom.elementsByTagName( QLatin1String("user") );
    if ( nodeList.count() > 0 ) {
        kDebug() << "Authorization is OK";
        emit authOk();
        return;
    }

    emit authFailed( i18n( "Authentication error" ) );
}

void Communication::retrieveFolder( const QString &folder, qlonglong since )
{
    m_retrievingFolder = folder;

    KUrl url = getBaseUrl();
    if ( folder ==  QLatin1String("home") ) {
        url.addPath( QLatin1String("statuses/friends_timeline.xml") );
    } else if ( folder == QLatin1String("replies") ) {
        url.addPath( QLatin1String("statuses/replies.xml") );
    } else if ( folder == QLatin1String("favorites") ) {
        url.addPath( QLatin1String("favorites.xml") );
    } else if ( folder == QLatin1String("inbox") ) {
        url.addPath( QLatin1String("direct_messages.xml") );
    } else if ( folder == QLatin1String("outbox") ) {
        url.addPath( QLatin1String("direct_messages/sent.xml") );
    }

    if ( since == 0 )
        url.addQueryItem( QLatin1String("page"), QLatin1String("1") );
    else
        url.addQueryItem( QLatin1String("since_id"), QString::number( since ) );

    kDebug() << url;

    KIO::StoredTransferJob *job = KIO::storedGet( url, Reload, HideProgressInfo ) ;
    connect( job, SIGNAL(result(KJob*)), this, SLOT(slotStatusListReceived(KJob*)) );
}

void Communication::slotStatusListReceived( KJob* job )
{
    QList<QByteArray> list;

    if ( job->error() ) {
        kDebug() << "Job error, " << job->errorString();
        emit statusList( list );
        return;
    }

    StoredTransferJob* transJob = static_cast<StoredTransferJob*>( job );
    QByteArray data = transJob->data();

    // Parse the data.
    QDomDocument document;
    document.setContent( data );

    QLatin1String main( "statuses" );
    QLatin1String sub( "status" );
    if ( m_retrievingFolder == QLatin1String("inbox") || m_retrievingFolder ==QLatin1String( "outbox") ) {
        main = QLatin1String( "direct-messages" );
        sub = QLatin1String( "direct_message" );
    }

    QDomElement root = document.documentElement();
    if ( root.tagName() != main ) {
        kDebug() << "** there's no " << main << "tag in XML\t the XML is: \n" << data.data();
        emit statusList( list );
        return;
    }

    QDomNode node = root.firstChild();
    while ( !node.isNull() ) {
        if ( node.toElement().tagName() != sub ) {
            kDebug() << "** there's no " << sub << "tag in XML, maybe there is no new status!" << data.data();
            emit statusList( list );
            return;
        }

        QByteArray g;
        QTextStream out( &g );
        node.save( out, 0 );
        list << g;

        node = node.nextSibling();
    }
    emit statusList( list );
}

QString Communication::serviceToApi( int service )
{
    if ( service == 0 )
        return QLatin1String( "http://identi.ca/api/" );
    else if ( service == 1 )
        return QLatin1String( "http://twitter.com/" );

    return QString();
}



