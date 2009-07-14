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
#include "pop3resource.h"
#include "accountdialog.h"

#include "settings.h"
#include "settingsadaptor.h"

#include <Akonadi/ItemCreateJob>
#include <kmime/kmime_util.h>

#include <KLocalizedString>
#include <KIO/Scheduler>
#include <KIO/Slave>
#include <KIO/SimpleJob>
#include <KIO/PasswordDialog>
#include <KJobUiDelegate>
#include <KStandardDirs>
#include <KMessageBox>

#include <QtDBus/QDBusConnection>

#include <algorithm>
#include <kmime/kmime_message.h>

using namespace Akonadi;

// List of prime numbers shamelessly stolen from GCC STL
enum { num_primes = 29 };

static const unsigned short int pop3DefaultPort = 110;

#define POP_PROTOCOL "pop3"
#define POP_SSL_PROTOCOL "pop3s"

static const unsigned long prime_list[ num_primes ] =
{
  31ul,        53ul,         97ul,         193ul,       389ul,
  769ul,       1543ul,       3079ul,       6151ul,      12289ul,
  24593ul,     49157ul,      98317ul,      196613ul,    393241ul,
  786433ul,    1572869ul,    3145739ul,    6291469ul,   12582917ul,
  25165843ul,  50331653ul,   100663319ul,  201326611ul, 402653189ul,
  805306457ul, 1610612741ul, 3221225473ul, 4294967291ul
};

static inline unsigned long nextPrime( unsigned long n )
{
  const unsigned long *first = prime_list;
  const unsigned long *last = prime_list + num_primes;
  const unsigned long *pos = std::lower_bound( first, last, n );
  return pos == last ? *( last - 1 ) : *pos;
}

POP3Resource::POP3Resource( const QString &id )
  : ResourceBase( id ),
    mJob( 0 ),
    mSlave( 0 ),
    mStage( Idle ),
    mAskAgain( false ),
    indexOfCurrentMsg( -1 ),
    processingDelay( 2 * 100 ),
    interactive( true ),
    curMsgStrm( 0 ),
    dataCounter( 0 )
{
  new SettingsAdaptor( Settings::self() );

  QDBusConnection::sessionBus().registerObject( QLatin1String( "/Settings" ),
                            Settings::self(), QDBusConnection::ExportAdaptors );

  connect( &processMsgsTimer, SIGNAL( timeout() ),
           SLOT( slotProcessPendingMsgs() ) );
  KIO::Scheduler::connect(
    SIGNAL(slaveError(KIO::Slave *, int, const QString &)),
    this, SLOT(slotSlaveError(KIO::Slave *, int, const QString &)) );
}

POP3Resource::~POP3Resource()
{
  if ( mJob ) {
    kDebug() << "Killing jobs and saving UID list.";
    mJob->kill();
    mMsgsPendingDownload.clear();
    createJobsMap.clear();
    saveUidList();
  }
}

void POP3Resource::retrieveCollections()
{ 
  kDebug() << "Starting mail check...";
  emit status( Running, i18n( "Starting mail check..." ) );

  if ( mStage == Idle ) {

    if ( !Collection( Settings::targetCollection() ).isValid() ) {
      kWarning() << "We have no target collection!";
      cancelTask( i18n("No target folder set, aborting mail check.") );
      return;
    }

    // SHow the user/password dialog if needed 
    if ( (mAskAgain || Settings::password().isEmpty() || Settings::login().isEmpty()) &&
         Settings::authenticationMethod() != "GSSAPI" ) {
      //QString passwd = NetworkAccount::passwd();
      QString passwd = Settings::password();
      QString login = Settings::login();
      bool b = Settings::storePassword();
      if ( KIO::PasswordDialog::getNameAndPassword( login, passwd, &b,
                         i18n("You need to supply a username and a password to access this "
                              "mailbox."),
                         false, QString(), name(), i18n("Account:") )
        != KDialog::Accepted)
      {
        cancelTask( i18n( "No username and password supplied." ) );
        return;
      } else {
        Settings::setPassword( passwd );
        Settings::setLogin( login );
        Settings::setStorePassword( b );
        if ( b ) {
          Settings::self()->writeConfig();
        }
        mAskAgain = false;
      }
    }

    QString seenUidList;
    QStringList uidsOfSeenMsgs = Settings::seenUidList();
    mUidsOfSeenMsgsDict.clear();
    mUidsOfSeenMsgsDict.reserve( nextPrime( ( uidsOfSeenMsgs.count() * 11 ) / 10 ) );
    for ( int i = 0; i < uidsOfSeenMsgs.count(); ++i ) {
      // we use mUidsOfSeenMsgsDict to just provide fast random access to the
      // keys, so we can store the index that corresponds to the index of
      // mTimeOfSeenMsgsVector for use in PopAccount::slotData()
      mUidsOfSeenMsgsDict.insert( uidsOfSeenMsgs[i].toLatin1(), i );
    }
    QList<int> timeOfSeenMsgs = Settings::seenUidTimeList();
    // If the counts differ then the config file has presumably been tampered
    // with and so to avoid possible unwanted message deletion we'll treat
    // them all as newly seen by clearing the seen times vector
    if ( timeOfSeenMsgs.count() == mUidsOfSeenMsgsDict.count() )
      mTimeOfSeenMsgsVector = timeOfSeenMsgs.toVector();
    else
      mTimeOfSeenMsgsVector.clear();
    QStringList downloadLater = Settings::downloadLater();
    for ( int i = 0; i < downloadLater.count(); ++i ) {
      mHeaderLaterUids.insert( downloadLater[i].toLatin1() );
    }
    mUidsOfNextSeenMsgsDict.clear();
    mTimeOfNextSeenMsgsMap.clear();
    mSizeOfNextSeenMsgsDict.clear();

    //FIXME interactive = _interactive;
    interactive = false;
    mUidlFinished = false;
    startJob();
  }
  else {
    cancelTask( i18n( "Mail check already in progress, unable to start a second check." ) );
    return;
  }
}

