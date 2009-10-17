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
#include "jobs.h"

#include <Akonadi/CollectionFetchJob>
#include <Akonadi/ItemCreateJob>
#include <akonadi/kmime/localfoldersrequestjob.h>
#include <akonadi/kmime/localfolders.h>

#include <KIO/PasswordDialog>
#include <KMessageBox>

using namespace Akonadi;

POP3Resource::POP3Resource( const QString &id )
    : ResourceBase( id ),
      mState( Idle ),
      mPopSession( 0 ),
      mAskAgain( false )
{
  new SettingsAdaptor( Settings::self() );

  QDBusConnection::sessionBus().registerObject( QLatin1String( "/Settings" ),
                            Settings::self(), QDBusConnection::ExportAdaptors );
  connect( this, SIGNAL(abortRequested()),
           this, SLOT(slotAbortRequested()) );
}

POP3Resource::~POP3Resource()
{
  Settings::self()->writeConfig();
}

void POP3Resource::aboutToQuit()
{
  if ( mState != Idle )
    cancelSync( i18n( "Mail check aborted." ) );
}

void POP3Resource::slotAbortRequested()
{
  if ( mState != Idle )
    cancelSync( i18n( "Mail check was canceled manually." ), false /* no error */ );
}

void POP3Resource::configure( WId windowId )
{
  QPointer<AccountDialog> accountDialog( new AccountDialog( this, windowId ) );
  if ( accountDialog->exec() == QDialog::Accepted ) {
    emit configurationDialogAccepted();
  }
  else {
    emit configurationDialogRejected();
  }

  if ( accountDialog ) {
    delete accountDialog;
  }
}

void POP3Resource::retrieveItems( const Akonadi::Collection &collection )
{
  Q_UNUSED( collection );
  kWarning() << "This should never be called, we don't have a collection!";
}

bool POP3Resource::retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts )
{
  Q_UNUSED( item );
  Q_UNUSED( parts );
  kWarning() << "This should never be called, we don't have any item!";
  return false;
}

