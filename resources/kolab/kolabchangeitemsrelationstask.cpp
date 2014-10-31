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

#include "kolabchangeitemsrelationstask.h"

#include <imapflags.h>
#include <kolabobject.h>

#include <kimap/appendjob.h>
#include <kimap/imapset.h>
#include <kimap/selectjob.h>
#include <kimap/session.h>
#include <kimap/storejob.h>

#include <akonadi/relationfetchjob.h>

KolabChangeItemsRelationsTask::KolabChangeItemsRelationsTask(ResourceStateInterface::Ptr resource, QObject *parent)
    : KolabRelationResourceTask(resource, parent)
{
}

void KolabChangeItemsRelationsTask::startRelationTask(KIMAP::Session *session)
{
    mSession = session;
    mAddedRelations = resourceState()->addedRelations();
    mRemovedRelations = resourceState()->removedRelations();

    processNextRelation();
}

void KolabChangeItemsRelationsTask::processNextRelation()
{
    Akonadi::Relation relation;
    if (!mAddedRelations.isEmpty()) {
        relation = mAddedRelations.takeFirst();
        mAdding = true;
    } else if (!mRemovedRelations.isEmpty()) {
        relation = mRemovedRelations.takeFirst();
        mAdding = false;
    } else {
        changeProcessed();
        return;
    }

    //We have to fetch it again in case it changed since the notification was emitted (which is likely)
    //Otherwise we get an empty remoteid for new tags that were immediately applied on an item
    Akonadi::RelationFetchJob *fetch = new Akonadi::RelationFetchJob(relation);
    connect(fetch, SIGNAL(result(KJob*)), this, SLOT(onRelationFetchDone(KJob*)));
}

void KolabChangeItemsRelationsTask::onRelationFetchDone(KJob *job)
{
    if (job->error()) {
        kWarning() << "RelatonFetch failed: " << job->errorString();
        processNextRelation();
        return;
    }

    const Akonadi::Relation::List relations = static_cast<Akonadi::RelationFetchJob *>(job)->relations();
    Q_ASSERT(relations.size() == 1);
    if (relations.size() != 1) {
        kWarning() << "Invalid number of relations retrieved: " << relations.size();
        processNextRelation();
        return;
    }

    Akonadi::Relation relation = relations.first();
    if (mAdding) {
        addRelation(relation);
    } else {
        removeRelation(relation);
    }
}

void KolabChangeItemsRelationsTask::addRelation(const Akonadi::Relation &relation)
{
    const QLatin1String productId("Akonadi-Kolab-Resource");
    const KMime::Message::Ptr message = Kolab::KolabObjectWriter::writeRelation(relation, Kolab::KolabV3, productId);

    KIMAP::AppendJob *job = new KIMAP::AppendJob(mSession);
    job->setMailBox(mailBoxForCollection(relationCollection()));
    job->setContent(message->encodedContent(true));
    job->setInternalDate(message->date()->dateTime());
    connect(job, SIGNAL(result(KJob*)), SLOT(onChangeCommitted(KJob*)));
}

void KolabChangeItemsRelationsTask::removeRelation(const Akonadi::Relation &relation)
{
    mCurrentRelation = relation;
    const QString mailBox = mailBoxForCollection(relationCollection());

    if (mSession->selectedMailBox() != mailBox) {
        KIMAP::SelectJob *select = new KIMAP::SelectJob(mSession);
        select->setMailBox(mailBox);

        connect(select, SIGNAL(result(KJob*)), this, SLOT(onSelectDone(KJob*)));

        select->start();
    } else {
        triggerStoreJob();
    }
}

void KolabChangeItemsRelationsTask::onSelectDone(KJob *job)
{
    if (job->error()) {
        kWarning() << "Failed to select mailbox: " << job->errorString();
        cancelTask(job->errorString());
    } else {
        triggerStoreJob();
    }
}

void KolabChangeItemsRelationsTask::triggerStoreJob()
{
    KIMAP::ImapSet set;
    set.add(mCurrentRelation.remoteId().toLong());

    KIMAP::StoreJob *store = new KIMAP::StoreJob(mSession);
    store->setUidBased(true);
    store->setSequenceSet(set);
    store->setFlags(QList<QByteArray>() << ImapFlags::Deleted);
    store->setMode(KIMAP::StoreJob::AppendFlags);
    connect(store, SIGNAL(result(KJob*)), SLOT(onChangeCommitted(KJob*)));
    store->start();
}

void KolabChangeItemsRelationsTask::onChangeCommitted(KJob *job)
{
    if (job->error()) {
        cancelTask(job->errorString());
    } else {
        processNextRelation();
    }
}