void POP3Resource::startJob()
{
 // Run the precommand
  /*if ( !runPrecommand(precommand() ) ) {
    checkDone( false, CheckError );
    return;
    //FIXME!
  }*/
  // end precommand code

  KUrl url = getUrl();

  if ( !url.isValid() ) {
    KMessageBox::error(0, i18n("Source URL is malformed"),
                          i18n("Kioslave Error Message") );
    return;
  }

  msgsAwaitingProcessing.clear();
  msgIdsAwaitingProcessing.clear();
  msgUidsAwaitingProcessing.clear();
  mMsgsPendingDownload.clear();
  idsOfMsgs.clear();
  mUidForIdMap.clear();
  idsOfMsgsToDelete.clear();
  idsOfForcedDeletes.clear();
  createJobsMap.clear();
  //delete any headers if there are some this have to be done because of check again
  /*qDeleteAll( mHeadersOnServer );
  mHeadersOnServer.clear();
  headers = false;*/ // FIXME: headers!!
  indexOfCurrentMsg = -1;

  /*Q_ASSERT( !mMailCheckProgressItem );
  mMailCheckProgressItem = KPIM::ProgressManager::createProgressItem(
    "MailCheck" + mName,
    mName,
    i18n("Preparing transmission from \"%1\"...", mName),
    true, // can be canceled
    useSSL() || useTLS() );
  connect( mMailCheckProgressItem, SIGNAL( progressItemCanceled( KPIM::ProgressItem* ) ),
           this, SLOT( slotAbortRequested() ) );*/
  // FIXME
  emit status( Running, i18n( "Preparing transmission from \"%1\"...", name() ) );

  numBytes = 0;
  numBytesRead = 0;
  mStage = List;
  mSlave = KIO::Scheduler::getConnectedSlave( url, slaveConfig() );
  if (!mSlave)
  {
    slotSlaveError(0, KIO::ERR_CANNOT_LAUNCH_PROCESS, url.protocol());
    return;
  }
  url.setPath( "/index" );
  mJob = KIO::get( url, KIO::NoReload, KIO::HideProgressInfo );
  connectJob();
}

void POP3Resource::retrieveItems( const Akonadi::Collection &collection )
{
  Q_UNUSED( collection );
}

bool POP3Resource::retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts )
{
  Q_UNUSED( item );
  Q_UNUSED( parts );
  return true;
}

void POP3Resource::aboutToQuit()
{
  // TODO: any cleanup you need to do while there is still an active
  // event loop. The resource will terminate after this method returns
}

void POP3Resource::configure( WId windowId )
{
  Q_UNUSED( windowId );

  AccountDialog accountDialog;
  accountDialog.exec();

  synchronize();
}

void POP3Resource::itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection )
{
  Q_UNUSED( item );
  Q_UNUSED( collection );
}

void POP3Resource::itemChanged( const Akonadi::Item &item, const QSet<QByteArray> &parts )
{
  Q_UNUSED( item );
  Q_UNUSED( parts );
}

void POP3Resource::itemRemoved( const Akonadi::Item &item )
{
  Q_UNUSED( item );
}

const KUrl POP3Resource::getUrl() const
{
  KUrl url;
  url.setProtocol( protocol() );
  url.setUser( Settings::login() );
  url.setPass( Settings::password() );
  url.setHost( Settings::host() );
  url.setPort( Settings::port() );
  return url;
}

KIO::MetaData POP3Resource::slaveConfig() const
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

void POP3Resource::connectJob()
{
  KIO::Scheduler::assignJobToSlave( mSlave, mJob );
  connect( mJob, SIGNAL( data( KIO::Job*, const QByteArray &)),
           SLOT( slotData( KIO::Job*, const QByteArray &)));
  connect( mJob, SIGNAL( result( KJob * ) ),
           SLOT( slotResult( KJob * ) ) );
  connect( mJob, SIGNAL(infoMessage( KJob*, const QString &, const QString & )),
           SLOT( slotMsgRetrieved(KJob*, const QString &, const QString &)));
}

void POP3Resource::slotSlaveError( KIO::Slave* slave, int error,
                                   const QString &errorMsg )
{
  Q_UNUSED( slave );
  Q_UNUSED( error );
  kWarning() << "Got a slave error:" << errorMsg;
  emit status( Broken, errorMsg ); // FIXME, see below

  if ( slave != mSlave )
    return;
  if (error == KIO::ERR_SLAVE_DIED) mSlave = 0;

  // explicitly disconnect the slave if the connection went down
  if ( error == KIO::ERR_CONNECTION_BROKEN && mSlave ) {
    KIO::Scheduler::disconnectSlave( mSlave );
    mSlave = 0;
  }

 /* if (interactive && kmkernel) {
    KMessageBox::error(kmkernel->mainWin(), KIO::buildErrorString(error, errorMsg));
  }*/ // FIXME: use status!!


  mStage = Quit;
  if (error == KIO::ERR_COULD_NOT_LOGIN && !Settings::storePassword())
    mAskAgain = true;
  /* We need a timer, otherwise slotSlaveError of the next account is also
     executed, if it reuses the slave, because the slave member variable
     is changed too early */
  QTimer::singleShot(0, this, SLOT(slotCancel()));
}

