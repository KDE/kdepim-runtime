/*
    Copyright (c) 2013 Christian Mollekopf <mollekopf@kolabsys.com>

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

#include "gidmigrationjob.h"
#include <akonadi/collectionfetchjob.h>
#include <akonadi/collectionfetchscope.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemmodifyjob.h>
#include <akonadi/itemfetchscope.h>

using namespace Akonadi;

UpdateJob::UpdateJob(const Collection &col, QObject *parent)
:   Job(parent),
    mCollection(col),
    mModJobRunning(false)
{}

UpdateJob::~UpdateJob()
{}

void UpdateJob::doStart()
{
    ItemFetchJob *fetchJob = new ItemFetchJob(mCollection, this);
    fetchJob->fetchScope().setCacheOnly(true);
    fetchJob->fetchScope().setIgnoreRetrievalErrors(true);
    fetchJob->fetchScope().setFetchModificationTime(false);
    fetchJob->fetchScope().setFetchRemoteIdentification(false);
    fetchJob->fetchScope().fetchFullPayload(true);
    //Limit scope to envelope only for mail
    connect(fetchJob, SIGNAL(itemsReceived(Akonadi::Item::List)), this, SLOT(itemsReceived(Akonadi::Item::List)));
}

void UpdateJob::itemsReceived(const Akonadi::Item::List &items)
{
    //We're queuing items rather than creating ItemModifyJobs directly due to memory concerns
    //I'm not sure if that would indeed be a problem (a ModifyJob shouldn't be much larger than the item) but we'd have to compare memory usage first when creating large amounts of ItemModifyJobs.
    foreach (const Akonadi::Item &item, items) {
        mItemQueue.enqueue(item);
    }
    processNext();
}

void UpdateJob::slotResult(KJob *job)
{
    //This slot is automatically called for all subjobs by KCompositeJob
    //FIXME the fetch job emits result before itemsReceived, because itemsReceived is triggered using the result signal (which is wrong IMO). See ItemFetchJob::timeout
    //If result was emitted at the end we could avoid having to call processNext in itemsReceived and locking it.
    ItemFetchJob * const fetchJob = dynamic_cast<ItemFetchJob*>(job);
    const bool fetchReturnedNoItems = fetchJob && fetchJob->items().isEmpty();
    Job::slotResult(job);
    if (fetchReturnedNoItems) {
        emitResult();
    } else if (!fetchJob) {
        mModJobRunning = false;
        if (!hasSubjobs()) {
            if (!processNext()) {
                emitResult();
            }
        }
    }
}

bool UpdateJob::processNext()
{
    if (mModJobRunning || mItemQueue.isEmpty()) {
        return false;
    }
    const Akonadi::Item &item = mItemQueue.dequeue();
    //Only the single item modifyjob updates the gid
    ItemModifyJob *modJob = new ItemModifyJob(item, this);
    modJob->setUpdateGid(true);
    modJob->setIgnorePayload(true);
    mModJobRunning = true;
    return true;
}


GidMigrationJob::GidMigrationJob(const QStringList &mimeTypeFilter, QObject *parent)
:   Job(parent),
    mMimeTypeFilter(mimeTypeFilter)
{
}

GidMigrationJob::~GidMigrationJob()
{
}

void GidMigrationJob::doStart()
{
    CollectionFetchJob *fetchJob = new CollectionFetchJob(Collection::root(), CollectionFetchJob::Recursive, this);
    fetchJob->fetchScope().setContentMimeTypes(mMimeTypeFilter);
    connect(fetchJob, SIGNAL(collectionsReceived(Akonadi::Collection::List)), this, SLOT(collectionsReceived(Akonadi::Collection::List)));
    connect(fetchJob, SIGNAL(result(KJob*)), this, SLOT(collectionsFetched(KJob*)));
}

void GidMigrationJob::collectionsReceived(const Collection::List &collections)
{
    mCollections << collections;
}

void GidMigrationJob::collectionsFetched( KJob *job )
{
    //Errors are propagated by KCompositeJob
    if (!job->error()) {
        processCollection();
    }
}

void GidMigrationJob::processCollection()
{
    if (mCollections.isEmpty()) {
        emitResult();
        return;
    }
    const Collection col = mCollections.takeLast();
    UpdateJob *updateJob = new UpdateJob(col, this);
    connect(updateJob, SIGNAL(result(KJob*)), this, SLOT(itemsUpdated(KJob*)));
}

void GidMigrationJob::itemsUpdated(KJob *job)
{
    //Errors are propagated by KCompositeJob
    if (!job->error()) {
        processCollection();
    }
}