void POP3Resource::doStateStep()
{
  switch ( mState )
  {
  case Idle:
    {
      Q_ASSERT( false );
      kWarning() << "State machine should not be called in idle state!";
      break;
    }
  case FetchTargetCollection:
    {
      kDebug() << "================ Starting state FetchTargetCollection ==========";
      emit status( Running, i18n( "Preparing transmission from \"%1\"...", name() ) );
      Collection targetCollection( Settings::targetCollection() );
      if ( !targetCollection.isValid() ) {
        // No target collection set in the config? Try requesting a default inbox
        LocalFoldersRequestJob *requestJob = new LocalFoldersRequestJob( this );
        requestJob->requestDefaultFolder( LocalFolders::Inbox );
        requestJob->start();
        connect ( requestJob, SIGNAL(result(KJob*)),
                  this, SLOT(localFolderRequestJobFinished(KJob*)) );
      }
      else {
        CollectionFetchJob *fetchJob = new CollectionFetchJob( targetCollection,
                                                          CollectionFetchJob::Base );
        fetchJob->start();
        connect( fetchJob, SIGNAL(result(KJob*)),
                 this, SLOT(targetCollectionFetchJobFinished(KJob*)) );
      }
      break;
    }
  case Connect:
    {
      kDebug() << "================ Starting state Connect ========================";
      Q_ASSERT( !mPopSession );
      mPopSession = new POPSession();
      connect( mPopSession, SIGNAL( slaveError(int,const QString &)),
               this, SLOT(slotSessionError(int, const QString&)) );
      mState = Login;
      doStateStep();
      break;
    }
  case Login:
    {
      kDebug() << "================ Starting state Login ==========================";
      // Show the user/password dialog if needed
      if ( ( mAskAgain || Settings::password().isEmpty() || Settings::login().isEmpty() ) &&
           Settings::authenticationMethod() != "GSSAPI" ) {
        QString password = Settings::password();
        QString login = Settings::login();
        bool rememberPassword = Settings::storePassword();

        // FIXME: give this a proper parent widget
        if ( KIO::PasswordDialog::getNameAndPassword(
             login, password, &rememberPassword,
             i18n( "You need to supply a username and a password to access this "
                   "mailbox." ),
             false, name(), name(), i18n( "Account:" ) ) != KDialog::Accepted )
          {
          cancelSync( i18n( "No username and password supplied." ) );
          return;
        } else {
          Settings::setPassword( password );
          Settings::setLogin( login );
          Settings::setStorePassword( rememberPassword );
          if ( rememberPassword ) {
            Settings::self()->writeConfig();
          }
          mAskAgain = false;
        }
      }

      LoginJob *loginJob = new LoginJob( mPopSession );
      connect( loginJob, SIGNAL(result(KJob*)),
               this, SLOT(loginJobResult(KJob*)) );
      loginJob->start();
      break;
    }
  case List:
    {
      kDebug() << "================ Starting state List =============================";
      emit status( Running, i18n( "Starting mail check..." ) );
      ListJob *listJob = new ListJob( mPopSession );
      connect( listJob, SIGNAL(result(KJob*)),
               this, SLOT(listJobResult(KJob*)) );
      listJob->start();
    }
    break;
  case UIDList:
    {
      kDebug() << "================ Starting state UIDList ========================";
      UIDListJob *uidListJob = new UIDListJob( mPopSession );
      connect( uidListJob, SIGNAL(result(KJob*)),
               this, SLOT(uidListJobResult(KJob*)) );
      uidListJob->start();
    }
    break;
  case Download:
    {
      kDebug() << "================ Starting state Download =========================";
      FetchJob *fetchJob = new FetchJob( mPopSession );

      // Determine which mails we want to download. Those are all mails which are
      // currently on ther server, minus the ones we have already downloaded (we
      // remember which UIDs we have downloaded in the settings)
      QList<int> idsToDownload = mIdsToSizeMap.keys();
      const QList<QString> UIDsOnServer = mIdsToUidsMap.values();
      const QList<QString> alreadyDownloadedUIDs = Settings::seenUidList();
      foreach( const QString &uidOnServer, UIDsOnServer ) {
        if ( alreadyDownloadedUIDs.contains( uidOnServer ) ) {
          const int idOfUIDOnServer = mIdsToUidsMap.key( uidOnServer, -1 );
          Q_ASSERT( idOfUIDOnServer != -1 );
          idsToDownload.removeAll( idOfUIDOnServer );
        }
      }
      mIdsToDownload = idsToDownload;
      kDebug() << "We are going to download" << mIdsToDownload.size() << "messages";

      // For proper progress, the job needs to know the sizes of the messages, so
      // put them into a list here
      QList<int> sizesOfMessagesToDownload;
      foreach( int id, idsToDownload ) {
        sizesOfMessagesToDownload.append( mIdsToSizeMap.value( id ) );
      }

      fetchJob->setFetchIds( idsToDownload, sizesOfMessagesToDownload );
      connect( fetchJob, SIGNAL(result(KJob*)),
               this, SLOT(fetchJobResult(KJob*)) );
      connect( fetchJob, SIGNAL(messageFinished(int,KMime::Message::Ptr)),
               this, SLOT(messageFinished(int,KMime::Message::Ptr)) );
      connect( fetchJob, SIGNAL(processedAmount(KJob*,KJob::Unit,qulonglong)),
               this, SLOT(messageDownloadProgress(KJob*,KJob::Unit,qulonglong)) );
      fetchJob->start();
    }
    break;
  case Save:
    {
      kDebug() << "================ Starting state Save =============================";
      kDebug() << mPendingCreateJobs.size() << "item create jobs are pending";
      if ( mPendingCreateJobs.size() > 0 )
        emit status( Running, i18n( "Saving downloaded messages..." ) );

      // It can happen that the create job map is empty, for example if there was no
      // mail to download or if all ItemCreateJob's finished before reaching this
      // stage
      if ( mPendingCreateJobs.isEmpty() ) {
        mState = Delete;
        doStateStep();
      }
    }
    break;
  case Delete:
    {
      kDebug() << "================ Starting state Delete ===========================";
      emit status( Running, i18n( "Deleting messages from the server.") );
      QList<int> idsToKill = idsToDelete();
      if ( !idsToKill.isEmpty() ) {
        DeleteJob *deleteJob = new DeleteJob( mPopSession );
        deleteJob->setDeleteIds( idsToKill );
        connect( deleteJob, SIGNAL(result(KJob*)),
                 this, SLOT(deleteJobResult(KJob*)) );
        deleteJob->start();
      }
      else {
        mState = Quit;
        doStateStep();
      }
    }
    break;
  case Quit:
    {
      kDebug() << "================ Starting state Quit =============================";
      QuitJob *quitJob = new QuitJob( mPopSession );
      connect( quitJob, SIGNAL(result(KJob*)),
               this, SLOT(quitJobResult(KJob*)) );
      quitJob->start();
    }
    break;
  }
}