void POP3Resource::slotData( KIO::Job* job, const QByteArray &data )
{
  Q_UNUSED( job );

  if ( data.size() == 0 ) {
    if ( ( mStage == Retr ) && ( numMsgBytesRead < curMsgLen ) )
      numBytesRead += curMsgLen - numMsgBytesRead;
    return;
  }

  int oldNumMsgBytesRead = numMsgBytesRead;
  if ( mStage == Retr ) {
    headers = false;
    curMsgStrm->writeRawData( data.data(), data.size() );
    numMsgBytesRead += data.size();
    if ( numMsgBytesRead > curMsgLen )
      numMsgBytesRead = curMsgLen;
    numBytesRead += numMsgBytesRead - oldNumMsgBytesRead;
    dataCounter++;
    if ( dataCounter % 5 == 0 )
    {
      QString msg;
      if ( numBytes != numBytesToRead && Settings::leaveOnServer() )
      {
        msg = ki18n( "Fetching message %1 of %2 (%3 of %4 KB) for %5@%6 "
                     "(%7 KB remain on the server)." )
           .subs( indexOfCurrentMsg + 1 ).subs( numMsgs )
           .subs( numBytesRead / 1024 ).subs( numBytesToRead / 1024 )
           .subs( Settings::login() ).subs( Settings::host() ).subs( numBytes / 1024 )
           .toString();
      }
      else
      {
        msg = ki18n( "Fetching message %1 of %2 (%3 of %4 KB) for %5@%6." )
           .subs( indexOfCurrentMsg + 1 ).subs( numMsgs ).subs( numBytesRead / 1024 )
           .subs( numBytesToRead / 1024 ).subs( Settings::login() ).subs( Settings::host() )
           .toString();
      }
      emit status( Running, msg );
      emit percent(
        ( numBytesToRead <= 100 ) ? 50  // We never know what the server tells us
        // This way of dividing is required for > 21MB of mail
        : ( numBytesRead / ( numBytesToRead / 100 ) ) );
    }
    return;
  }

  /* FIXME: if ( mStage == Head ) {
    curMsgStrm->writeRawData( data.data(), data.size() );
    return;
  }*/

  // otherwise stage is List Or Uidl
  QByteArray qdata = data.simplified(); // Workaround for Maillennium POP3/UNIBOX
  const int spc = qdata.indexOf( ' ' );

  //Get rid of the null-terminating character if that exists.
  //Because mUidsOfSeenMsgsDict doesn't have those either, comparing the
  //values would otherwise fail.
  if ( qdata.at( qdata.size() - 1 ) == 0 )
    qdata.chop( 1 );

  if ( mStage == List ) {
    if ( spc > 0 ) {
      QByteArray length = qdata.mid( spc + 1 );
      const int spaceInLengthPos = length.indexOf( ' ' );
      if ( spaceInLengthPos != -1 )
        length.truncate( spaceInLengthPos );
      const int len = length.toInt();
      numBytes += len;
      QByteArray id = qdata.left( spc );
      idsOfMsgs.append( id );
      mMsgsPendingDownload.insert( id, len );
    }
    else {
      /*slotAbortRequested();
      KMessageBox::error( 0, i18n( "Unable to complete LIST operation." ),
                          i18n( "Invalid Response From Server" ) );*/
      //FIXME
      return;
    }
  }
  else { // stage == Uidl

    Q_ASSERT ( mStage == Uidl);
    QByteArray id;
    QByteArray uid;

    // If there is no space in the UIDL entry, the response is invalid.
    // However, some servers seem to do this, see bug 127696. Try to work
    // around this problem by downloading and deleting the message if we
    // can still parse the ID part of the entry.
    // Otherwise, just skip that message.
    if ( spc <= 0 ) {

      // Try to convert the entire UIDL entry to an ID
      QByteArray testID = qdata;
      bool idIsNumber;
      testID.toInt( &idIsNumber );
      if ( !idIsNumber ) {
        // we'll just have to skip this
        kWarning() << "Skipping UIDL entry due to parse error:" << qdata;
        return;
      }

      id = testID;
      // Generate a fake UID, so we don't get problems because all the code
      // requires a UID. This is a rather bad hack, but it works, since the
      // message will be deleted from the server.
      uid = QString( QString( "uidlgen" ) + time( 0 ) + QString( "." ) +
                     ( ++dataCounter ) ).toAscii();
      kWarning() << "Message" << id << "has bad UIDL, cannot keep a copy on server!";
      idsOfForcedDeletes.insert( id );
    }
    else {
      id = qdata.left( spc );
      uid = qdata.mid( spc + 1 );
    }

    mSizeOfNextSeenMsgsDict.insert( uid, mMsgsPendingDownload[id] );
    if ( mUidsOfSeenMsgsDict.contains( uid ) ) {
      if ( mMsgsPendingDownload.contains( id ) ) {
        mMsgsPendingDownload.remove( id );
      }
      else
        kDebug() << "Synchronization failure.";
      idsOfMsgsToDelete.insert( id );
      mUidsOfNextSeenMsgsDict.insert( uid, 1 );
      if ( mTimeOfSeenMsgsVector.empty() ) {
        mTimeOfNextSeenMsgsMap.insert( uid, time( 0 ) );
      }
      else {
        mTimeOfNextSeenMsgsMap.insert( uid,
                            mTimeOfSeenMsgsVector[ mUidsOfSeenMsgsDict[uid] ] );
      }
    }
    mUidForIdMap.insert( id, uid );
  }
}

