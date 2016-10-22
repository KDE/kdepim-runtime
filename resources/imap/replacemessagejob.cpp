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

#include "replacemessagejob.h"

#include <KIMAP/AppendJob>
#include <KIMAP/SearchJob>
#include <KIMAP/SelectJob>
#include <KIMAP/StoreJob>
#include <KIMAP/ImapSet>
#include "imapresource_debug.h"
#include <KMime/Message>

#include "imapflags.h"

ReplaceMessageJob::ReplaceMessageJob(const KMime::Message::Ptr &msg, KIMAP::Session *session, const QString &mailbox, qint64 uidNext, KIMAP::ImapSet oldUids, QObject *parent)
    : KJob(parent),
      mSession(session),
      mMessage(msg),
      mMailbox(mailbox),
      mUidNext(uidNext),
      mOldUids(oldUids),
      mNewUid(-1),
      mMessageId(msg->messageID()->asUnicodeString().toUtf8())
{
}

void ReplaceMessageJob::start()
{
    KIMAP::AppendJob *job = new KIMAP::AppendJob(mSession);
    job->setMailBox(mMailbox);
    job->setContent(mMessage->encodedContent(true));
    job->setInternalDate(mMessage->date()->dateTime());
    connect(job, &KJob::result, this, &ReplaceMessageJob::onAppendMessageDone);
    job->start();
}

void ReplaceMessageJob::onAppendMessageDone(KJob *job)
{
    KIMAP::AppendJob *append = qobject_cast<KIMAP::AppendJob *>(job);

    if (append->error()) {
        qCWarning(IMAPRESOURCE_LOG) << append->errorString();
        setError(KJob::UserDefinedError);
        emitResult();
        return;
    }

    // We get it directly if UIDPLUS is supported...
    mNewUid = append->uid();

    if (mNewUid > 0 && mOldUids.isEmpty()) {
        //We have the uid an no message to delete, we're done
        emitResult();
        return;
    }

    if (mSession->selectedMailBox() != mMailbox) {
        //For search and delete we need to select the right mailbox first
        KIMAP::SelectJob *select = new KIMAP::SelectJob(mSession);
        select->setMailBox(mMailbox);
        connect(select, &KJob::result, this, &ReplaceMessageJob::onSelectDone);
        select->start();
    } else {
        if (mNewUid > 0) {
            triggerDeleteJobIfNecessary();
        } else {
            triggerSearchJob();
        }
    }
}

void ReplaceMessageJob::onSelectDone(KJob *job)
{
    if (job->error()) {
        qCWarning(IMAPRESOURCE_LOG) << job->errorString();
        setError(KJob::UserDefinedError);
        emitResult();
    } else {
        if (mNewUid > 0) {
            triggerDeleteJobIfNecessary();
        } else {
            triggerSearchJob();
        }
    }
}

void ReplaceMessageJob::triggerSearchJob()
{
    KIMAP::SearchJob *search = new KIMAP::SearchJob(mSession);

    search->setUidBased(true);

    if (!mMessageId.isEmpty()) {
        search->setTerm(KIMAP::Term(QStringLiteral("Message-ID"), QString::fromLatin1(mMessageId)));
    } else {
        if (mUidNext < 0) {
            qCWarning(IMAPRESOURCE_LOG) << "Could not determine the UID for the newly created message on the server";
            search->deleteLater();
            setError(KJob::UserDefinedError);
            emitResult();
            return;
        }
        search->setTerm(KIMAP::Term(KIMAP::Term::And,
                                    { KIMAP::Term(KIMAP::Term::New),
                                      KIMAP::Term(KIMAP::Term::Uid,
                                                  KIMAP::ImapSet(mUidNext, 0))
                                    }));
    }

    connect(search, &KJob::result,
            this, &ReplaceMessageJob::onSearchDone);

    search->start();
}

void ReplaceMessageJob::onSearchDone(KJob *job)
{
    if (job->error()) {
        qCWarning(IMAPRESOURCE_LOG) << job->errorString();
        setError(KJob::UserDefinedError);
        emitResult();
        return;
    }

    KIMAP::SearchJob *search = static_cast<KIMAP::SearchJob *>(job);

    if (search->results().count() == 1) {
        mNewUid = search->results().at(0);
    } else {
        qCWarning(IMAPRESOURCE_LOG) << "Failed to find uid for message. Got 0 or too many results: " << search->results().count();
        setError(KJob::UserDefinedError);
        emitResult();
        return;
    }
    triggerDeleteJobIfNecessary();
}

void ReplaceMessageJob::triggerDeleteJobIfNecessary()
{
    if (mOldUids.isEmpty()) {
        //Nothing to do, we're done
        emitResult();
    } else {
        KIMAP::StoreJob *store = new KIMAP::StoreJob(mSession);
        store->setUidBased(true);
        store->setSequenceSet(mOldUids);
        store->setFlags(QList<QByteArray>() << ImapFlags::Deleted);
        store->setMode(KIMAP::StoreJob::AppendFlags);
        connect(store, &KJob::result, this, &ReplaceMessageJob::onDeleteDone);
        store->start();
    }
}

void ReplaceMessageJob::onDeleteDone(KJob *job)
{
    if (job->error()) {
        qCWarning(IMAPRESOURCE_LOG) << job->errorString();
    }
    emitResult();
}

qint64 ReplaceMessageJob::newUid() const
{
    return mNewUid;
}