void POP3Resource::localFolderRequestJobFinished( KJob *job )
{
  if ( job->error() ) {
    cancelSync( i18n( "Error while trying to get the local inbox folder, "
                      "aborting mail check." ) + "\n" + job->errorString() );
    return;
  }

  mTargetCollection = LocalFolders::self()->defaultFolder( LocalFolders::Inbox );
  Q_ASSERT( mTargetCollection.isValid() );
  mState = Connect;
  doStateStep();
}

void POP3Resource::targetCollectionFetchJobFinished( KJob *job )
{
  if ( job->error() ) {
    cancelSync( i18n( "Error while trying to get the folder for incoming mail, "
                      "aborting mail check." ) + "\n" + job->errorString() );
    return;
  }
  Akonadi::CollectionFetchJob *fetchJob =
      dynamic_cast<Akonadi::CollectionFetchJob*>( job );
  Q_ASSERT( fetchJob );
  Q_ASSERT( fetchJob->collections().size() <= 1 );

  if ( fetchJob->collections().isEmpty() ) {
    cancelSync( i18n( "Could not find folder for incoming mail, aborting mail check.") );
    return;
  }
  else {
    mTargetCollection = fetchJob->collections().first();
    mState = Connect;
    doStateStep();
  }
}

void POP3Resource::loginJobResult( KJob *job )
{
  if ( job->error() ) {
    if ( job->error() == KIO::ERR_COULD_NOT_LOGIN && !Settings::storePassword() )
      mAskAgain = true;
    cancelSync( i18n( "Unable to login to the server %1.", Settings::host() ) +
                '\n' + job->errorString() );
  }
  else {
    mState = List;
    doStateStep();
  }
}

void POP3Resource::listJobResult( KJob *job )
{
  if ( job->error() ) {
    cancelSync( i18n( "Error while getting the list of messages on the server." ) +
                '\n' + job->errorString() );
  }
  else {
    ListJob *listJob = dynamic_cast<ListJob*>( job );
    Q_ASSERT( listJob );
    mIdsToSizeMap = listJob->idList();
    kDebug() << "IdsToSizeMap:" << mIdsToSizeMap;
    mState = UIDList;
    doStateStep();
  }
}