void POP3Resource::slotResult( KJob* )
{
  if ( !mJob )
    return;
  if ( mJob->error() )
  {
    if ( interactive ) {
      if ( headers ) { // nothing to be done for headers
        idsOfMsgs.clear();
      }
      if ( mStage == Head && mJob->error() == KIO::ERR_COULD_NOT_READ )
      {
        /*KMessageBox::error(0, i18n("Your server does not support the "
          "TOP command. Therefore it is not possible to fetch the headers "
          "of large emails first, before downloading them."));*/
        slotCancel();
        // FIXME: later
        return;
      }
      // force the dialog to be shown next time the account is checked
      /*if ( !mStorePasswd )
        mPasswd.clear();*/ //FIXME:
      //mJob->ui()->setWindow( 0 );
      //mJob->ui()->showErrorMessage();
      // FIXME
    }
    slotCancel();
  }
  else
    slotJobFinished();
}

// finit state machine to cycle trow the stages
void POP3Resource::slotJobFinished()
{
  if ( mStage == List ) {
    kDebug() << "Finished LIST stage.";
    // set the initial size of mUidsOfNextSeenMsgsDict to the number of
    // messages on the server + 10%
    mUidsOfNextSeenMsgsDict.reserve( nextPrime( ( idsOfMsgs.count() * 11 ) / 10 ) );
    KUrl url = getUrl();
    url.setPath( "/uidl" );
    mJob = KIO::get( url, KIO::NoReload, KIO::HideProgressInfo );
    connectJob();
    mStage = Uidl;
  }
  else if ( mStage == Uidl ) {
    kDebug() << "Finished UIDL stage.";
    mUidlFinished = true;

    if ( Settings::leaveOnServer() && mUidForIdMap.isEmpty() &&
        mUidsOfNextSeenMsgsDict.isEmpty() && !idsOfMsgs.isEmpty() ) {
     /* KMessageBox::sorry(0, i18n("Your POP3 server (Account: %1) does not support "
      "the UIDL command: this command is required to determine, in a reliable way, "
      "which of the mails on the server KMail has already seen before;\n"
      "the feature to leave the mails on the server will therefore not "
      "work properly.", NetworkAccount::name()) );*/ //FIXME
      // An attempt to work around buggy pop servers, these seem to be popular.
      mUidsOfNextSeenMsgsDict = mUidsOfSeenMsgsDict;
    }

    //check if filter on server
    // FIXME (!)
    /*if ( mFilterOnServer == true) {
      for ( QMap<QByteArray, int>::const_iterator hids = mMsgsPendingDownload.constBegin();
            hids != mMsgsPendingDownload.constEnd(); ++hids ) {
          kDebug() <<"Length:" << hids.value();
          //check for mails bigger mFilterOnServerCheckSize
          if ( (unsigned int)hids.value() >= mFilterOnServerCheckSize ) {
            kDebug() <<"bigger than" << mFilterOnServerCheckSize;
            const QByteArray uid = mUidForIdMap[ hids.key() ];
            KMPopHeaders *header = new KMPopHeaders( hids.key(), uid, Later );
            //set Action if already known
            if ( mHeaderDeleteUids.contains( uid ) ) {
              header->setAction(Delete);
            }
            else if ( mHeaderDownUids.contains( uid ) ) {
              header->setAction(Down);
            }
            else if ( mHeaderLaterUids.contains( uid ) ) {
              header->setAction(Later);
            }
            mHeadersOnServer.append( header );
          }
      }
      // delete the uids so that you don't get them twice in the list
      mHeaderDeleteUids.clear();
      mHeaderDownUids.clear();
      mHeaderLaterUids.clear();
    }*/
    // kDebug() <<"Num of Msgs to Filter:" << mHeadersOnServer.count();
    // if there are mails which should be checkedc download the headers
    /* if ( ( mHeadersOnServer.count() > 0 ) && ( mFilterOnServer == true ) ) {
      KUrl url = getUrl();
      QByteArray headerIds = mHeadersOnServer[0]->id();
      for ( int i = 1; i < mHeadersOnServer.count(); ++i ) {
        headerIds += ',';
        headerIds += mHeadersOnServer[i]->id();
      }
      mHeaderIndex = 0;
      url.setPath( "/headers/" + headerIds );
      job = KIO::get( url, KIO::NoReload, KIO::HideProgressInfo );
      connectJob();
      slotGetNextHdr();
      stage = Head;
    }
    else */{
      mStage = Retr;
      numMsgs = mMsgsPendingDownload.count();
      numBytesToRead = 0;
      idsOfMsgs.clear();
      QByteArray ids;
      if ( numMsgs > 0 ) {
        for ( QMap<QByteArray, int>::const_iterator it = mMsgsPendingDownload.constBegin();
              it != mMsgsPendingDownload.constEnd(); ++it ) {
          numBytesToRead += it.value();
          ids += it.key() + ',';
          idsOfMsgs.append( it.key() );
        }
        ids.chop( 1 ); // cut off the trailing ','
      }
      KUrl url = getUrl();
      url.setPath( "/download/" + ids );
      mJob = KIO::get( url, KIO::NoReload, KIO::HideProgressInfo );
      connectJob();
      slotGetNextMsg();
      processMsgsTimer.start( processingDelay );
    }
  }
  /*else if (stage == Head) {
    kDebug() <<"stage == Head";

    // All headers have been downloaded, check which mail you want to get
    // data is in list mHeadersOnServer

    // check if headers apply to a filter
    // if set the action of the filter
    KMPopFilterAction action;
    bool dlgPopup = false;
    for ( int i = 0; i < mHeadersOnServer.count(); ++i ) {
      KMPopHeaders *header = mHeadersOnServer[i];
      action = (KMPopFilterAction)kmkernel->popFilterMgr()->process( header->header() );
      //debug todo
      switch ( action ) {
        case NoAction:
          kDebug() <<"PopFilterAction = NoAction";
          break;
        case Later:
          kDebug() <<"PopFilterAction = Later";
          break;
        case Delete:
          kDebug() <<"PopFilterAction = Delete";
          break;
        case Down:
          kDebug() <<"PopFilterAction = Down";
          break;
        default:
          kDebug() <<"PopFilterAction = default oops!";
          break;
      }
      switch ( action ) {
        case NoAction:
          //kDebug() <<"PopFilterAction = NoAction";
          dlgPopup = true;
          break;
        case Later:
          if (kmkernel->popFilterMgr()->showLaterMsgs())
            dlgPopup = true;
          // fall through
        default:
          header->setAction( action );
          header->setRuleMatched( true );
          break;
      }
    }

    // if there are some messages which are not coverd by a filter
    // show the dialog
    headers = true;
    if ( dlgPopup ) {
      mPopFilterConfirmationDialog =
          new KMPopFilterCnfrmDlg( mHeadersOnServer, this->name(),
                                   kmkernel->popFilterMgr()->showLaterMsgs() );
      connect( mPopFilterConfirmationDialog, SIGNAL( rejected() ),
               this, SLOT( slotAbortRequested() ) );
      mPopFilterConfirmationDialog->exec();
    }

    // If the dialog was accepted or never shown (because all pop filters already
    // set the actions), mark the messages for deletion, download or for keeping
    // them. Then advance to the next stage, the download stage.
    if ( !dlgPopup ||
         mPopFilterConfirmationDialog->result() == KDialog::Accepted ) {

      for ( int i = 0; i < mHeadersOnServer.count(); ++i ) {
        const KMPopHeaders *header = mHeadersOnServer[i];
        if ( header->action() == Delete || header->action() == Later) {
          //remove entries from the lists when the mails should not be downloaded
          //(deleted or downloaded later)
          mMsgsPendingDownload.remove( header->id() );
          if ( header->action() == Delete ) {
            mHeaderDeleteUids.insert( header->uid() );
            mUidsOfNextSeenMsgsDict.insert( header->uid(), 1 );
            idsOfMsgsToDelete.insert( header->id() );
            mTimeOfNextSeenMsgsMap.insert( header->uid(), time(0) );
          }
          else {
            mHeaderLaterUids.insert( header->uid() );
          }
        }
        else if ( header->action() == Down ) {
          mHeaderDownUids.insert( header->uid() );
        }
      }

      qDeleteAll( mHeadersOnServer );
      mHeadersOnServer.clear();
      stage = Retr;
      numMsgs = mMsgsPendingDownload.count();
      numBytesToRead = 0;
      idsOfMsgs.clear();
      QByteArray ids;
      if ( numMsgs > 0 ) {
        for ( QMap<QByteArray, int>::const_iterator it = mMsgsPendingDownload.constBegin();
              it != mMsgsPendingDownload.constEnd(); ++it ) {
          numBytesToRead += it.value();
          ids += it.key() + ',';
          idsOfMsgs.append( it.key() );
        }
        ids.chop( 1 ); // cut off the trailing ','
      }
      KUrl url = getUrl();
      url.setPath( "/download/" + ids );
      job = KIO::get( url, KIO::NoReload, KIO::HideProgressInfo );
      connectJob();
      slotGetNextMsg();
      processMsgsTimer.start(processingDelay);
    }
    delete mPopFilterConfirmationDialog;
    mPopFilterConfirmationDialog = 0;
  }*/
  else if ( mStage == Retr ) {
    kDebug() << "Finished RETR stage.";
    emit percent( 100 );
    emit status( Running, i18n( "Adding remaining messages to the target folder." ) );
    mStage = ProcessRemainingMessages;
    processRemainingQueuedMessages();
  }
  else if ( mStage == ProcessRemainingMessages ) {

    kDebug() << "Finished ProcessRemainingMessages stage.";
    mHeaderDeleteUids.clear();
    mHeaderDownUids.clear();
    mHeaderLaterUids.clear();

    //FIXME kmkernel->folderMgr()->syncAllFolders();

    KUrl url = getUrl();

    // Check if we want to keep any messages.
    //
    // The default is to delete all messages which have been successfully downloaded
    // or which we have seen before (which are remembered in the config file).
    // This excludes only messages which we have not seen before and at
    // the same time failed to download correctly, or messages which the pop
    // filter manager decided to leave on the server (the "download later" option)
    // The messages which we want to delete are contained in idsOfMsgsToDelete.
    //
    // In the code below, we check if any "leave on server" rules apply and remove
    // the messages which should be left on the server from idsOfMsgsToDelete.
    // This is done by storing the messages to leave on the server in idsToSave,
    // which is later subtracted from idsOfMsgsToDelete.

    // Start with an empty list of messages to keep
    QList< QPair<time_t, QByteArray> > idsToSave;

    if ( Settings::leaveOnServer() && !idsOfMsgsToDelete.isEmpty() ) {

      kDebug() << "We're going to leave some messages on the server.";

      // If the time-limited leave rule is checked, add the newer messages to
      // the list of messages to keep
      if ( Settings::leaveOnServerDays() > 0 && !mTimeOfNextSeenMsgsMap.isEmpty() ) {
        time_t timeLimit = time(0) - (86400 * Settings::leaveOnServerDays());
        for ( QSet<QByteArray>::const_iterator it = idsOfMsgsToDelete.constBegin();
              it != idsOfMsgsToDelete.constEnd(); ++it ) {
          time_t msgTime = mTimeOfNextSeenMsgsMap[ mUidForIdMap[*it] ];
          if ( msgTime >= timeLimit || msgTime == 0 ) {
            QPair<time_t, QByteArray> pair( msgTime, *it );
            kDebug() << "Going to save id" << *it;
            idsToSave.append( pair );
          }
        }
      }

      // Otherwise, add all messages to the list of messages to keep - this may
      // be reduced in the following number-limited leave rule and size-limited
      // leave rule checks
      else {
        kDebug() << "Keeping all messages on the server.";
        foreach ( const QByteArray& id, idsOfMsgsToDelete ) {
          time_t msgTime = mTimeOfNextSeenMsgsMap[ mUidForIdMap[id] ];
          QPair<time_t, QByteArray> pair( msgTime, id );
          idsToSave.append( pair );
        }
      }

      // sort the idsToSave list so that in the following we remove the oldest messages
      qSort( idsToSave );

      // Delete more old messages if there are more than mLeaveOnServerCount
      if ( Settings::leaveOnServerCount() > 0 ) {
        int numToDelete = idsToSave.count() - Settings::leaveOnServerCount();
        if ( numToDelete > 0 && numToDelete < idsToSave.count() ) {
          // get rid of the first numToDelete messages
          idsToSave = idsToSave.mid( numToDelete );
        }
        else if ( numToDelete >= idsToSave.count() )
          idsToSave.clear();
      }

      // Delete more old messages until we're under mLeaveOnServerSize MBs
      if ( Settings::leaveOnServerSize() > 0 ) {
        const long limitInBytes = Settings::leaveOnServerSize() * ( 1024 * 1024 );
        long sizeOnServer = 0;
        int firstMsgToKeep = idsToSave.count() - 1;
        for ( ; firstMsgToKeep >= 0 && sizeOnServer <= limitInBytes; --firstMsgToKeep ) {
          sizeOnServer +=
            mSizeOfNextSeenMsgsDict[ mUidForIdMap[ idsToSave[firstMsgToKeep].second ] ];
        }
        if ( sizeOnServer > limitInBytes )
          firstMsgToKeep++;
        if ( firstMsgToKeep > 0 )
          idsToSave = idsToSave.mid( firstMsgToKeep );
      }
      // Save msgs from deletion
      kDebug() << "Going to keep" << idsToSave.count() << "messages";
      for ( int i = 0; i < idsToSave.count(); ++i ) {
        //kDebug() << "keeping msg id" << idsToSave[i].second;
        idsOfMsgsToDelete.remove( idsToSave[i].second );
      }
    }

    if ( !idsOfForcedDeletes.isEmpty() ) {
      idsOfMsgsToDelete += idsOfForcedDeletes;
      idsOfForcedDeletes.clear();
    }

    // If there are messages to delete then delete them
    if ( !idsOfMsgsToDelete.isEmpty() ) {
      mStage = Dele;
      emit status( Running,
                   i18np( "Fetched 1 message from %2. Deleting messages from server...",
                          "Fetched %1 messages from %2. Deleting messages from server...",
                          numMsgs, Settings::host() ) );
      QSet<QByteArray>::const_iterator it = idsOfMsgsToDelete.constBegin();
      QByteArray ids = *it;
      ++it;
      for ( ; it != idsOfMsgsToDelete.constEnd(); ++it ) {
        ids += ',';
        ids += *it;
      }
      kDebug() << "Going to delete these messages:" << ids;
      url.setPath( "/remove/" + ids );
    } else {
      kDebug() << "No messages to delete.";
      mStage = Quit;
      emit status( Running, i18np( "Fetched 1 message from %2. Terminating transmission...",
                                   "Fetched %1 messages from %2. Terminating transmission...",
                                   numMsgs, Settings::host() ) );
      url.setPath( "/commit" );
    }
    mJob = KIO::get( url, KIO::NoReload, KIO::HideProgressInfo );
    connectJob();
  }
  else if (mStage == Dele) {
    kDebug() << "Finished DELE stage";
    // remove the uids of all messages which have been deleted
    for ( QSet<QByteArray>::const_iterator it = idsOfMsgsToDelete.constBegin();
          it != idsOfMsgsToDelete.constEnd(); ++it ) {
      mUidsOfNextSeenMsgsDict.remove( mUidForIdMap[*it] );
    }
    idsOfMsgsToDelete.clear();
    emit status( Running, i18np( "Fetched 1 message from %2. Terminating transmission...",
                                 "Fetched %1 messages from %2. Terminating transmission...",
                                 numMsgs, Settings::host() ) );
    KUrl url = getUrl();
    url.setPath( "/commit" );
    mJob = KIO::get( url, KIO::NoReload, KIO::HideProgressInfo );
    mStage = Quit;
    connectJob();
  }
  else if (mStage == Quit) {
    kDebug() << "Finished QUIT stage";
    saveUidList();
    mJob = 0;
    if ( mSlave )
      KIO::Scheduler::disconnectSlave(mSlave);
    mSlave = 0;
    mStage = Idle;

    // FIXME: correct place?
    collectionsRetrieved( Akonadi::Collection::List() );
    emit status( Idle );
    /*if ( mMailCheckProgressItem ) { // do this only once...
      bool canceled = !kmkernel || kmkernel->mailCheckAborted() ||
                      mMailCheckProgressItem->canceled();
      int numMessages = canceled ? indexOfCurrentMsg : idsOfMsgs.count();
      BroadcastStatus::instance()->setStatusMsgTransmissionCompleted(
                       this->name(), numMessages, numBytes, numBytesRead,
                       numBytesToRead, mLeaveOnServer, mMailCheckProgressItem );
      mMailCheckProgressItem->setComplete();
      mMailCheckProgressItem = 0;
      checkDone( ( numMessages > 0 ), canceled ? CheckAborted : CheckOK );
    }*/ //FIXME
  }
}

