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

#ifndef REPLACEMESSAGEJOB_H
#define REPLACEMESSAGEJOB_H

#include <KJob>
#include <KMime/Message>
#include <KIMAP/Session>
#include <KIMAP/ImapSet>

/**
 * This job appends a message, marks the old one as deleted, and returns the uid of the appended message.
 */
class ReplaceMessageJob : public KJob
{
    Q_OBJECT
public:
    ReplaceMessageJob(const KMime::Message::Ptr &msg, KIMAP::Session *session, const QString &mailbox, qint64 uidNext = -1, const KIMAP::ImapSet &oldUids = KIMAP::ImapSet(), QObject *parent = nullptr);

    qint64 newUid() const;

    void start() override;

private:
    void triggerSearchJob();
    void triggerDeleteJobIfNecessary();

private Q_SLOTS:
    void onAppendMessageDone(KJob *job);
    void onSelectDone(KJob *job);
    void onSearchDone(KJob *job);
    void onDeleteDone(KJob *job);

private:
    KIMAP::Session *mSession = nullptr;
    const KMime::Message::Ptr mMessage;
    const QString mMailbox;
    qint64 mUidNext;
    KIMAP::ImapSet mOldUids;
    qint64 mNewUid;
    const QByteArray mMessageId;
};

#endif