void POP3Resource::uidListJobResult( KJob *job )
{
  if ( job->error() ) {
    cancelSync( i18n( "Error while getting list of unique mail identifiers from the server." ) +
                '\n' + job->errorString() );
  }
  else {
    UIDListJob *listJob = dynamic_cast<UIDListJob*>( job );
    Q_ASSERT( listJob );
    mIdsToUidsMap = listJob->uidList();
    kDebug() << "IdToUidMap:" << mIdsToUidsMap;

    if ( Settings::leaveOnServer() && mIdsToUidsMap.isEmpty() &&
         !mIdsToSizeMap.isEmpty() ) {
      // FIXME: this needs a proper parent window
      KMessageBox::sorry( 0,
            i18n( "Your POP3 server (Account: %1) does not support "
                  "the UIDL command: this command is required to determine, in a reliable way, "
                  "which of the mails on the server KMail has already seen before;\n"
                  "the feature to leave the mails on the server will therefore not "
                  "work properly.", name() ) );
    }

    mState = Download;
    doStateStep();
  }
}

void POP3Resource::fetchJobResult( KJob *job )
{
  if ( job->error() ) {
    cancelSync( i18n( "Error while fetching mails from the server." ) +
                '\n' + job->errorString() );
    return;
  }
  else {
    kDebug() << "Downloaded" << mDownloadedIDs.size() << "mails";

    if ( !mIdsToDownload.isEmpty() ) {
      kWarning() << "We did not download all messages, there are still some remaining "
                    "IDs, even though we requested their download:" << mIdsToDownload;
    }

    mState = Save;
    doStateStep();
  }
}

void POP3Resource::messageFinished( int messageId, KMime::Message::Ptr message )
{
  if( mState != Download ) {
    // This can happen if the slave does not get notified in time about the fact
    // that the job was killed
    return;
  }

  kDebug() << "Got message" << messageId
           << "with subject" << message->subject()->asUnicodeString();

  Akonadi::Item item;
  item.setMimeType( "message/rfc822" );
  item.setPayload<KMime::Message::Ptr>( message );

  ItemCreateJob *itemCreateJob = new ItemCreateJob( item, mTargetCollection );
  // TEST ItemCreateJob *itemCreateJob = new ItemCreateJob( item, Collection( -1 ) );

  mPendingCreateJobs.insert( itemCreateJob, messageId );
  connect( itemCreateJob, SIGNAL(result(KJob*)),
           this, SLOT(itemCreateJobResult(KJob*)) );

  mDownloadedIDs.append( messageId );
  mIdsToDownload.removeAll( messageId );
}

void POP3Resource::messageDownloadProgress( KJob *job, KJob::Unit unit, qulonglong totalBytes )
{
  Q_UNUSED( totalBytes );
  Q_ASSERT( unit == KJob::Bytes );
  QString statusMessage;
  const int totalMessages = mIdsToDownload.size() + mDownloadedIDs.size();
  int bytesRemainingOnServer = 0;
  foreach( const QString &alreadyDownloadedUID, Settings::seenUidList() ) {
    const int alreadyDownloadedID = mIdsToUidsMap.key( alreadyDownloadedUID, -1 );
    if ( alreadyDownloadedID != -1 )
      bytesRemainingOnServer += mIdsToSizeMap.value( alreadyDownloadedID );
  }

  if ( Settings::leaveOnServer() && bytesRemainingOnServer > 0 ) {

    statusMessage = i18n( "Fetching message %1 of %2 (%3 of %4 KB) for %5 "
                          "(%6 KB remain on the server).",
                          mDownloadedIDs.size() + 1, totalMessages,
                          job->processedAmount( KJob::Bytes ) / 1024,
                          job->totalAmount( KJob::Bytes ) / 1024, name(),
                          bytesRemainingOnServer / 1024 );
  }
  else {
    statusMessage = i18n( "Fetching message %1 of %2 (%3 of %4 KB) for %5",
                          mDownloadedIDs.size() + 1, totalMessages,
                          job->processedAmount( KJob::Bytes ) / 1024,
                          job->totalAmount( KJob::Bytes ) / 1024, name() );
  }
  emit status( Running, statusMessage );
  emit percent( job->percent() );
}