void POP3Resource::slotGetNextMsg()
{
  curMsgData.resize( 0 );
  numMsgBytesRead = 0;
  curMsgLen = 0;
  delete curMsgStrm;
  curMsgStrm = 0;

  if ( !mMsgsPendingDownload.isEmpty() ) {
    // get the next message
    QMap<QByteArray, int>::iterator next = mMsgsPendingDownload.begin();
    curMsgStrm = new QDataStream( &curMsgData, QIODevice::WriteOnly );
    curMsgLen = next.value();
    ++indexOfCurrentMsg;
    kDebug() << "Length of message about to get:" << curMsgLen;
    mMsgsPendingDownload.erase( next );
  }
}

void POP3Resource::slotProcessPendingMsgs()
{
  kDebug() << "Going to process pending messages, we have" << msgsAwaitingProcessing.count() << "pending.";

  // If we are in the processing stage and have nothing to process, advance to the
  // next stage immediately
  if ( msgsAwaitingProcessing.isEmpty() && mStage == ProcessRemainingMessages &&
       createJobsMap.isEmpty() ) {
    kDebug() << "No more messages to process, going to next stage.";
    slotJobFinished();
    return;
  }

  // For each message in the list of messages to be processed, create an
  // ItemCreateJob that adds the message to the target collection.
  // When such a job is complete, slotItemCreateResult() is called.
  while ( !msgsAwaitingProcessing.isEmpty() ) {

    MessagePtr msg = msgsAwaitingProcessing.dequeue();
    const QByteArray curId = msgIdsAwaitingProcessing.dequeue();
    const QByteArray curUid = msgUidsAwaitingProcessing.dequeue();

    kDebug() << "Going to add message with id" << curId << "to the target folder.";
    Q_ASSERT( Collection( Settings::targetCollection() ).isValid() );

    Akonadi::Item item;
    item.setMimeType( "message/rfc822" );
    item.setPayload<MessagePtr>( msg );
    Akonadi::Collection collection( Settings::targetCollection() );
    ItemCreateJob *job = new ItemCreateJob( item, collection );
    IdUidPair pair( curId, curUid );
    createJobsMap.insert( job, pair );

    connect( job, SIGNAL( result( KJob* ) ),
             this, SLOT( slotItemCreateResult( KJob*) ) );
  }
}

