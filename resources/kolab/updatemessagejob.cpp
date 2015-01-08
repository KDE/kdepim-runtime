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

#include "updatemessagejob.h"

#include <KIMAP/AppendJob>
#include <KIMAP/SearchJob>
#include <KIMAP/SelectJob>
#include <KIMAP/StoreJob>
#include <KIMAP/FetchJob>
#include <KIMAP/ImapSet>
#include <KDebug>
#include <KMime/Message>
#include <replacemessagejob.h>

#include "imapflags.h"

//Check if the expected uid message is still there => no modification, replace message.
//otherwise search for uptodate message by subject containing UID, merge contents, and replace message

UpdateMessageJob::UpdateMessageJob(const KMime::Message::Ptr &msg, KIMAP::Session *session, const QByteArray &kolabUid, QSharedPointer<Merger> merger, const QString &mailbox, qint64 uidNext, qint64 oldUid, QObject *parent)
    : KJob(parent),
    mSession(session),
    mMessage(msg),
    mMailbox(mailbox),
    mUidNext(uidNext),
    mOldUid(oldUid),
    mNewUid(-1),
    mMessageId(msg->messageID()->asUnicodeString().toUtf8()),
    mKolabUid(kolabUid),
    mMerger(merger)
{
}

void UpdateMessageJob::start()
{
    KIMAP::FetchJob * fetchJob = new KIMAP::FetchJob(mSession);

    fetchJob->setSequenceSet(KIMAP::ImapSet(mOldUid));
    fetchJob->setUidBased(true);

    KIMAP::FetchJob::FetchScope scope;
    scope.parts.clear();
    scope.mode = KIMAP::FetchJob::FetchScope::Headers;
    fetchJob->setScope(scope);

    connect(fetchJob, SIGNAL(headersReceived(QString,
                                          QMap<qint64,qint64>,
                                          QMap<qint64,qint64>,
                                          QMap<qint64,KIMAP::MessageAttribute>,
                                          QMap<qint64,KIMAP::MessageFlags>,
                                          QMap<qint64,KIMAP::MessagePtr>)),
            this, SLOT(onHeadersReceived(QString,
                                         QMap<qint64,qint64>,
                                         QMap<qint64,qint64>,
                                         QMap<qint64,KIMAP::MessageAttribute>,
                                         QMap<qint64,KIMAP::MessageFlags>,
                                         QMap<qint64,KIMAP::MessagePtr>)));
    connect(fetchJob, SIGNAL(result(KJob*)),
            this, SLOT(onHeadersFetchDone(KJob*)));
    fetchJob->start();


}

void UpdateMessageJob::onHeadersReceived(QString,
                                         QMap<qint64,qint64> uids,
                                         QMap<qint64,qint64>,
                                         QMap<qint64,KIMAP::MessageAttribute>,
                                         QMap<qint64,KIMAP::MessageFlags> flags,
                                         QMap<qint64,KIMAP::MessagePtr>)
{
    //Filter deleted messages
    foreach (qint64 number, uids.keys()) { //krazy:exclude=foreach
        // const KMime::Message::Ptr msg = messages[number];
        if (!flags[number].contains(ImapFlags::Deleted)) {
            mFoundUids << uids[number];
        }
    }
}

void UpdateMessageJob::onHeadersFetchDone(KJob *job)
{
    if (job->error()) {
        kWarning() << "Failed to fetch message: " << job->errorString();
    }
    if (mFoundUids.size() >= 1) {
        kDebug() << "Fast-forward update, replacing message.";
        appendMessage();
    } else {
        if (mSession->selectedMailBox() != mMailbox) {
            KIMAP::SelectJob *select = new KIMAP::SelectJob(mSession);
            select->setMailBox(mMailbox);
            connect(select, SIGNAL(result(KJob*)), this, SLOT(onSelectDone(KJob*)));
            select->start();
        } else {
            searchForLatestVersion();
        }
    }
}

void UpdateMessageJob::onSelectDone(KJob *job)
{
    if (job->error()) {
        kWarning() << job->errorString();
        setError(KJob::UserDefinedError);
        emitResult();
    } else {
        searchForLatestVersion();
    }
}