void POP3Resource::itemCreateJobResult( KJob *job )
{
  if ( mState != Download && mState != Save ) {
    // This can happen if the slave does not get notified in time about the fact
    // that the job was killed
    return;
  }

  ItemCreateJob *createJob = dynamic_cast<ItemCreateJob*>( job );
  Q_ASSERT( createJob );

  if ( job->error() ) {
    cancelSync( i18n( "Unable to store downloaded mails." ) +
                '\n' + job->errorString() );
    return;
  }

  const int idOfMessageJustCreated = mPendingCreateJobs.value( createJob, -1 );
  Q_ASSERT( idOfMessageJustCreated != -1 );
  mPendingCreateJobs.remove( createJob );
  mIDsStored.append( idOfMessageJustCreated );
  kDebug() << "Just stored message with ID" << idOfMessageJustCreated
           << "on the Akonadi server";

  // Have all create jobs finished? Go to the next state, then
  if ( mState == Save && mPendingCreateJobs.isEmpty() ) {
    mState = Delete;
    doStateStep();
  }
}

int POP3Resource::idToTime( int id ) const
{  
  const QString uid = mIdsToUidsMap.value( id );
  if ( !uid.isEmpty() ) {
    const int index = Settings::seenUidList().indexOf( uid );
    if ( index != -1 )
      return Settings::seenUidTimeList().at( index );
  }

  // If we don't find any mail, either we have no UID, or it is not in the seen UID
  // list. In that case, we assume that the mail is new, i.e. from now
  return time( 0 );
}

int POP3Resource::idOfOldestMessage( QList<int> &idList ) const
{
  int timeOfOldestMessage = time( 0 ) + 999;
  int idOfOldestMessage = -1;
  foreach( int id, idList ) {
    const int idTime = idToTime( id );
    if ( idTime < timeOfOldestMessage ) {
      timeOfOldestMessage = idTime;
      idOfOldestMessage = id;
    }
  }
  Q_ASSERT( idList.isEmpty() || idOfOldestMessage != -1 );
  return idOfOldestMessage;
}

QList<int> POP3Resource::idsToDelete() const
{
  QList<int> idsToDeleteFromServer = mIdsToSizeMap.keys();
  QList<int> idsToSave;

  // Don't attempt to delete messages that weren't downloaded correctly
  foreach( int idNotDownloaded, mIdsToDownload )
    idsToDeleteFromServer.removeAll( idNotDownloaded );

  // By default, we delete all messages. But if we have "leave on server"
  // rules, we can save some messages.
  if ( Settings::leaveOnServer() && !idsToDeleteFromServer.isEmpty() ) {

    // If the time-limited leave rule is checked, add the newer messages to
    // the list of messages to keep
    if ( Settings::leaveOnServerDays() > 0 ) {
      const int secondsPerDay = 86400;
      time_t timeLimit = time( 0 ) - ( secondsPerDay * Settings::leaveOnServerDays() );
      foreach( int idToDelete, idsToDeleteFromServer ) {
        const int msgTime = idToTime( idToDelete );
        if ( msgTime >= timeLimit ) {
          idsToSave.append( idToDelete );
        }
        else {
          kDebug() << "Message" << idToDelete << "is too old and will be deleted.";
        }
      }
    }

    // Otherwise, add all messages to the list of messages to keep - this may
    // be reduced in the following number-limited leave rule and size-limited
    // leave rule checks
    else {
      foreach ( int idToDelete, idsToDeleteFromServer ) {
        idsToSave.append( idToDelete );
      }
    }

    //
    // Delete more old messages if there are more than mLeaveOnServerCount
    //
    if ( Settings::leaveOnServerCount() > 0 ) {
      const int numToDelete = idsToSave.count() - Settings::leaveOnServerCount();
      if ( numToDelete > 0 && numToDelete < idsToSave.count() ) {
        // Get rid of the first numToDelete messages
        for ( int i = 0; i < numToDelete; i++ ) {
          idsToSave.removeAll( idOfOldestMessage( idsToSave ) );
        }
      }
      else if ( numToDelete >= idsToSave.count() )
        idsToSave.clear();
    }

    //
    // Delete more old messages until we're under mLeaveOnServerSize MBs
    //
    if ( Settings::leaveOnServerSize() > 0 ) {
      const qint64 limitInBytes = Settings::leaveOnServerSize() * ( 1024 * 1024 );
      qint64 sizeOnServerAfterDeletion = 0;
      foreach( int id, idsToSave ) {
        sizeOnServerAfterDeletion += mIdsToSizeMap.value( id );
      }
      while ( sizeOnServerAfterDeletion > limitInBytes ) {
        int oldestId = idOfOldestMessage( idsToSave );
        idsToSave.removeAll( oldestId );
        sizeOnServerAfterDeletion -= mIdsToSizeMap.value( oldestId );
      }
    }

    //
    // Now save the messages from deletion
    //
    foreach( int idToSave, idsToSave ) {
      idsToDeleteFromServer.removeAll( idToSave );
    }
  }

  // I don't trust this enough...
  kDebug() << "Going to delete" << idsToDeleteFromServer.size() << idsToDeleteFromServer;
  return idsToDeleteFromServer;
  // TEST return QList<int>();
}