void POP3Resource::slotItemCreateResult( KJob* job )
{
  Akonadi::ItemCreateJob* createJob = dynamic_cast<Akonadi::ItemCreateJob*>( job );
  Q_ASSERT( job );
  if ( !createJobsMap.contains( createJob ) ) {
    kWarning() << "Got a message from a job that is no longer in the map!";
    Q_ASSERT( mStage == Idle );
    // FIXME: What should we actually do here? This means duplicated messages on the
    //        next mail check!! Should only happen when the user canceled the mail check...
    // Maybe add the message to the seen uid list??
    return;
  }

  IdUidPair pair = createJobsMap.value( createJob );
  createJobsMap.remove( createJob );

  if ( job->error() ) {
    kWarning() << "Could not add the message to the folder! Error was:" << job->errorString();
    cancelTask( i18n("The message could not be added to the target folder.\n%1", job->errorString() ) );
    slotAbortRequested();
    mMsgsPendingDownload.clear();
    return;
  }
  else {
    QByteArray curId = pair.first;
    QByteArray curUid = pair.second;
    kDebug() << "Processed mail with POP3 ID" << curId << "and UID" << curUid
             << ". The Akonadi ID is" << createJob->item().id();

    // Now that the message is added to the folder correctly, we can delete it
    // from the server.
    // If something else fails in the meantime and the message is not deleted,
    // we at least add the message to the list of "next seen" messages, which 
    // will cause KMail to not download the message on the next check again,
    // to prevent duplicate mails.
    Q_ASSERT( !idsOfMsgsToDelete.contains ( curId ) );
    Q_ASSERT( !mUidsOfNextSeenMsgsDict.contains( curUid ) );
    Q_ASSERT( !mTimeOfNextSeenMsgsMap.contains( curUid ) );
    idsOfMsgsToDelete.insert( curId );
    mUidsOfNextSeenMsgsDict.insert( curUid, 1 );
    mTimeOfNextSeenMsgsMap.insert( curUid, time( 0 ) );

    // If we processed all messages, we can advance to the next stage
    if ( createJobsMap.isEmpty() && mStage == ProcessRemainingMessages ) {

      // slotJobFinished of the Retr stage should have called processRemainingQueuedMgs.
      // At this point, we really want to have all messages processed, because the next
      // stage will be delete, and only processed messages will be deleted correctly
      Q_ASSERT( msgsAwaitingProcessing.isEmpty() );

      // This will activate the next stage, Delete or Quit.
      slotJobFinished();
    }
  }
}


