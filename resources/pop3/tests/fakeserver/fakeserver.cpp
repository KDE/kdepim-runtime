/*
   Copyright (C) 2008 Omat Holding B.V. <info@omat.nl>
   Copyright (C) 2009 Thomas McGuire <mcguire@kde.org>

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
#include <QtTest>

// KDE
#include <KDebug>

FakeServer::FakeServer( QObject* parent )
    : QThread( parent ),
      mConnections( 0 )
{
}


FakeServer::~FakeServer()
{
  quit();
  wait();
}

QByteArray FakeServer::parseResponse( const QByteArray& expectedData, const QByteArray& dataReceived )
{
  const QByteArray deleteMark = QString( "%DELE%" ).toUtf8();
  if ( expectedData.contains( deleteMark ) ) {
    Q_ASSERT( !mAllowedDeletions.isEmpty() );
    for ( int i = 0; i < mAllowedDeletions.size(); i++ ) {
      QByteArray substituted = expectedData;
      substituted = substituted.replace( deleteMark, mAllowedDeletions[i] );
      if (  substituted == dataReceived ) {
        mAllowedDeletions.removeAt( i );
        return substituted;
      }
    }
    Q_ASSERT_X( false, "FakeServer::parseResponse", "Unable to substitute data!" );
    return QByteArray();
  }
  else return expectedData;
}

static QByteArray removeCRLF( const QByteArray &ba )
{
  QByteArray returnArray = ba;
  returnArray.replace( "\r\n", QByteArray() );
  return returnArray;
}

void FakeServer::dataAvailable()
{
  QMutexLocker locker( &mMutex );

  emit progress();

  // We got data, so we better expect it and have an answer!
  Q_ASSERT( !mReadData.isEmpty() );
  Q_ASSERT( !mWriteData.isEmpty() );

  const QByteArray data = mTcpServerConnection->readAll();
  qDebug() << "Got data:" << removeCRLF( data );
  const QByteArray expected( mReadData.takeFirst() );
  qDebug() << "Expected data:" << removeCRLF( expected );
  const QByteArray reallyExpected = parseResponse( expected, data );
  qDebug() << "Really expected:" << removeCRLF( reallyExpected );

  Q_ASSERT( data == reallyExpected );

  QByteArray toWrite = mWriteData.takeFirst();
  qDebug() << "Going to write data:" << removeCRLF( toWrite );
  Q_ASSERT( mTcpServerConnection->write( toWrite ) == toWrite.size() );
  Q_ASSERT( mTcpServerConnection->flush() );
}

void FakeServer::newConnection()
{
  QMutexLocker locker( &mMutex );

  Q_ASSERT( mConnections == 0 );
  mConnections++;

  mTcpServerConnection = mTcpServer->nextPendingConnection();
  mTcpServerConnection->write( QByteArray( "+OK Initech POP3 server ready.\r\n" ) );
  connect( mTcpServerConnection, SIGNAL(readyRead()), this, SLOT(dataAvailable()) );
  connect( mTcpServerConnection, SIGNAL(disconnected()), this, SLOT(slotDisconnected()) );
}

void FakeServer::slotDisconnected()
{
  qDebug() << "FakeServer got disconnected.";
  QMutexLocker locker( &mMutex );
  mConnections--;
  Q_ASSERT( mConnections == 0 );
  Q_ASSERT( mAllowedDeletions.isEmpty() );
  Q_ASSERT( mReadData.isEmpty() );
  Q_ASSERT( mWriteData.isEmpty() );
  emit disconnected();
}

void FakeServer::run()
{
  mTcpServer = new QTcpServer();
  if ( !mTcpServer->listen( QHostAddress( QHostAddress::LocalHost ), 5989 ) ) {
    kFatal() << "Unable to start the server";
  }

  connect( mTcpServer, SIGNAL(newConnection()), this, SLOT(newConnection()) );

  exec();

  if ( mConnections > 0 )
    disconnect( mTcpServerConnection, SIGNAL(readyRead()), this, SLOT(dataAvailable()) );

  delete mTcpServer;
  mTcpServer = 0;
}

void FakeServer::setAllowedDeletions( const QString &deleteIds )
{
  QStringList ids = deleteIds.split( ',' );
  foreach( const QString &id, ids ) {
    mAllowedDeletions.append( id.toUtf8() );
  }
}

void FakeServer::setMails( const QList<QByteArray> &mails)
{
  mMails = mails;
}

void FakeServer::setNextConversation( const QString& conversation,
                                      const QList<int> &exceptions )
{
  QMutexLocker locker( &mMutex );

  Q_ASSERT( mReadData.isEmpty() );
  Q_ASSERT( mWriteData.isEmpty() );
  Q_ASSERT( !conversation.isEmpty() );

  QStringList lines = conversation.split( "\r\n", QString::SkipEmptyParts );
  Q_ASSERT( lines.first().startsWith( "C:" ) );

  enum Mode { Client, Server };
  Mode mode = Client;

  const QByteArray mailSizeMarker = QString( "%MAILSIZE%" ).toAscii();
  const QByteArray mailMarker = QString( "%MAIL%" ).toAscii();
  int sizeIndex = 0;
  int mailIndex = 0;

  foreach( const QString &line, lines ) {

    QByteArray lineData( line.toUtf8() );

    if ( lineData.contains( mailSizeMarker ) ) {
      Q_ASSERT( mMails.size() > sizeIndex );
      lineData.replace( mailSizeMarker, QString( mMails[sizeIndex++].size() ).toAscii() );
    }
    if ( lineData.contains( mailMarker ) ) {
      while( exceptions.contains( mailIndex + 1 ) )
        mailIndex++;
      Q_ASSERT( mMails.size() > mailIndex );
      lineData.replace( mailMarker, mMails[mailIndex++] );
    }

    if ( lineData.startsWith( "S: " ) ) {
      mWriteData.append( lineData.mid( 3 ) + "\r\n" );
      mode = Server;
    }
    else if ( line.startsWith("C: " ) ) {
      mReadData.append( lineData.mid( 3 ) + "\r\n" );
      mode = Client;
    }
    else {
      switch ( mode ) {
        case Server: mWriteData.last() += ( lineData + "\r\n" ); break;
        case Client: mReadData.last() += ( lineData + "\r\n" ); break;
      }
    }
  }
}

#include "fakeserver.moc"
