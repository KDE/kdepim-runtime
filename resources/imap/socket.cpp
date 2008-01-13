/* This file is part of the KDE project
   Copyright (C) 2006-2007 KovoKs <info@kovoks.nl>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

// Uncomment the next line for full comm debug
//#define comm_debug

// Own
#include "socket.h"
#include "db.h"

// Qt
#include <QRegExp>
#include <QByteArray>
#include <QSslSocket>
#include <QDateTime>

// KDE
#include <KDebug>
#include <KLocale>
#include <KMessageBox>
#include <ksocketfactory.h>

Socket::Socket( QObject* parent )
        :QObject( parent ), m_socket( 0 ), m_port( 0 ), m_aboutToClose( false ),
        m_interactive( true ), m_secure( false ), m_tls( false )
{
    kDebug( 50002 ) << endl;
}

Socket::~Socket()
{
    kDebug( 50002 ) << objectName() << " " << endl;
    m_aboutToClose=true;
}

void Socket::reconnect()
{
    kDebug( 50002 ) << endl;
    if ( m_aboutToClose )
        return;

    kDebug( 50002 ) << objectName() << " " << "Connecting to: " << m_server << ":"
    <<  m_port << endl;

#ifdef comm_debug
    // kDebug(50002) << objectName() << m_proto << endl;
#endif

    if ( m_socket )
        return;

    m_socket = static_cast<QSslSocket*>
               ( KSocketFactory::connectToHost( m_proto, m_server, m_port, this ) );

    m_socket->setProtocol( QSsl::AnyProtocol );

    connect( m_socket, SIGNAL( stateChanged( QAbstractSocket::SocketState ) ),
             SLOT( slotStateChanged( QAbstractSocket::SocketState ) ) );
    connect( m_socket, SIGNAL( modeChanged( QSslSocket::SslMode ) ),
             SLOT( slotModeChanged( QSslSocket::SslMode ) ) );
    connect( m_socket, SIGNAL( connected() ), SLOT( slotConnected() ) );
    connect( m_socket, SIGNAL( disconnected() ), SLOT( slotDisconnected() ) );
    connect( m_socket, SIGNAL( readyRead() ), SLOT( slotSocketRead() ) );
    connect( m_socket, SIGNAL( encrypted() ), SIGNAL( connected() ) );
    connect( m_socket, SIGNAL( sslErrors( const QList<QSslError> & ) ),
             SLOT( slotSslErrors( const QList<QSslError>& ) ) );
}

void Socket::slotConnected()
{
    kDebug( 50002 ) << endl;

    if ( !m_secure || m_tls ) {
        kDebug( 50002 ) << "normal connect" << endl;
        emit connected();
    } else {
        kDebug( 50002 ) << "encrypted connect" << endl;
        startShake();
    }
}

void Socket::slotStateChanged( QAbstractSocket::SocketState state )
{
#ifdef comm_debug
    kDebug( 50002 ) << objectName() << " "
    << "State is now: " << ( int )state << endl;
#else
    Q_UNUSED( state );
#endif
}

void Socket::slotModeChanged( QSslSocket::SslMode  state )
{
#ifdef comm_debug
    kDebug( 50002 ) << objectName() << " "
    << "Mode is now: " << state << endl;
#else
    Q_UNUSED( state );
#endif

}
void Socket::slotSocketRead()
{
    if ( !m_socket )
        return;

    static QString msg;
    msg += QString( m_socket->readAll() );

    if ( !msg.endsWith( '\n' ) )
        return;

#ifdef comm_debug
    kDebug( 50002 ) << objectName() << " " << m_socket->isEncrypted()
    << msg.trimmed() << endl;
#endif

    if ( !m_socket->isEncrypted() ) {
        // The bits received are not crypted. This can happen when
        // we don't want tls or ssl or when the connection is ready for tls.

        if ( m_tls ) {
            // TODO move intelligence to imap/smtp
            if ( msg.indexOf( "a02 OK" ) != -1 ||
                    msg.indexOf( "220 " ) < msg.indexOf( "TLS" ) && msg.indexOf( "220 " ) != -1 ) {
                // Request accepted.
                kDebug( 50002 ) << objectName() << " "
                << "Accepted, starting TLS handshake..."
                << m_socket->mode() << endl;
                startShake();
            } else if ( msg.indexOf( "a02 " ) != -1 ) {
                // request returned, but not ok.
                KMessageBox::information( 0,i18n( "TLS was refused:\n\n" )+msg );
            } else {
                // request tls.
                kDebug( 50002 ) << objectName() << " "
                << "Connected, deal with TLS please"
                << endl;

                emit data( msg );
                msg.clear();
            }
        } else {
            emit data( msg );
            msg.clear();
        }
    } else {
        emit data( msg );
        msg.clear();
    }
}

void Socket::startShake()
{
    kDebug( 50002 ) << objectName() << " " << endl;
    m_socket->startClientEncryption();
}

void Socket::slotSslErrors( const QList<QSslError> & errors )
{
    kDebug( 50002 ) << objectName() << " " << "\n"
    << QString( m_socket->peerCertificate().toPem() ) << "\n"  << endl;

    if ( !m_interactive ) {
        emit connected();
        return;
    }

    QString errorList;
    QString key;
    for ( int i = 0 ; i < errors.count() ; ++i ) {
        // duplicates were here.
        if ( !errorList.contains( errors.at( i ).errorString() ) ) {
            errorList.append( "- " + errors.at( i ).errorString() + "<br>" );
            key.append( '|' + QString::number( errors.at( i ).error() ) + '|' );
        }
    }

    m_aboutToClose = true;
    DB* db = new DB;
    if ( !db->hasCert( m_socket->peerCertificate().toPem(), key ) ) {
        int i = KMessageBox::warningYesNoCancel( 0,
                i18n( "<Qt>There were problems connecting to the server:"
                      "<br><br><b>%1</b><br><Br>"
                      "The certificate:<br>"
                      "Hostname: %2<br>"
                      "Peer certificate issuer: %3<br>"
                      "Expiry date: %4<br></Qt>", errorList, m_socket->peerName(),
                      m_socket->peerCertificate().issuerInfo
                      ( QSslCertificate::Organization ),
                      m_socket->peerCertificate().expiryDate().toString() ),
                i18n( "Errors Connecting to Server" ),
                KGuiItem( i18n( "Ignore these errors permanently" ) ),
                KGuiItem( i18n( "Ignore these errors this time" ) ),
                KGuiItem( i18n( "Cancel connection" ) ) );

        if ( i == KMessageBox::Yes ) {
            db->addCert( m_socket->peerCertificate().toPem(), key );
        }

        if ( i == KMessageBox::Yes || i == KMessageBox::No )
            acceptSslErrors();
        else {
            m_aboutToClose = true;
            emit disconnected();
        }
    } else {
        kDebug( 50002 ) << objectName() << " "
        << "Certificate invalid  but that is known and accepted: \n"
        << errorList.trimmed() << endl;
        acceptSslErrors();
    }
    delete db;
}

void Socket::acceptSslErrors()
{
    m_socket->ignoreSslErrors();
}

void Socket::write( const QString& text )
{
    // kDebug(50002) << objectName() << " " << endl;
    // Eat things in the queue when there is no connection. We need
    // to get a connection first don't we...
    if ( !m_socket || !available() )
        return;

    // \r\n needed for at least:
    //Microsoft Exchange Server 2003 IMAP4rev1 server version 6.5.7638.1
    QByteArray cs = ( text+"\r\n" ).toLatin1();

#ifdef comm_debug
    kDebug( 50002 ) << objectName() << " " << "C   : "
    << QString( cs ).trimmed() << endl;
#endif

    m_socket->write( cs.data(), cs.size() );
}

bool Socket::available()
{
    // kDebug(50002) << objectName() << " " << endl;
    bool ok = m_socket && m_socket->state() == QAbstractSocket::ConnectedState;
    return ok;
}

void Socket::slotDisconnected()
{
    // kDebug(50002) << objectName() << " " << endl;
    if ( m_aboutToClose )
        return;

    emit unexpectedDisconnect();
}

#include "socket.moc"
