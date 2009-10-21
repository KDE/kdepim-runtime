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
#include <akonadi/kmime/specialcollectionsrequestjob.h>
#include <akonadi/kmime/specialcollections.h>
#include <Mailtransport/PrecommandJob>

#include <KIO/PasswordDialog>
#include <KMessageBox>

using namespace Akonadi;
using namespace MailTransport;

POP3Resource::POP3Resource( const QString &id )
    : ResourceBase( id ),
      mState( Idle ),
      mPopSession( 0 ),
      mAskAgain( false ),
      mIntervalTimer( new QTimer( this ) )
{
  new SettingsAdaptor( Settings::self() );

  QDBusConnection::sessionBus().registerObject( QLatin1String( "/Settings" ),
                            Settings::self(), QDBusConnection::ExportAdaptors );
  connect( this, SIGNAL(abortRequested()),
           this, SLOT(slotAbortRequested()) );
  connect( mIntervalTimer, SIGNAL(timeout()),
           this, SLOT(intervalCheckTriggered()) );
}

POP3Resource::~POP3Resource()
{
  Settings::self()->writeConfig();
}

void POP3Resource::updateIntervalTimer()
{
  if ( Settings::intervalCheckEnabled() && mState == Idle ) {
    mIntervalTimer->start( Settings::intervalCheckInterval() * 1000 * 60 );
  }
  else {
    mIntervalTimer->stop();
  }
}

void POP3Resource::intervalCheckTriggered()
{
  kDebug() << "Starting interval mail check.";
  Q_ASSERT( mState == Idle );
  startMailCheck();
  mIntervalCheckInProgress = true;
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
    updateIntervalTimer();
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
        SpecialCollectionsRequestJob *requestJob = new SpecialCollectionsRequestJob( this );
        requestJob->requestDefaultCollection( SpecialCollections::Inbox );
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
  case Precommand:
    {
      kDebug() << "================ Starting state Precommand =====================";
      if ( !Settings::precommand().isEmpty() ) {
        PrecommandJob *precommandJob = new PrecommandJob( Settings::precommand(), this );
        connect( precommandJob, SIGNAL(result(KJob*)),
                 this, SLOT(precommandResult(KJob*)) );
        precommandJob->start();
        emit status( Running, i18n( "Executing precommand..." ) );
      }
      else {
        mState = Connect;
        doStateStep();
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
      QList<int> idsToKill = idsToDelete();
      if ( !idsToKill.isEmpty() ) {
        emit status( Running, i18n( "Deleting messages from the server.") );
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

  mTargetCollection = SpecialCollections::self()->defaultCollection( SpecialCollections::Inbox );
  Q_ASSERT( mTargetCollection.isValid() );
  mState = Precommand;
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
    mState = Precommand;
    doStateStep();
  }
}

void POP3Resource::precommandResult( KJob *job )
{
  if ( job->error() ) {
    cancelSync( i18n( "Error while executing precommand." ) +
                '\n' + job->errorString() );
    return;
  }
  else {
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

    mUidListValid = !mIdsToUidsMap.isEmpty() || mIdsToSizeMap.isEmpty();
    if ( Settings::leaveOnServer() && !mUidListValid ) {
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
  if ( !mIntervalCheckInProgress )
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
  QList<QString> seenUIDs = Settings::seenUidList();
  QList<int> timeOfSeenUIDs = Settings::seenUidTimeList();

  //
  // Find the messages that we have sucessfully stored, but did not actually get
  // deleted.
  // Those messages, we have to remember, so we don't download them again.
  //
  QList<int> idsOfMessagesDownloadedButNotDeleted = mIDsStored;
  foreach( int deletedId, mDeletedIDs )
    idsOfMessagesDownloadedButNotDeleted.removeAll( deletedId );
  QList<QString> uidsOfMessagesDownloadedButNotDeleted;
  foreach( int id, idsOfMessagesDownloadedButNotDeleted ) {
    QString uid = mIdsToUidsMap.value( id );
    if ( !uid.isEmpty() ) {
      uidsOfMessagesDownloadedButNotDeleted.append( uid );
    }
  }
  Q_ASSERT( seenUIDs.size() == timeOfSeenUIDs.size() );
  foreach( const QString uid, uidsOfMessagesDownloadedButNotDeleted ) {
    if ( !seenUIDs.contains( uid ) ) {
      seenUIDs.append( uid );
      timeOfSeenUIDs.append( time( 0 ) );
    }
  }

  //
  // If we have a valid UID list from the server, delete those UIDs that are in
  // the seenUidList but are not on the server.
  // This can happen if someone else than this resource deleted the mails from the
  // server which we kept here.
  //
  if ( mUidListValid ) {
    QList<QString>::iterator uidIt = seenUIDs.begin();
    QList<int>::iterator timeIt = timeOfSeenUIDs.begin();
    while ( uidIt != seenUIDs.end() ) {
      const QString curSeenUID = *uidIt;
      if ( !mIdsToUidsMap.values().contains( curSeenUID ) ) {
        // Ok, we have a UID in the seen UID list that is not anymore on the server.
        // Therefore remove it from the seen UID list, it is not needed there anymore,
        // it just wastes space.
        uidIt = seenUIDs.erase( uidIt );
        timeIt = timeOfSeenUIDs.erase( timeIt );
      }
      else {
        uidIt++;
        timeIt++;
      }
    }
  }
  else
    kWarning() << "UID list from server is not valid.";


  //
  // Now save it in the settings
  //
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
  mUidListValid = false;
  mIntervalCheckInProgress = false;
  updateIntervalTimer();

  if ( mPopSession ) {
    // Closing the POP session means the KIO slave will get disconnected, which
    // automatically issues the QUIT command.
    // Delete the POP session later, otherwise the scheduler makes us crash
    mPopSession->abortCurrentJob();
    mPopSession->deleteLater();
    mPopSession = 0;
  }
}

void POP3Resource::startMailCheck()
{
  resetState();
  mIntervalTimer->stop();
  emit percent( 0 ); // Otherwise the value from the last sync is taken
  mState = FetchTargetCollection;
  doStateStep();
}

void POP3Resource::retrieveCollections()
{
  if ( mState == Idle ) {
    startMailCheck();
  }
  else {
    cancelSync(
        i18n( "Mail check already in progress, unable to start a second check." ) );
  }
}

AKONADI_RESOURCE_MAIN( POP3Resource )
