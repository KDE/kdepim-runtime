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

FakeServerThread::FakeServerThread( QObject *parent )
  : QThread( parent ),
    mServer( 0 )
{
}

void FakeServerThread::run()
{
  mServer = new FakeServer();

  // Run forever, until someone from the outside calls quit() on us and quits the
  // event loop
  exec();

  delete mServer;
  mServer = 0;
}

FakeServer *FakeServerThread::server() const
{
  Q_ASSERT( mServer != 0 );
  return mServer;
}

FakeServer::FakeServer( QObject* parent )
    : QObject( parent ),
      mConnections( 0 ),
      mProgress( 0 ),
      mGotDisconnected( false )
{
  mTcpServer = new QTcpServer();
  if ( !mTcpServer->listen( QHostAddress( QHostAddress::LocalHost ), 5989 ) ) {
    kFatal() << "Unable to start the server";
  }

  connect( mTcpServer, SIGNAL(newConnection()),
           this, SLOT(newConnection()) );
}

FakeServer::~FakeServer()
{
  if ( mConnections > 0 )
    disconnect( mTcpServerConnection, SIGNAL(readyRead()),
                this, SLOT(dataAvailable()) );

  delete mTcpServer;
  mTcpServer = 0;
}

QByteArray FakeServer::parseDeleteMark( const QByteArray &expectedData,
                                        const QByteArray &dataReceived )
{
  // Only called from parseResponse(), which is already thread-safe

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
    qWarning() << "Received:" << dataReceived.data()
               << "\nExpected:" << expectedData.data();
    Q_ASSERT_X( false, "FakeServer::parseDeleteMark", "Unable to substitute data!" );
    return QByteArray();
  }
  else return expectedData;
}

QByteArray FakeServer::parseRetrMark( const QByteArray &expectedData,
                                      const QByteArray &dataReceived )
{
  // Only called from parseResponse(), which is already thread-safe

  const QByteArray retrMark = QString( "%RETR%" ).toUtf8();
  if ( expectedData.contains( retrMark ) ) {
    Q_ASSERT( !mAllowedRetrieves.isEmpty() );
    for ( int i = 0; i < mAllowedRetrieves.size(); i++ ) {
      QByteArray substituted = expectedData;
      substituted = substituted.replace( retrMark, mAllowedRetrieves[i] );
      if (  substituted == dataReceived ) {
        mAllowedRetrieves.removeAt( i );
        return substituted;
      }
    }
    qWarning() << "Received:" << dataReceived.data()
               << "\nExpected:" << expectedData.data();
    Q_ASSERT_X( false, "FakeServer::parseRetrMark", "Unable to substitute data!" );
    return QByteArray();
  }
  else return expectedData;
}

QByteArray FakeServer::parseResponse( const QByteArray& expectedData,
                                      const QByteArray& dataReceived )
{
  // Only called from dataAvailable, which is already thread-safe

  QByteArray result;
  result = parseDeleteMark( expectedData, dataReceived );
  if ( result != expectedData )
    return result;
  else return parseRetrMark( expectedData,dataReceived );
}

/*
// Used only for the debug output in dataAvailable()
static QByteArray removeCRLF( const QByteArray &ba )
{
  QByteArray returnArray = ba;
  returnArray.replace( "\r\n", QByteArray() );
  return returnArray;
}
*/

void FakeServer::dataAvailable()
{
  QMutexLocker locker( &mMutex );
  mProgress++;

  // We got data, so we better expect it and have an answer!
  Q_ASSERT( !mReadData.isEmpty() );
  Q_ASSERT( !mWriteData.isEmpty() );

  const QByteArray data = mTcpServerConnection->readAll();
  //qDebug() << "Got data:" << removeCRLF( data );
  const QByteArray expected( mReadData.takeFirst() );
  //qDebug() << "Expected data:" << removeCRLF( expected );
  const QByteArray reallyExpected = parseResponse( expected, data );
  //qDebug() << "Really expected:" << removeCRLF( reallyExpected );

  Q_ASSERT( data == reallyExpected );

  QByteArray toWrite = mWriteData.takeFirst();
  //qDebug() << "Going to write data:" << removeCRLF( toWrite );
  const bool allWritten = mTcpServerConnection->write( toWrite ) == toWrite.size();
  Q_ASSERT( allWritten );
  const bool flushed = mTcpServerConnection->flush();
  Q_ASSERT( flushed );
}

void FakeServer::newConnection()
{
  QMutexLocker locker( &mMutex );
  Q_ASSERT( mConnections == 0 );
  mConnections++;
  mGotDisconnected = false;

  mTcpServerConnection = mTcpServer->nextPendingConnection();
  mTcpServerConnection->write( QByteArray( "+OK Initech POP3 server ready.\r\n" ) );
  connect( mTcpServerConnection, SIGNAL(readyRead()), this, SLOT(dataAvailable()) );
  connect( mTcpServerConnection, SIGNAL(disconnected()), this, SLOT(slotDisconnected()) );
}

void FakeServer::slotDisconnected()
{
  QMutexLocker locker( &mMutex );
  mConnections--;
  mGotDisconnected = true;
  Q_ASSERT( mConnections == 0 );
  Q_ASSERT( mAllowedDeletions.isEmpty() );
  Q_ASSERT( mAllowedRetrieves.isEmpty() );
  Q_ASSERT( mReadData.isEmpty() );
  Q_ASSERT( mWriteData.isEmpty() );
  emit disconnected();
}

void FakeServer::setAllowedDeletions( const QString &deleteIds )
{
  QMutexLocker locker( &mMutex );
  mAllowedDeletions.clear();
  QStringList ids = deleteIds.split( ',', QString::SkipEmptyParts );
  foreach( const QString &id, ids ) {
    mAllowedDeletions.append( id.toUtf8() );
  }
}

void FakeServer::setAllowedRetrieves( const QString &retrieveIds )
{
  QMutexLocker locker( &mMutex );
  mAllowedRetrieves.clear();
  QStringList ids = retrieveIds.split( ',', QString::SkipEmptyParts );
  foreach( const QString &id, ids ) {
    mAllowedRetrieves.append( id.toUtf8() );
  }
}

void FakeServer::setMails( const QList<QByteArray> &mails)
{
  QMutexLocker locker( &mMutex );
  mMails = mails;
}

void FakeServer::setNextConversation( const QString& conversation,
                                      const QList<int> &exceptions )
{
  QMutexLocker locker( &mMutex );

  Q_ASSERT( mReadData.isEmpty() );
  Q_ASSERT( mWriteData.isEmpty() );
  Q_ASSERT( !conversation.isEmpty() );

  mGotDisconnected = false;
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
      lineData.replace( mailSizeMarker,
                        QString::number( mMails[sizeIndex++].size() ).toAscii() );
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

int FakeServer::progress() const
{
  QMutexLocker locker( &mMutex );
  return mProgress;
}

bool FakeServer::gotDisconnected() const
{
  QMutexLocker locker( &mMutex );
  return mGotDisconnected;
}

#include "fakeserver.moc"
