/* Copyright 2009 Thomas McGuire <mcguire@kde.org>

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU Library General Public License as published
   by the Free Software Foundation; either version 2 of the License or
   ( at your option ) version 3 or, at the discretion of KDE e.V.
   ( which shall act as a proxy as in section 14 of the GPLv3 ), any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "jobs.h"
#include "settings.h"

#include <KIO/Scheduler>
#include <KIO/Slave>
#include <KDebug>
#include <KLocalizedString>

POPSession::POPSession()
{
  KIO::Scheduler::connect(
    SIGNAL( slaveError( KIO::Slave *, int, const QString & ) ), this,
    SLOT( slotSlaveError( KIO::Slave*, int, const QString & ) ) );
}

POPSession::~POPSession()
{
  closeSession();
}

void POPSession::slotSlaveError( KIO::Slave *slave , int errorCode,
                                 const QString &errorMessage )
{
  Q_UNUSED( slave );
  kWarning() << "Got a slave error:" << errorMessage;

  if ( slave != mSlave )
    return;

  if ( errorCode == KIO::ERR_SLAVE_DIED )
    mSlave = 0;

  // Explicitly disconnect the slave if the connection went down
  if ( errorCode == KIO::ERR_CONNECTION_BROKEN && mSlave ) {
    KIO::Scheduler::disconnectSlave( mSlave );
    mSlave = 0;
  }

  if ( !mCurrentJob ) {
    emit slaveError( errorCode, errorMessage );
  }
  else {
    // Let the job deal with the problem
    mCurrentJob->slaveError( errorCode, errorMessage );
  }
}

void POPSession::setCurrentJob( SlaveBaseJob *job )
{
  mCurrentJob = job;
}

KIO::MetaData POPSession::slaveConfig() const
{
  KIO::MetaData m;

  m.insert( "progress", "off" );
  m.insert( "tls", Settings::useTLS() ? "on" : "off" );
  m.insert( "pipelining", ( Settings::pipelining() ) ? "on" : "off" );
  const QString &auth = Settings::authenticationMethod();
  if ( auth == "PLAIN" || auth == "LOGIN" || auth == "CRAM-MD5" ||
       auth == "DIGEST-MD5" || auth == "NTLM" || auth == "GSSAPI" ) {
    m.insert( "auth", "SASL" );
    m.insert( "sasl", auth );
  } else if ( auth == "*" )
    m.insert( "auth", "USER" );
  else
    m.insert( "auth", auth );

  return m;
}

KUrl POPSession::getUrl() const
{
  KUrl url;

  if ( Settings::useSSL() )
    url.setProtocol( "pop3s ");
  else
    url.setProtocol( "pop3" );

  url.setUser( Settings::login() );
  url.setPass( Settings::password() );
  url.setHost( Settings::host() );
  url.setPort( Settings::port() );
  return url;
}

bool POPSession::connectSlave()
{
  mSlave = KIO::Scheduler::getConnectedSlave( getUrl(), slaveConfig() );
  return mSlave != 0;
}

void POPSession::abortCurrentJob()
{
  if ( mCurrentJob ) {
    mCurrentJob->kill( KJob::Quietly );
    mCurrentJob = 0;
  }
}

void POPSession::closeSession()
{
  if ( mSlave ) {
    KIO::Scheduler::disconnectSlave( mSlave );
  }
}

KIO::Slave* POPSession::getSlave() const
{
  return mSlave;
}





static QByteArray cleanupListRespone( const QByteArray &response )
{
  QByteArray ret = response.simplified(); // Workaround for Maillennium POP3/UNIBOX

  // Get rid of the null terminating character, if it exists
  if ( ret.size() > 0 && ret.at( ret.size() - 1 ) == 0 )
    ret.chop( 1 );
  return ret;
}

static QString intListToString( const QList<int> &intList )
{
  QString idList;
  foreach( int id, intList )
    idList += QString::number( id ) + ",";
  idList.chop( 1 );
  return idList;
}




SlaveBaseJob::SlaveBaseJob( POPSession *POPSession )
  : mJob( 0 ),
    mPOPSession( POPSession )
{
  mPOPSession->setCurrentJob( this );
}

SlaveBaseJob::~SlaveBaseJob()
{
  // Don't do that here, the job might be destroyed after another one was started
  // and therefore overwrite the current job
  //mPOPSession->setCurrentJob( 0 );
}

bool SlaveBaseJob::doKill()
{
  if ( mJob )
    return mJob->kill();
  else
    return KJob::doKill();
}

void SlaveBaseJob::slotSlaveResult( KJob *job )
{
  mPOPSession->setCurrentJob( 0 );
  if ( job->error() ) {
    setError( job->error() );
    setErrorText( job->errorText() );
  }
  emitResult();
  mJob = 0;
}

void SlaveBaseJob::slotSlaveData( KIO::Job *job, const QByteArray &data )
{
  Q_UNUSED( job );
  kWarning() << "Got unexpected slave data:" << data.data();
}

void SlaveBaseJob::slaveError( int errorCode, const QString &errorMessage )
{
  // The slave experienced some problem while running our job.
  // Just treat this as an error.
  // Derived jobs can do something more sophisticated here
  setError( errorCode );
  setErrorText( errorMessage);
  emitResult();
  mJob = 0;
}

void SlaveBaseJob::connectJob()
{
  connect( mJob, SIGNAL( data( KIO::Job*, const QByteArray & ) ),
           SLOT( slotSlaveData( KIO::Job*, const QByteArray & ) ) );
  connect( mJob, SIGNAL( result( KJob * ) ),
           SLOT( slotSlaveResult( KJob * ) ) );
}

void SlaveBaseJob::startJob( const QString &path)
{
  KUrl url = mPOPSession->getUrl();
  url.setPath( path );
  mJob = KIO::get( url, KIO::NoReload, KIO::HideProgressInfo );
  KIO::Scheduler::assignJobToSlave( mPOPSession->getSlave(), mJob );
  connectJob();
}

LoginJob::LoginJob( POPSession *popSession )
  : SlaveBaseJob( popSession )
{
}

void LoginJob::start()
{
  // This will create a connected slave, which means it will also try to login.
  KIO::Scheduler::connect( SIGNAL( slaveConnected(KIO::Slave*)),
                           this, SLOT(slaveConnected(KIO::Slave*)));
  if ( !mPOPSession->connectSlave() ) {
    setError ( KJob::UserDefinedError );
    setErrorText( i18n( "Unable to create POP3 slave, aborting mail check." ) );
    emitResult();
  }
}

void LoginJob::slaveConnected( KIO::Slave *slave )
{
  if ( slave != mPOPSession->getSlave() ) {
    // Odd, not our slave...
    return;
  }

  // Yeah it connected, so login was sucessful!
  emitResult();
}

void LoginJob::slaveError( int errorCode, const QString &errorMessage )
{
  setError( errorCode );
  setErrorText( errorMessage );
  emitResult();
}

ListJob::ListJob( POPSession *popSession )
  : SlaveBaseJob( popSession )
{
}

void ListJob::start()
{
  startJob( "/index" );
}

void ListJob::slotSlaveData( KIO::Job *job, const QByteArray &data )
{
  Q_UNUSED( job );

  // Silly slave, why are you sending us empty data?
  if ( data.isEmpty() )
    return;

  QByteArray cleanData = cleanupListRespone( data );
  const int space = cleanData.indexOf( ' ' );

  if ( space > 0 ) {
    QByteArray lengthString = cleanData.mid( space + 1 );
    const int spaceInLengthPos = lengthString.indexOf( ' ' );
    if ( spaceInLengthPos != -1 )
      lengthString.truncate( spaceInLengthPos );
    const int length = lengthString.toInt();

    QByteArray idString = cleanData.left( space );

    bool idIsNumber;
    int id = QString::fromAscii( idString ).toInt( &idIsNumber );
    if ( idIsNumber )
      mIdList.insert( id, length );
    else
      kWarning() << "Got non-integer ID as part of the LIST response, ignoring"
                 << idString.data();
  }
  else {
    kWarning() << "Got invalid LIST response:" << data.data();
  }
}

QMap<int,int> ListJob::idList() const
{
  return mIdList;
}

UIDListJob::UIDListJob( POPSession *popSession )
  : SlaveBaseJob( popSession )
{
}

void UIDListJob::start()
{
  startJob( "/uidl" );
}

void UIDListJob::slotSlaveData( KIO::Job *job, const QByteArray &data )
{
  Q_UNUSED( job );

  // Silly slave, why are you sending us empty data?
  if ( data.isEmpty() )
    return;

  QByteArray cleanData = cleanupListRespone( data );
  const int space = cleanData.indexOf( ' ' );

  if ( space <= 0 ) {
    kWarning() << "Invalid response to the UIDL command:" << data.data();
    kWarning() << "Ignoring this entry.";
  }
  else {
    QByteArray idString = cleanData.left( space );
    QByteArray uidString = cleanData.mid( space + 1 );
    bool idIsNumber;
    int id = QString::fromAscii( idString ).toInt( &idIsNumber );
    if ( idIsNumber ) {
      mUidList.insert( id, QString::fromAscii( uidString ) );
    }
    else {
      kWarning() << "Got invalid ID from the UIDL command:" << idString.data();
      kWarning() << "The whole response was:" << data.data();
    }
  }
}

QMap<int,QString> UIDListJob::uidList() const
{
  return mUidList;
}

DeleteJob::DeleteJob( POPSession *popSession )
  : SlaveBaseJob( popSession )
{
}

void DeleteJob::setDeleteIds( const QList<int> ids )
{
  mIdsToDelete = ids;
}

void DeleteJob::start()
{
  startJob( "/remove/" + intListToString( mIdsToDelete ) );
}

QList<int> DeleteJob::deletedIDs() const
{
  // FIXME : The slave doesn't tell us which of the IDs were actually deleted, we
  //         just assume all of them here
  return mIdsToDelete;
}

QuitJob::QuitJob( POPSession *popSession )
  : SlaveBaseJob( popSession )
{
}

void QuitJob::start()
{
  startJob( "/commit" );
}

FetchJob::FetchJob ( POPSession *session )
  : SlaveBaseJob( session ),
    mBytesDownloaded( 0 ),
    mTotalBytesToDownload( 0 ),
    mDataCounter( 0 )
{
}

void FetchJob::setFetchIds( const QList<int> ids, QList<int> sizes )
{
  mIdsPendingDownload = ids;
  foreach( int size, sizes )
    mTotalBytesToDownload += size;
}

void FetchJob::start()
{
  startJob( "/download/" + intListToString( mIdsPendingDownload ) );
  setTotalAmount( KJob::Bytes, mTotalBytesToDownload );
}

void FetchJob::connectJob()
{
  SlaveBaseJob::connectJob();
  connect( mJob, SIGNAL(infoMessage( KJob*, const QString &, const QString & ) ),
           SLOT( slotInfoMessage( KJob*, const QString &, const QString & ) ) );
}

void FetchJob::slotSlaveData( KIO::Job *job, const QByteArray &data )
{
  Q_UNUSED( job );
  mCurrentMessage += data;
  mBytesDownloaded += data.size();
  mDataCounter++;
  if ( mDataCounter % 5 == 0 ) {
    setProcessedAmount( KJob::Bytes, mBytesDownloaded );
  }
}

void FetchJob::slotInfoMessage( KJob *job, const QString &infoMessage, const QString & )
{
  Q_UNUSED( job );
  if ( infoMessage != "message complete" )
    return;

  KMime::Message::Ptr msg( new KMime::Message );
  msg->setContent( KMime::CRLFtoLF( mCurrentMessage ) );
  msg->parse();

  mCurrentMessage.clear();
  const int idOfCurrentMessage = mIdsPendingDownload.takeFirst();
  emit messageFinished( idOfCurrentMessage, msg );
}


