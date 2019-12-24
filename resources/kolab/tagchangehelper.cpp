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

#include "tagchangehelper.h"
#include "kolabresource_debug.h"
#include "kolabrelationresourcetask.h"
#include "kolabhelpers.h"
#include "updatemessagejob.h"

#include <imapflags.h>
#include <uidnextattribute.h>

#include "pimkolab/kolabformat/kolabobject.h"

#include <kimap/appendjob.h>
#include <kimap/searchjob.h>
#include <kimap/selectjob.h>
#include <kimap/session.h>
#include <kimap/storejob.h>
#include <replacemessagejob.h>
#include <AkonadiCore/TagModifyJob>

KMime::Message::Ptr TagConverter::createMessage(const Akonadi::Tag &tag, const Akonadi::Item::List &items, const QString &username)
{
    QStringList itemRemoteIds;
    itemRemoteIds.reserve(items.count());
    for (const Akonadi::Item &item : items) {
        const QString memberUrl = KolabHelpers::createMemberUrl(item, username);
        if (!memberUrl.isEmpty()) {
            itemRemoteIds << memberUrl;
        }
    }

    // save message to the server.
    const QLatin1String productId("Akonadi-Kolab-Resource");
    const KMime::Message::Ptr message = Kolab::KolabObjectWriter::writeTag(tag, itemRemoteIds, Kolab::KolabV3, productId);
    return message;
}

struct TagMerger : public Merger {
    ~TagMerger() override
    {
    }

    KMime::Message::Ptr merge(const KMime::Message::Ptr &newMessage, const QList<KMime::Message::Ptr> &conflictingMessages) const override
    {
        qCDebug(KOLABRESOURCE_LOG) << "Got " << conflictingMessages.size() << " conflicting relation configuration objects. Overwriting with local version.";
        return newMessage;
    }
};

TagChangeHelper::TagChangeHelper(KolabRelationResourceTask *parent)
    : QObject(parent)
    , mTask(parent)
{
}

void TagChangeHelper::start(const Akonadi::Tag &tag, const KMime::Message::Ptr &message, KIMAP::Session *session)
{
    Q_ASSERT(tag.isValid());
    const QString mailBox = mTask->mailBoxForCollection(mTask->relationCollection());
    const qint64 oldUid = tag.remoteId().toLongLong();
    qCDebug(KOLABRESOURCE_LOG) << mailBox << oldUid;

    const qint64 uidNext = -1;

    UpdateMessageJob *append = new UpdateMessageJob(message, session, tag.gid(), QSharedPointer<TagMerger>(new TagMerger), mailBox, uidNext, oldUid, this);
    connect(append, &KJob::result, this, &TagChangeHelper::onReplaceDone);
    append->setProperty("tag", QVariant::fromValue(tag));
    append->start();
}

void TagChangeHelper::recordNewUid(qint64 newUid, const Akonadi::Tag &tag)
{
    Q_ASSERT(newUid > 0);
    Q_ASSERT(tag.isValid());

    const QByteArray remoteId = QByteArray::number(newUid);
    qCDebug(KOLABRESOURCE_LOG) << "Setting remote ID to " << remoteId << " on tag with local id: " << tag.id();
    //Make sure we only update the id and send nothing else
    Akonadi::Tag updateTag;
    updateTag.setId(tag.id());
    updateTag.setRemoteId(remoteId);
    Akonadi::TagModifyJob *modJob = new Akonadi::TagModifyJob(updateTag);
    connect(modJob, &KJob::result, this, &TagChangeHelper::onModifyDone);
}

void TagChangeHelper::onReplaceDone(KJob *job)
{
    if (job->error()) {
        qCWarning(KOLABRESOURCE_LOG) << "Replace failed: " << job->errorString();
    }
    UpdateMessageJob *replaceJob = static_cast<UpdateMessageJob *>(job);
    const qint64 newUid = replaceJob->newUid();
    const Akonadi::Tag tag = job->property("tag").value<Akonadi::Tag>();
    if (newUid > 0) {
        recordNewUid(newUid, tag);
    } else {
        Q_EMIT cancelTask(job->errorString());
    }
}

void TagChangeHelper::onModifyDone(KJob *job)
{
    if (job->error()) {
        qCWarning(KOLABRESOURCE_LOG) << "Modify failed: " << job->errorString();
        Q_EMIT cancelTask(job->errorString());
        return;
    }
    Q_EMIT changeCommitted();
}
