/*
    Copyright (c) 2014 Klarälvdalens Datakonsult AB,
                       a KDAB Group company <info@kdab.com>
    Author: Kevin Krammer <kevin.krammer@kdab.com>

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

#include "kolabaddtagtask.h"
#include "kolabresource_debug.h"
#include "../imap/uidnextattribute.h"

#include "pimkolab/kolabformat/kolabobject.h"

#include <kimap/appendjob.h>
#include <kimap/imapset.h>
#include <kimap/searchjob.h>
#include <kimap/selectjob.h>
#include <kimap/session.h>

#include <KLocalizedString>

KolabAddTagTask::KolabAddTagTask(const ResourceStateInterface::Ptr &resource, QObject *parent)
    : KolabRelationResourceTask(resource, parent)
{
}

void KolabAddTagTask::startRelationTask(KIMAP::Session *session)
{
    qCDebug(KOLABRESOURCE_LOG) << "converted tag";

    const QLatin1String productId("Akonadi-Kolab-Resource");
    const KMime::Message::Ptr message = Kolab::KolabObjectWriter::writeTag(resourceState()->tag(), QStringList(), Kolab::KolabV3, productId);
    mMessageId = message->messageID()->asUnicodeString().toUtf8();

    KIMAP::AppendJob *job = new KIMAP::AppendJob(session);
    job->setMailBox(mailBoxForCollection(relationCollection()));
    job->setContent(message->encodedContent(true));
    job->setInternalDate(message->date()->dateTime());
    connect(job, &KJob::result, this, &KolabAddTagTask::onAppendMessageDone);
    job->start();
}

void KolabAddTagTask::applyFoundUid(qint64 uid)
{
    Akonadi::Tag tag = resourceState()->tag();

    //If we failed to get the remoteid the tag remains local only
    if (uid > 0) {
        tag.setRemoteId(QByteArray::number(uid));
    }

    qCDebug(KOLABRESOURCE_LOG) << "committing new tag";
    changeCommitted(tag);

    Akonadi::Collection c = relationCollection();

    // Get the current uid next value and store it
    UidNextAttribute *uidAttr = nullptr;
    int oldNextUid = 0;
    if (c.hasAttribute("uidnext")) {
        uidAttr = static_cast<UidNextAttribute *>(c.attribute("uidnext"));
        oldNextUid = uidAttr->uidNext();
    }

    // If the uid we just got back is the expected next one of the box
    // then update the property to the probable next uid to keep the cache in sync.
    // If not something happened in our back, so we don't update and a refetch will
    // happen at some point.
    if (uid == oldNextUid) {
        if (uidAttr == nullptr) {
            uidAttr = new UidNextAttribute(uid + 1);
            c.addAttribute(uidAttr);
        } else {
            uidAttr->setUidNext(uid + 1);
        }

        applyCollectionChanges(c);
    }
}

void KolabAddTagTask::triggerSearchJob(KIMAP::Session *session)
{
    KIMAP::SearchJob *search = new KIMAP::SearchJob(session);

    search->setUidBased(true);

    if (!mMessageId.isEmpty()) {
        search->setTerm(KIMAP::Term(QStringLiteral("Message-ID"), QString::fromLatin1(mMessageId)));
    } else {
        UidNextAttribute *uidNext = relationCollection().attribute<UidNextAttribute>();
        if (!uidNext) {
            cancelTask(i18n("Could not determine the UID for the newly created message on the server"));
            search->deleteLater();
            return;
        }
        KIMAP::ImapInterval interval(uidNext->uidNext());

        search->setTerm(KIMAP::Term(KIMAP::Term::And, {
            KIMAP::Term(KIMAP::Term::New),
            KIMAP::Term(KIMAP::Term::Uid,
                        KIMAP::ImapSet(uidNext->uidNext(), 0))
        }));
    }

    connect(search, &KJob::result,
            this, &KolabAddTagTask::onSearchDone);

    search->start();
}

void KolabAddTagTask::onAppendMessageDone(KJob *job)
{
    KIMAP::AppendJob *append = qobject_cast<KIMAP::AppendJob *>(job);

    if (append->error()) {
        qCWarning(KOLABRESOURCE_LOG) << append->errorString();
        cancelTask(append->errorString());
        return;
    }

    qint64 uid = append->uid();
    qCDebug(KOLABRESOURCE_LOG) << "appended message with uid: " << uid;

    if (uid > 0) {
        // We got it directly if UIDPLUS is supported...
        applyFoundUid(uid);
    } else {
        // ... otherwise prepare searching for the message
        KIMAP::Session *session = append->session();
        const QString mailBox = append->mailBox();

        if (session->selectedMailBox() != mailBox) {
            KIMAP::SelectJob *select = new KIMAP::SelectJob(session);
            select->setMailBox(mailBox);

            connect(select, &KJob::result,
                    this, &KolabAddTagTask::onPreSearchSelectDone);

            select->start();
        } else {
            triggerSearchJob(session);
        }
    }
}

void KolabAddTagTask::onPreSearchSelectDone(KJob *job)
{
    if (job->error()) {
        qCWarning(KOLABRESOURCE_LOG) << job->errorString();
        cancelTask(job->errorString());
    } else {
        KIMAP::SelectJob *select = static_cast<KIMAP::SelectJob *>(job);
        triggerSearchJob(select->session());
    }
}

void KolabAddTagTask::onSearchDone(KJob *job)
{
    if (job->error()) {
        qCWarning(KOLABRESOURCE_LOG) << job->errorString();
        cancelTask(job->errorString());
        return;
    }

    KIMAP::SearchJob *search = static_cast<KIMAP::SearchJob *>(job);

    qint64 uid = 0;
    if (search->results().count() == 1) {
        uid = search->results().at(0);
    }

    applyFoundUid(uid);
}
