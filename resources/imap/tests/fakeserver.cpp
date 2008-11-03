/*
   Copyright (C) 2008 Omat Holding B.V. <info@omat.nl>

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

// Own
#include "fakeserver.h"

// Qt
#include <QDebug>
#include <QTcpServer>

// KDE
#include <KDebug>

FakeServer::FakeServer( QObject* parent ) : QThread( parent )
{
}

void FakeServer::run()
{
    QTcpServer tcpServer;
    if ( !tcpServer.listen( QHostAddress( QHostAddress::LocalHost ), 5989 ) ) {
        kFatal() << "Unable to start the server";
    }

    if ( tcpServer.waitForNewConnection( -1 ) ) {

        QTcpSocket* tcpServerConnection = tcpServer.nextPendingConnection();
        tcpServerConnection->write( QByteArray( "* OK localhost Test Library server ready\r\n" ) );
        tcpServerConnection->waitForBytesWritten( -1 );

        while ( tcpServerConnection->waitForReadyRead( -1 ) ) {

            QByteArray data = tcpServerConnection->readAll();
            kDebug() << "R: " << data.trimmed();

            Q_ASSERT( !m_data.isEmpty() );

            QByteArray toWrite = QString( m_data.takeFirst() + "\r\n" ).toLatin1();
            kDebug() << "S: " << toWrite.trimmed();

            tcpServerConnection->write( toWrite );
            tcpServerConnection->waitForBytesWritten( -1 );
        }

    }
}

void FakeServer::setResponse( const QStringList& data )
{
    m_data = data;
}

#include "fakeserver.moc"
