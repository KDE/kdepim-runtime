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
#ifndef POP3RESOURCE_H
#define POP3RESOURCE_H

#include <Akonadi/ResourceBase>
#include <KMime/Message>
#include <KJob>

namespace Akonadi {
class ItemCreateJob;
}
namespace KWallet {
class Wallet;
}

class POPSession;
class QTimer;

class POP3Resource : public Akonadi::ResourceBase,
                     public Akonadi::AgentBase::Observer
{
  Q_OBJECT

  public:
    POP3Resource( const QString &id );
    ~POP3Resource();

  public Q_SLOTS:
    virtual void configure( WId windowId );

  protected Q_SLOTS:
    void retrieveCollections();
    void retrieveItems( const Akonadi::Collection &col );
    bool retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts );

  protected:

    virtual void aboutToQuit();

  private Q_SLOTS:

    void slotAbortRequested();
    void intervalCheckTriggered();
    void configurationChanged();

    // Error unrelated to a state
    void slotSessionError( int errorCode, const QString &errorMessage );

    // For state FetchTargetCollection
    void targetCollectionFetchJobFinished( KJob *job );
    void localFolderRequestJobFinished( KJob *job );

    // For state Precommand
    void precommandResult( KJob *job );

    // For state RequestPassword
    void walletOpenedForLoading( bool success );

    // For state Login
    void loginJobResult( KJob *job );

    // For state List
    void listJobResult( KJob *job );

    // For state UIDList
    void uidListJobResult( KJob *job );

    // For state Download
    void messageFinished( int messageId, KMime::Message::Ptr message );
    void fetchJobResult( KJob *job );
    void messageDownloadProgress( KJob *job, KJob::Unit unit, qulonglong totalBytes );

    // For state Save
    void itemCreateJobResult( KJob *job );

    // For state Delete
    void deleteJobResult( KJob *job );

    // For state Quit
    void quitJobResult( KJob *job );

    // For state SavePassword
    void walletOpenedForSaving( bool success );

  private:

    enum State {
      Idle,
      FetchTargetCollection,
      Precommand,
      RequestPassword,
      Connect,
      Login,
      List,
      UIDList,
      Download,
      Save,
      Delete,
      Quit,
      SavePassword
    };

    void resetState();
    void doStateStep();
    void advanceState( State nextState );
    void cancelSync( const QString &errorMessage, bool error = true );
    void saveSeenUIDList();
    QList<int> idsToDelete() const;
    int idToTime( int id ) const;
    int idOfOldestMessage( QList<int> &idList ) const;
    void startMailCheck();
    void updateIntervalTimer();
    void showPasswordDialog( const QString &queryText );
    QString buildLabelForPasswordDialog( const QString &detailedError ) const;
    void finish();

    State mState;
    Akonadi::Collection mTargetCollection;
    POPSession *mPopSession;
    bool mAskAgain;
    QTimer *mIntervalTimer;
    bool mIntervalCheckInProgress;
    QString mPassword;
    bool mSavePassword;
    bool mTestLocalInbox;
    KWallet::Wallet *mWallet;

    // Maps IDs on the server to message sizes on the server
    QMap<int,int> mIdsToSizeMap;

    // Maps IDs on the server to UIDs on the server.
    // This can be empty, if the server doesn't support UIDL
    QMap<int,QString> mIdsToUidsMap;

    // Whether we actually received a valid UID list from the server
    bool mUidListValid;

    // IDs of messages that we have successfully downloaded. This does _not_ mean
    // that the messages corresponding to the IDs are stored in Akonadi yet
    QList<int> mDownloadedIDs;

    // IDs of messages that we want to download and that we have started the
    // FetchJob with. After the FetchJob, this should be empty, except if there
    // was some error
    QList<int> mIdsToDownload;

    // After downloading a message, we store it in Akonadi by using an ItemCreateJob.
    // This map stores the currently running ItemCreateJob's and their corresponding
    // POP3 IDs.
    // When an ItemCreateJob finished, it is removed from this map.
    // The Save state waits until this map becomes empty.
    QMap<Akonadi::ItemCreateJob*,int> mPendingCreateJobs;

    // List of message IDs that were successfully stored in Akonadi
    QList<int> mIDsStored;

    // List of message IDs that were successfully deleted
    QList<int> mDeletedIDs;
};

#endif
