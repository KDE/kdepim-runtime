/*
    Copyright (c) 2014 Christian Mollekopf <mollekopf@kolabsys.com>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#ifndef UPDATEMESSAGEJOB_H
#define UPDATEMESSAGEJOB_H

#include <KJob>
#include <KMime/Message>
#include <KIMAP/Session>
#include <KIMAP/FetchJob>

struct Merger {
    virtual ~Merger() {}
    virtual KMime::Message::Ptr merge(KMime::Message::Ptr newMessage, QList<KMime::Message::Ptr> conflictingMessages) const = 0;
};

/**
 * This job appends a message, marks the old one as deleted, and returns the uid of the appended message.
 */
class UpdateMessageJob : public KJob
{
    Q_OBJECT
public:
    UpdateMessageJob(const KMime::Message::Ptr &msg, KIMAP::Session *session, const QByteArray &kolabUid, QSharedPointer<Merger> merger, const QString &mailbox, qint64 uidNext = -1, qint64 oldUid = -1, QObject *parent = 0);

    qint64 newUid() const;

    void start();

private:
    void searchForLatestVersion();
    void appendMessage();

private Q_SLOTS:
    void onHeadersReceived(QString,
                           QMap<qint64, qint64> uids,
                           QMap<qint64, qint64>,
                           QMap<qint64, KIMAP::MessageAttribute>,
                           QMap<qint64, KIMAP::MessageFlags>,
                           QMap<qint64, KIMAP::MessagePtr>);
    void onHeadersFetchDone(KJob *job);
    void onSelectDone(KJob *job);
    void onSearchDone(KJob *job);
    void onConflictingMessagesReceived(QString,
                                       QMap<qint64, qint64> uids,
                                       QMap<qint64, qint64>,
                                       QMap<qint64, KIMAP::MessageAttribute>,
                                       QMap<qint64, KIMAP::MessageFlags>,
                                       QMap<qint64, KIMAP::MessagePtr>);
    void onConflictingMessageFetchDone(KJob *job);
    void onReplaceDone(KJob *job);

private:
    KIMAP::Session *mSession;
    KMime::Message::Ptr mMessage;
    const QString mMailbox;
    qint64 mUidNext;
    qint64 mOldUid;
    KIMAP::ImapSet mOldUids;
    qint64 mNewUid;
    const QByteArray mMessageId;
    const QByteArray mKolabUid;
    QList<qint64> mFoundUids;
    QList<KIMAP::MessagePtr> mMessagesToMerge;
    QSharedPointer<Merger> mMerger;
};

#endif