void POP3Resource::deleteJobResult( KJob *job )
{
  if ( job->error() ) {
    cancelSync( i18n( "Failed to delete the messages from the server.") +
                '\n' + job->errorString() );
    return;
  }

  DeleteJob *deleteJob = dynamic_cast<DeleteJob*>( job );
  Q_ASSERT( deleteJob );
  mDeletedIDs = deleteJob->deletedIDs();

  // Remove all deleted messages from the list of already downloaded messages,
  // as it is no longer necessary to store them (they just waste space)
  QList<QString> seenUIDs = Settings::seenUidList();
  QList<int> timeOfSeenUids = Settings::seenUidTimeList();
  Q_ASSERT( seenUIDs.size() == timeOfSeenUids.size() );
  foreach( int deletedId, mDeletedIDs ) {
    QString deletedUID = mIdsToUidsMap.value( deletedId );
    if ( !deletedUID.isEmpty() ) {
      int index = seenUIDs.indexOf( deletedUID );
      if ( index != -1 ) {
        // TEST
        kDebug() << "Removing UID" << deletedUID << "from the seen UID list, as it was deleted.";
        seenUIDs.removeAt( index );
        timeOfSeenUids.removeAt( index );
      }
    }
  }
  Settings::setSeenUidList( seenUIDs );
  Settings::setSeenUidTimeList( timeOfSeenUids );
  Settings::self()->writeConfig(),

  mState = Quit;
  doStateStep();
}

void POP3Resource::quitJobResult( KJob *job )
{
  if ( job->error() ) {
    cancelSync( i18n( "Unable to complete the mail fetch." ) +
                '\n' + job->errorString() );
    return;
  }

  kDebug() << "================= Mail check finished. =============================";
  saveSeenUIDList();
  collectionsRetrieved( Akonadi::Collection::List() );
  if ( mDownloadedIDs.isEmpty() )
    emit status( Idle, i18n( "Finished mail check, no message downloaded." ) );
  else
    emit status( Idle, i18np( "Finished mail check, 1 message downloaded.",
                              "Finished mail check, %1 messages downloaded.",
                              mDownloadedIDs.size(), mDownloadedIDs.size() ) );

  resetState();
}

void POP3Resource::slotSessionError( int errorCode, const QString &errorMessage )
{
  kWarning() << "Error in our session, unrelated to a currently running job!";
  cancelSync( KIO::buildErrorString( errorCode, errorMessage ) );
}

