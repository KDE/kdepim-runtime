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

#include "kolabretrievetagstask.h"

#include "tagchangehelper.h"

#include <kimap/selectjob.h>
#include <kimap/fetchjob.h>
#include <kolabobject.h>

KolabRetrieveTagTask::KolabRetrieveTagTask(ResourceStateInterface::Ptr resource, QObject *parent)
    : KolabRelationResourceTask(resource, parent)
    , mSession(0)
{
}

void KolabRetrieveTagTask::startRelationTask(KIMAP::Session *session)
{
    mSession = session;
    const QString mailBox = mailBoxForCollection(relationCollection());

    KIMAP::SelectJob *select = new KIMAP::SelectJob(session);
    select->setMailBox(mailBox);
    connect( select, SIGNAL(result(KJob*)),
            this, SLOT(onFinalSelectDone(KJob*)) );
    select->start();
}

void KolabRetrieveTagTask::onFinalSelectDone(KJob *job)
{
    if (job->error()) {
        kWarning() << job->errorString();
        cancelTask(job->errorString());
        return;
    }

    KIMAP::SelectJob *select = static_cast<KIMAP::SelectJob*>(job);
    KIMAP::FetchJob *fetch = new KIMAP::FetchJob(select->session());

    if (select->messageCount() == 0) {
        resourceState()->tagsRetrieved(mTags, mTagMembers);
        deleteLater();
        return;
    }

    KIMAP::ImapSet set;
    set.add(KIMAP::ImapInterval(1, 0));
    fetch->setSequenceSet(set);
    fetch->setUidBased(false);

    KIMAP::FetchJob::FetchScope scope;
    scope.parts.clear();
    scope.mode = KIMAP::FetchJob::FetchScope::Full;
    fetch->setScope(scope);

    connect(fetch, SIGNAL(headersReceived(QString,
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
    connect(fetch, SIGNAL(result(KJob*)),
            this, SLOT(onHeadersFetchDone(KJob*)));
    fetch->start();
}

void KolabRetrieveTagTask::onHeadersReceived(const QString &mailBox,
                                     const QMap<qint64, qint64> &uids,
                                     const QMap<qint64, qint64> &sizes,
                                     const QMap<qint64, KIMAP::MessageAttribute> &attrs,
                                     const QMap<qint64, KIMAP::MessageFlags> &flags,
                                     const QMap<qint64, KIMAP::MessagePtr> &messages)
{
    KIMAP::FetchJob *fetch = static_cast<KIMAP::FetchJob*>( sender() );
    Q_ASSERT(fetch);

    foreach (qint64 number, uids.keys()) { //krazy:exclude=foreach
        const KMime::Message::Ptr msg = messages[number];
        const Kolab::KolabObjectReader reader(msg);
        switch (reader.getType()) {
            case Kolab::RelationConfigurationObject:
                if (reader.isTag()) {
                    extractTag(reader, uids[number]);
                } else if (reader.isRelation()) {
                    extractRelation(reader, uids[number]);
                }
                break;

            default:
                break;
        }
    }
}

void KolabRetrieveTagTask::extractTag(const Kolab::KolabObjectReader &reader, qint64 remoteUid)
{
    Akonadi::Tag tag = reader.getTag();
    tag.setRemoteId(QByteArray::number(remoteUid));
    mTags << tag;

    Akonadi::Item::List members;
    Q_FOREACH (const QString &memberUrl, reader.getTagMembers()) {
        Kolab::RelationMember member = Kolab::parseMemberUrl(memberUrl);
        //TODO should we create a dummy item if it isn't yet available?
        Akonadi::Item i;
        if (!member.gid.isEmpty()) {
            //Reference by GID
            i.setGid(member.gid);
        } else {
            //Reference by imap uid
            if (member.uid < 0) {
                kWarning() << "Failed to parse uid: " << memberUrl;
                continue;
            }
            i.setRemoteId(QString::number(member.uid));
            kDebug() << "got member: " << member.uid << member.mailbox;
            Akonadi::Collection parent;
            {
                //The root collection is not part of the mailbox path
                Akonadi::Collection col;
                col.setRemoteId(rootRemoteId());
                col.setParentCollection(Akonadi::Collection::root());
                parent = col;
            }
            Q_FOREACH(const QByteArray part, member.mailbox) {
                Akonadi::Collection col;
                col.setRemoteId(separatorCharacter() + QString::fromLatin1(part));
                col.setParentCollection(parent);
                parent = col;
            }
            i.setParentCollection(parent);
        }
        //TODO implement fallback to search if uid is not available
        members << i;
    }
    mTagMembers.insert(QString::fromLatin1(tag.remoteId()), members);
}

void KolabRetrieveTagTask::extractRelation(const Kolab::KolabObjectReader &reader, qint64 remoteUid)
{
    Akonadi::Relation relation = reader.getRelation();
    relation.setRemoteId(QByteArray::number(remoteUid));
    mRelations << relation;
}

void KolabRetrieveTagTask::onHeadersFetchDone(KJob *job)
{
    if (job->error()) {
        kWarning() << "Fetch job failed " << job->errorString();
        cancelTask(job->errorString());
        return;
    }

    if (!mTags.isEmpty() || !mTagMembers.isEmpty()) {
        kDebug() << "Fetched tags: " << mTags.size() << mTagMembers.keys().size();
        resourceState()->tagsRetrieved(mTags, mTagMembers);
    }

    if (!mRelations.isEmpty()) {
        kDebug() << "Fetched relations:" << mRelations.size();
        resourceState()->relationsRetrieved(mRelations);
    }

    deleteLater();
}