// one message is finished
// add data to a KMMessage
void POP3Resource::slotMsgRetrieved( KJob*, const QString &infoMsg, const QString & )
{
  if ( infoMsg != "message complete" )
    return;

  kDebug() << "Got a complete message with id" << idsOfMsgs[indexOfCurrentMsg];


  // Make sure to use LF as line ending to make the processing easier
  // when piping through external programs
  MessagePtr msg( new KMime::Message );
  msg->setContent( KMime::CRLFtoLF( curMsgData ) );
  msg->parse();

  /*if ( stage == Head ) {
  //FIXME:
    KMPopHeaders *header = mHeadersOnServer[ mHeaderIndex ];
    int size = mMsgsPendingDownload[ header->id() ];
    kDebug() <<"Size of Message:" << size;
    msg->setMsgLength( size );
    header->setHeader( msg );
    ++mHeaderIndex;
    slotGetNextHdr();
  } else*/ {
    //kDebug() << kfuncinfo <<"stage == Retr";
    //kDebug() <<"curMsgData.size() =" << curMsgData.size();
    //msg->setMsgLength( curMsgData.size() );
    msgsAwaitingProcessing.enqueue( msg );
    msgIdsAwaitingProcessing.enqueue( idsOfMsgs[indexOfCurrentMsg] );
    msgUidsAwaitingProcessing.enqueue( mUidForIdMap[ idsOfMsgs[indexOfCurrentMsg] ] );
    slotGetNextMsg();
  }
}