void UpdateMessageJob::searchForLatestVersion()
{
    KIMAP::SearchJob *search = new KIMAP::SearchJob(mSession);
    search->setUidBased(true);
    search->setSearchLogic(KIMAP::SearchJob::And);
    search->addSearchCriteria(KIMAP::SearchJob::Header, "Subject " + mKolabUid);
    connect(search, SIGNAL(result(KJob*)),
            this, SLOT(onSearchDone(KJob*)));
    search->start();
}

void UpdateMessageJob::onSearchDone(KJob *job)
{
    if (job->error()) {
        kWarning() << job->errorString();
        setError(KJob::UserDefinedError);
        emitResult();
        return;
    }

    KIMAP::SearchJob *search = static_cast<KIMAP::SearchJob*>(job);

    if (search->results().count() >= 1) {
        mOldUid = search->results().first();
        //TODO deal with all of them

        KIMAP::FetchJob * fetchJob = new KIMAP::FetchJob(mSession);
        fetchJob->setSequenceSet(KIMAP::ImapSet(mOldUid));
        fetchJob->setUidBased(true);

        KIMAP::FetchJob::FetchScope scope;
        scope.parts.clear();
        scope.mode = KIMAP::FetchJob::FetchScope::Full;
        fetchJob->setScope(scope);

        connect(fetchJob, SIGNAL(headersReceived(QString,
                                            QMap<qint64,qint64>,
                                            QMap<qint64,qint64>,
                                            QMap<qint64,KIMAP::MessageAttribute>,
                                            QMap<qint64,KIMAP::MessageFlags>,
                                            QMap<qint64,KIMAP::MessagePtr>)),
                this, SLOT(onConflictingMessagesReceived(QString,
                                            QMap<qint64,qint64>,
                                            QMap<qint64,qint64>,
                                            QMap<qint64,KIMAP::MessageAttribute>,
                                            QMap<qint64,KIMAP::MessageFlags>,
                                            QMap<qint64,KIMAP::MessagePtr>)));
        connect(fetchJob, SIGNAL(result(KJob*)),
                this, SLOT(onConflictingMessageFetchDone(KJob*)));
        fetchJob->start();
    } else {
        kWarning() << "failed to find latest version of object";
        appendMessage();
    }
}

void UpdateMessageJob::onConflictingMessagesReceived(QString,
                                         QMap<qint64,qint64> uids,
                                         QMap<qint64,qint64>,
                                         QMap<qint64,KIMAP::MessageAttribute>,
                                         QMap<qint64,KIMAP::MessageFlags> flags,
                                         QMap<qint64,KIMAP::MessagePtr> messages)
{
    foreach (qint64 number, uids.keys()) { //krazy:exclude=foreach
        if (!flags[number].contains(ImapFlags::Deleted)) {
            mMessagesToMerge << messages[number];
        }
    }
}

void UpdateMessageJob::onConflictingMessageFetchDone(KJob *job)
{
    if (job->error()) {
        kWarning() << "Failed to retrieve messages to merge: " << job->errorString();
    }
    mMessage = mMerger->merge(mMessage, mMessagesToMerge);
    appendMessage();
}

void UpdateMessageJob::appendMessage()
{
    const qint64 uidNext = -1;
    ReplaceMessageJob *replace = new ReplaceMessageJob(mMessage, mSession, mMailbox, uidNext, mOldUid, this);
    connect(replace, SIGNAL(result(KJob*)), this, SLOT(onReplaceDone(KJob*)));
    replace->start();
}

void UpdateMessageJob::onReplaceDone(KJob *job)
{
    if (job->error()) {
        kWarning() << job->errorString();
        setError(KJob::UserDefinedError);
        emitResult();
        return;
    }
    ReplaceMessageJob *replaceJob = static_cast<ReplaceMessageJob*>(job);
    mNewUid = replaceJob->newUid();
    emitResult();
}

qint64 UpdateMessageJob::newUid() const
{
    return mNewUid;
}

