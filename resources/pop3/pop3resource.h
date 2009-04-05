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

#include <akonadi/resourcebase.h>

#include <kio/global.h>
#include <KUrl>

#include <QPointer>
#include <QTimer>
#include <QQueue>

#include <boost/shared_ptr.hpp>

namespace KIO {
class SimpleJob;
class Slave;
class Job;
}

namespace KMime {
class Message;
}

namespace Akonadi {
class ItemCreateJob;
}

typedef boost::shared_ptr<KMime::Message> MessagePtr;

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

    void slotSlaveError( KIO::Slave*, int, const QString & );
    void slotData( KIO::Job* job, const QByteArray &data );
    void slotResult( KJob* );
    void slotJobFinished();
    void slotGetNextMsg();
    void slotProcessPendingMsgs();
    void slotMsgRetrieved( KJob*, const QString &infoMsg, const QString & );
    void slotAbortRequested();
    void slotCancel();
    void slotItemCreateResult( KJob* job );

  protected:

    const KUrl getUrl() const;
    void connectJob();
    KIO::MetaData slaveConfig() const;

    virtual void aboutToQuit();
    virtual void itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection );
    virtual void itemChanged( const Akonadi::Item &item, const QSet<QByteArray> &parts );
    virtual void itemRemoved( const Akonadi::Item &item );

  private:

    typedef QPair<QByteArray, QByteArray> IdUidPair;
    enum Stage { Idle, List, Uidl, Head, Retr, ProcessRemainingMessages, Dele, Quit };

    QMap<Akonadi::ItemCreateJob*, IdUidPair> createJobsMap;
    unsigned short int defaultPort() const;
    QString protocol() const;
    void processRemainingQueuedMessages();
    void saveUidList();
    void startJob();
    void cancelMailCheck();
    
    KIO::SimpleJob *mJob;
    QPointer<KIO::Slave> mSlave;
    Stage mStage;

    QTimer processMsgsTimer;
    int numBytesRead, numMsgBytesRead, curMsgLen, numBytes, numBytesToRead, numMsgs;
    bool mAskAgain;
    QList<QByteArray> idsOfMsgs;
    int indexOfCurrentMsg;
    int processingDelay;
    bool headers;
    bool interactive;
    bool mUidlFinished;
    QDataStream *curMsgStrm;
    QByteArray curMsgData;
    int dataCounter;
    int mTargetCollectionId;
    // Map of ID's vs. sizes of messages which should be downloaded; use QMap because the order needs to be
    // predictable
    QMap<QByteArray, int> mMsgsPendingDownload;
    // All IDs that we'll delete in any case, even if we have "leave on server"
    // checked. This is only used when the server issues an invalid UIDL response
    // and we can't map a UID to the ID. See bug 127696.
    QSet<QByteArray> idsOfForcedDeletes;

    QQueue<MessagePtr> msgsAwaitingProcessing;
    QQueue<QByteArray> msgIdsAwaitingProcessing;
    QQueue<QByteArray> msgUidsAwaitingProcessing;

    QHash<QByteArray, QByteArray> mUidForIdMap; // maps message ID (i.e. index on the server) to UID
    QHash<QByteArray, int> mUidsOfSeenMsgsDict; // set of UIDs of previously seen messages (for fast lookup)
    QHash<QByteArray, int> mUidsOfNextSeenMsgsDict; // set of UIDs of seen messages (for the next check)
    QVector<int> mTimeOfSeenMsgsVector; // list of times of previously seen messages
    QHash<QByteArray, int> mTimeOfNextSeenMsgsMap; // map of uid to times of seen messages
    QHash<QByteArray, int> mSizeOfNextSeenMsgsDict;
    QSet<QByteArray> idsOfMsgsToDelete;

    QSet<QByteArray> mHeaderDeleteUids;
    QSet<QByteArray> mHeaderDownUids;
    QSet<QByteArray> mHeaderLaterUids;
};

#endif
