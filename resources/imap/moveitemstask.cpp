/*
    Copyright (c) 2010 Klarälvdalens Datakonsult AB,
                       a KDAB Group company <info@kdab.com>
    Author: Kevin Ottens <kevin@kdab.com>

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

#include "moveitemstask.h"

#include <QtCore/QUuid>

#include "imapresource_debug.h"
#include <KLocalizedString>

#include <kimap/copyjob.h>
#include <kimap/searchjob.h>
#include <kimap/selectjob.h>
#include <kimap/session.h>
#include <kimap/storejob.h>
#include <kimap/movejob.h>

#include <kmime/kmime_message.h>

#include "imapflags.h"
#include "uidnextattribute.h"

MoveItemsTask::MoveItemsTask(const ResourceStateInterface::Ptr &resource, QObject *parent)
    : ResourceTask(DeferIfNoSession, resource, parent)
{

}

MoveItemsTask::~MoveItemsTask()
{
}

void MoveItemsTask::doStart(KIMAP::Session *session)
{
    if (item().remoteId().isEmpty()) {
        qCWarning(IMAPRESOURCE_LOG) << "Failed: messages has no rid";
        emitError(i18n("Cannot move message, it does not exist on the server."));
        changeProcessed();
        return;
    }

    if (sourceCollection().remoteId().isEmpty()) {
        qCWarning(IMAPRESOURCE_LOG) << "Failed: source collection has no rid";
        emitError(i18n("Cannot move message out of '%1', '%1' does not exist on the server.",
                       sourceCollection().name()));
        changeProcessed();
        return;
    }

    if (targetCollection().remoteId().isEmpty()) {
        qCWarning(IMAPRESOURCE_LOG) << "Failed: target collection has no rid";
        emitError(i18n("Cannot move message to '%1', '%1' does not exist on the server.",
                       targetCollection().name()));
        changeProcessed();
        return;
    }

    const QString oldMailBox = mailBoxForCollection(sourceCollection());
    const QString newMailBox = mailBoxForCollection(targetCollection());

    if (oldMailBox == newMailBox) {
        qCDebug(IMAPRESOURCE_LOG) << "Nothing to do, same mailbox";
        changeProcessed();
        return;
    }

    if (session->selectedMailBox() != oldMailBox) {
        KIMAP::SelectJob *select = new KIMAP::SelectJob(session);

        select->setMailBox(oldMailBox);
        connect(select, &KIMAP::SelectJob::result, this, &MoveItemsTask::onSelectDone);

        select->start();
    } else {
        startMove(session);
    }
}

void MoveItemsTask::onSelectDone(KJob *job)
{
    if (job->error()) {
        qCWarning(IMAPRESOURCE_LOG) << "Select failed: " << job->errorString();
        cancelTask(job->errorString());

    } else {
        KIMAP::SelectJob *select = static_cast<KIMAP::SelectJob *>(job);
        startMove(select->session());
    }
}

void MoveItemsTask::startMove(KIMAP::Session *session)
{
    const QString newMailBox = mailBoxForCollection(targetCollection());

    KIMAP::ImapSet set;

    // save message id, might be needed later to search for the
    // resulting message uid.
    foreach (const Akonadi::Item &item, items()) {
        try {
            KMime::Message::Ptr msg = item.payload<KMime::Message::Ptr>();
            const QByteArray messageId = msg->messageID()->asUnicodeString().toUtf8();
            if (!messageId.isEmpty()) {
                m_messageIds.insert(item.id(), messageId);
            }

            set.add(item.remoteId().toLong());
        } catch (const Akonadi::PayloadException &e) {
            qCWarning(IMAPRESOURCE_LOG) << "Move failed, payload exception " << item.id() << item.remoteId();
            cancelTask(i18n("Failed to move item, it has no message payload. Remote id: %1", item.remoteId()));
            return;
        }
    }

    const bool canMove = serverCapabilities().contains(QStringLiteral("MOVE"), Qt::CaseInsensitive);
    if (canMove) {
        auto job = new KIMAP::MoveJob(session);
        job->setUidBased(true);
        job->setSequenceSet(set);
        job->setMailBox(newMailBox);
        connect(job, &KIMAP::Job::result, this, &MoveItemsTask::onMoveDone);
        job->start();
    } else {
        auto job = new KIMAP::CopyJob(session);
        job->setUidBased(true);
        job->setSequenceSet(set);
        job->setMailBox(newMailBox);
        connect(job, &KIMAP::Job ::result, this, &MoveItemsTask::onCopyDone);
        job->start();
    }
    m_oldSet = set;
}

void MoveItemsTask::onCopyDone(KJob *job)
{
    if (job->error()) {
        qCWarning(IMAPRESOURCE_LOG) << job->errorString();
        cancelTask(job->errorString());

    } else {
        KIMAP::CopyJob *copy = static_cast<KIMAP::CopyJob *>(job);

        m_newUids = imapSetToList(copy->resultingUids());

        // Mark the old one ready for deletion
        KIMAP::StoreJob *store = new KIMAP::StoreJob(copy->session());

        store->setUidBased(true);
        store->setSequenceSet(m_oldSet);
        store->setFlags(QList<QByteArray>() << ImapFlags::Deleted);
        store->setMode(KIMAP::StoreJob::AppendFlags);

        connect(store, &KIMAP::StoreJob::result, this, &MoveItemsTask::onStoreFlagsDone);

        store->start();
    }
}

void MoveItemsTask::onMoveDone(KJob *job)
{
    if (job->error()) {
        qCWarning(IMAPRESOURCE_LOG) << job->errorString();
        cancelTask(job->errorString());
    } else {
        KIMAP::MoveJob *move = static_cast<KIMAP::MoveJob *>(job);

        m_newUids = imapSetToList(move->resultingUids());

        if (!m_newUids.isEmpty()) {
            recordNewUid();
        } else {
            // Let's go for a search to find a new UID :-)

            // We did a move, so we're very likely not in the right mailbox
            KIMAP::SelectJob *select = new KIMAP::SelectJob(move->session());
            select->setMailBox(mailBoxForCollection(targetCollection()));
            connect(select, &KIMAP::SelectJob::result, this, &MoveItemsTask::onPreSearchSelectDone);
            select->start();
        }
    }
}

void MoveItemsTask::onStoreFlagsDone(KJob *job)
{
    if (job->error()) {
        qCWarning(IMAPRESOURCE_LOG) << "Failed to mark message as deleted on source server: " << job->errorString();
        emitWarning(i18n("Failed to mark the message from '%1' for deletion on the IMAP server. "
                         "It will reappear on next sync.",
                         sourceCollection().name()));
    }

    if (!m_newUids.isEmpty()) {
        recordNewUid();
    } else {
        // Let's go for a search to find the new UID :-)

        // We did a copy we're very likely not in the right mailbox
        KIMAP::StoreJob *store = static_cast<KIMAP::StoreJob *>(job);
        KIMAP::SelectJob *select = new KIMAP::SelectJob(store->session());
        select->setMailBox(mailBoxForCollection(targetCollection()));

        connect(select, &KIMAP::SelectJob::result, this, &MoveItemsTask::onPreSearchSelectDone);

        select->start();
    }
}

void MoveItemsTask::onPreSearchSelectDone(KJob *job)
{
    if (job->error()) {
        qCWarning(IMAPRESOURCE_LOG) << "Select failed: " << job->errorString();
        cancelTask(job->errorString());
        return;
    }

    KIMAP::SelectJob *select = static_cast<KIMAP::SelectJob *>(job);
    KIMAP::SearchJob *search = new KIMAP::SearchJob(select->session());

    search->setUidBased(true);

    if (!m_messageIds.isEmpty()) {
        QVector<KIMAP::Term> subterms;
        subterms.reserve(m_messageIds.size());
        foreach (const QByteArray &messageId, m_messageIds) {
            QByteArray header = "Message-ID ";
            header += messageId;
            subterms << KIMAP::Term(QStringLiteral("Message-ID"), QString::fromLatin1(messageId));
        }
        search->setTerm(KIMAP::Term(KIMAP::Term::Or, subterms));
    } else {
        Akonadi::Collection c = targetCollection();
        UidNextAttribute *uidNext = c.attribute<UidNextAttribute>();
        if (!uidNext) {
            cancelTask(i18n("Could not determine the UID for the newly created message on the server"));
            search->deleteLater();
            return;
        }
        search->setTerm(KIMAP::Term(KIMAP::Term::And, {
            KIMAP::Term(KIMAP::Term::New),
            KIMAP::Term(KIMAP::Term::Uid,
            KIMAP::ImapSet(uidNext->uidNext(), 0))
        }));
    }

    connect(search, &KIMAP::SearchJob::result, this, &MoveItemsTask::onSearchDone);

    search->start();
}

void MoveItemsTask::onSearchDone(KJob *job)
{
    if (job->error()) {
        qCWarning(IMAPRESOURCE_LOG) << "Search failed: " << job->errorString();
        cancelTask(job->errorString());
        return;
    }

    KIMAP::SearchJob *search = static_cast<KIMAP::SearchJob *>(job);
    m_newUids = search->results();

    recordNewUid();
}

void MoveItemsTask::recordNewUid()
{
    // Create the item resulting of the operation, since at that point
    // the first part of the move succeeded
    QVector<qint64> oldUids = imapSetToList(m_oldSet);

    Akonadi::Item::List newItems;
    for (int i = 0; i < oldUids.count(); ++i) {
        const QString oldUid = QString::number(oldUids.at(i));
        Akonadi::Item item;
        Q_FOREACH (const Akonadi::Item &it, items()) {
            if (it.remoteId() == oldUid) {
                item = it;
                break;
            }
        }
        Q_ASSERT(item.isValid());

        // Update the item content with the new UID from the copy
        // if we didn't manage to get a valid UID from the server, use a random RID instead
        // this will make ItemSync clean up the mess during the next sync (while empty RIDs are protected as not yet existing on the server)
        if (m_newUids.count() <= i) {
            item.setRemoteId(QUuid::createUuid().toString());
        } else {
            item.setRemoteId(QString::number(m_newUids.at(i)));
        }
        newItems << item;
    }

    changesCommitted(newItems);

    Akonadi::Collection c = targetCollection();

    // Get the current uid next value and store it
    UidNextAttribute *uidAttr = Q_NULLPTR;
    int oldNextUid = 0;
    if (c.hasAttribute("uidnext")) {
        uidAttr = static_cast<UidNextAttribute *>(c.attribute("uidnext"));
        oldNextUid = uidAttr->uidNext();
    }

    // If the uid we just got back is the expected next one of the box
    // then update the property to the probable next uid to keep the cache in sync.
    // If not something happened in our back, so we don't update and a refetch will
    // happen at some point.
    if (!m_newUids.isEmpty() && m_newUids.last() == oldNextUid) {
        if (uidAttr == Q_NULLPTR) {
            uidAttr = new UidNextAttribute(m_newUids.last() + 1);
            c.addAttribute(uidAttr);
        } else {
            uidAttr->setUidNext(m_newUids.last() + 1);
        }

        applyCollectionChanges(c);
    }
}

QVector<qint64> MoveItemsTask::imapSetToList(const KIMAP::ImapSet &set)
{
    QVector<qint64> list;
    foreach (const KIMAP::ImapInterval &interval, set.intervals()) {
        for (qint64 i = interval.begin(); i <= interval.end(); ++i) {
            list << i;
        }
    }

    return list;
}