QString POP3Resource::protocol() const
{
  return Settings::useSSL() ? POP_SSL_PROTOCOL : POP_PROTOCOL;
}

unsigned short int POP3Resource::defaultPort() const
{
  return pop3DefaultPort;
}

void POP3Resource::cancelMailCheck()
{
  slotAbortRequested();
}

void POP3Resource::processRemainingQueuedMessages()
{
  slotProcessPendingMsgs(); // Force processing of any messages still in the queue
  processMsgsTimer.stop();

  /*if ( kmkernel && kmkernel->folderMgr() ) {
    kmkernel->folderMgr()->syncAllFolders();
  }*/ // FIXME(?)
}

void POP3Resource::saveUidList()
{
  kDebug() << "Going to save the UID list of messages which are still on the server.";

  // Don't update the seen uid list unless we successfully got
  // a new list from the server
  if ( !mUidlFinished )
    return;

  QStringList uidsOfNextSeenMsgs;
  QList<int> seenUidTimeList;
  foreach( const QByteArray &uid, mUidsOfNextSeenMsgsDict.keys() ) {
    uidsOfNextSeenMsgs.append( uid );
    seenUidTimeList.append( mTimeOfNextSeenMsgsMap[ uid ] );
  }

  QStringList laterList;
  foreach( const QByteArray& uid, mHeaderLaterUids ) {
    laterList.append( uid );
  }

  Settings::setSeenUidList( uidsOfNextSeenMsgs );
  Settings::setSeenUidTimeList( seenUidTimeList );
  Settings::setDownloadLater( laterList );
  Settings::self()->writeConfig();
  Settings::self()->config()->sync();
}

void POP3Resource::slotAbortRequested()
{
  kDebug() << "Aborting mail check, going to the quit stage.";
  if ( mStage == Idle )
    return;
  //disconnect( mMailCheckProgressItem, SIGNAL( progressItemCanceled( KPIM::ProgressItem* ) ),
  //            this, SLOT( slotAbortRequested() ) );
  //FIXME
  mStage = Quit;
  if ( mJob )
    mJob->kill();
  mJob = 0;
  mSlave = 0;
  slotCancel();
}

void POP3Resource::slotCancel()
{
  kDebug() << "Canceling mail check.";
  mMsgsPendingDownload.clear();
  createJobsMap.clear();
  processMsgsTimer.stop();
  saveUidList();
  slotJobFinished();

  // Close the pop filter confirmation dialog. Otherwise, KMail crashes because
  // slotJobFinished(), which creates that dialog, will try to continue downloading
  // when the user closes the dialog.
  /*if ( mPopFilterConfirmationDialog ) {

    // Disconnect the signal, as we are already called from slotAbortRequested()
    disconnect( mPopFilterConfirmationDialog, SIGNAL( rejected() ),
                this, SLOT( slotAbortRequested() ) );
    mPopFilterConfirmationDialog->reject();
  }*/
  // FIXME: make pop filters work
}

/* //FIXME: implement me
void PopAccount::slotGetNextHdr(){
  kDebug() <<"slotGetNextHeader";

  curMsgData.resize(0);
  delete curMsgStrm;
  curMsgStrm = 0;

  curMsgStrm = new QDataStream( &curMsgData, QIODevice::WriteOnly );
}*/


AKONADI_RESOURCE_MAIN( POP3Resource )

#include "pop3resource.moc"