void POP3Resource::saveSeenUIDList()
{
  // Find the messages that we have sucessfully stored, but did not actually get
  // deleted
  QList<int> idsOfMessagesDownloadedButNotDeleted = mIDsStored;
  foreach( int deletedId, mDeletedIDs )
    idsOfMessagesDownloadedButNotDeleted.removeAll( deletedId );

  // Those messages, we have to remember, so we don't download them again. Therefore,
  // we save the UIDs of those messages in the settings.
  QList<QString> uidsOfMessagesDownloadedButNotDeleted;
  foreach( int id, idsOfMessagesDownloadedButNotDeleted ) {
    QString uid = mIdsToUidsMap.value( id );
    if ( !uid.isEmpty() ) {
      uidsOfMessagesDownloadedButNotDeleted.append( uid );
    }
  }

  QList<QString> seenUIDs = Settings::seenUidList();
  QList<int> timeOfSeenUIDs = Settings::seenUidTimeList();
  Q_ASSERT( seenUIDs.size() == timeOfSeenUIDs.size() );
  foreach( const QString uid, uidsOfMessagesDownloadedButNotDeleted ) {
    if ( !seenUIDs.contains( uid ) ) {
      // Test
      kDebug() << "Appending to seen UID list:" << uid;
      seenUIDs.append( uid );
      timeOfSeenUIDs.append( time( 0 ) );
    }
  }

  kDebug() << "The seen UID list has" << seenUIDs.size() << "entries";
  Settings::setSeenUidList( seenUIDs );
  Settings::setSeenUidTimeList( timeOfSeenUIDs );
  Settings::self()->writeConfig();
}

void POP3Resource::cancelSync( const QString &errorMessage, bool error )
{
  if ( error ) {
    cancelTask( errorMessage );
    kWarning() << "============== ERROR DURING POP3 SYNC ==========================";
    kWarning() << errorMessage;
  }
  else {
    kDebug() << "Canceled the sync, but no error.";
    cancelTask();
  }
  saveSeenUIDList();
  resetState();
}

void POP3Resource::resetState()
{
  mState = Idle;
  mTargetCollection = Collection( -1 );
  mIdsToSizeMap.clear();
  mIdsToUidsMap.clear();
  mDownloadedIDs.clear();
  mIdsToDownload.clear();
  mPendingCreateJobs.clear();
  mIDsStored.clear();
  mDeletedIDs.clear();
  if ( mPopSession ) {
    // Closing the POP session means the KIO slave will get disconnected, which
    // automatically issues the QUIT command.
    // Delete the POP session later, otherwise the scheduler makes us crash
    mPopSession->abortCurrentJob();
    mPopSession->deleteLater();
    mPopSession = 0;
  }
}

void POP3Resource::retrieveCollections()
{
  if ( mState == Idle ) {
    resetState();
    emit percent( 0 ); // Otherwise the value from the last sync is taken
    mState = FetchTargetCollection;
    doStateStep();
  }
  else {
    cancelSync(
        i18n( "Mail check already in progress, unable to start a second check." ) );
  }
}

#if 0

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
}

POP3Resource::~POP3Resource()
{
  /*if ( mJob ) {
    mJob->kill();
    mMsgsPendingDownload.clear();
    createJobsMap.clear();
    saveUidList();
  }*/

  Settings::self()->writeConfig();
}

void POP3Resource::retrieveCollections()
{ 
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

  // TMG disabled numBytes = 0;
  // TMG disabled numBytesRead = 0;
  mStage = List;

  mSlaveHolder->createSlave();
  ListJob *listJob = new ListJob();
  listJob->start();
  // TMG: connect that listjob
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
      Q_ASSERT( mMsgsPendingDownload.contains( id ) );
      mMsgsPendingDownload.remove( id );
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
    // set the initial size of mUidsOfNextSeenMsgsDict to the number of
    // messages on the server + 10%
    mUidsOfNextSeenMsgsDict.reserve( idsOfMsgs.count() * 11 / 10 );
    KUrl url = getUrl();
    url.setPath( "/uidl" );
    mJob = KIO::get( url, KIO::NoReload, KIO::HideProgressInfo );
    connectJob();
    mStage = Uidl;
  }
  else if ( mStage == Uidl ) {
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
          //check for mails bigger mFilterOnServerCheckSize
          if ( (unsigned int)hids.value() >= mFilterOnServerCheckSize ) {
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
    emit percent( 100 );
    emit status( Running, i18n( "Adding remaining messages to the target folder." ) );
    mStage = ProcessRemainingMessages;
    processRemainingQueuedMessages();
  }
  else if ( mStage == ProcessRemainingMessages ) {

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

      // If the time-limited leave rule is checked, add the newer messages to
      // the list of messages to keep
      if ( Settings::leaveOnServerDays() > 0 && !mTimeOfNextSeenMsgsMap.isEmpty() ) {
        time_t timeLimit = time(0) - (86400 * Settings::leaveOnServerDays());
        for ( QSet<QByteArray>::const_iterator it = idsOfMsgsToDelete.constBegin();
              it != idsOfMsgsToDelete.constEnd(); ++it ) {
          time_t msgTime = mTimeOfNextSeenMsgsMap[ mUidForIdMap[*it] ];
          if ( msgTime >= timeLimit || msgTime == 0 ) {
            QPair<time_t, QByteArray> pair( msgTime, *it );
            idsToSave.append( pair );
          }
        }
      }

      // Otherwise, add all messages to the list of messages to keep - this may
      // be reduced in the following number-limited leave rule and size-limited
      // leave rule checks
      else {
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
      for ( int i = 0; i < idsToSave.count(); ++i ) {
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
      url.setPath( "/remove/" + ids );
    } else {
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
    mMsgsPendingDownload.erase( next );
  }
}

void POP3Resource::slotProcessPendingMsgs()
{
  // If we are in the processing stage and have nothing to process, advance to the
  // next stage immediately
  if ( msgsAwaitingProcessing.isEmpty() && mStage == ProcessRemainingMessages &&
       createJobsMap.isEmpty() ) {
    slotJobFinished();
    return;
  }

  // For each message in the list of messages to be processed, create an
  // ItemCreateJob that adds the message to the target collection.
  // When such a job is complete, slotItemCreateResult() is called.
  while ( !msgsAwaitingProcessing.isEmpty() ) {

    KMime::Message::Ptr msg = msgsAwaitingProcessing.dequeue();
    const QByteArray curId = msgIdsAwaitingProcessing.dequeue();
    const QByteArray curUid = msgUidsAwaitingProcessing.dequeue();

    Q_ASSERT( Collection( Settings::targetCollection() ).isValid() );

    Akonadi::Item item;
    item.setMimeType( "message/rfc822" );
    item.setPayload<KMime::Message::Ptr>( msg );
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

  // Make sure to use LF as line ending to make the processing easier
  // when piping through external programs
  KMime::Message::Ptr msg( new KMime::Message );
  msg->setContent( KMime::CRLFtoLF( curMsgData ) );
  msg->parse();

  /*if ( stage == Head ) {
  //FIXME:
    KMPopHeaders *header = mHeadersOnServer[ mHeaderIndex ];
    int size = mMsgsPendingDownload[ header->id() ];
    msg->setMsgLength( size );
    header->setHeader( msg );
    ++mHeaderIndex;
    slotGetNextHdr();
  } else*/ {
    //msg->setMsgLength( curMsgData.size() );
    msgsAwaitingProcessing.enqueue( msg );
    msgIdsAwaitingProcessing.enqueue( idsOfMsgs[indexOfCurrentMsg] );
    msgUidsAwaitingProcessing.enqueue( mUidForIdMap[ idsOfMsgs[indexOfCurrentMsg] ] );
    slotGetNextMsg();
  }
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

#endif

AKONADI_RESOURCE_MAIN( POP3Resource )


