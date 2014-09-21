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

#include "itemchangedjob.h"
#include "itemaddedjob.h"
#include <Akonadi/ItemFetchJob>
#include <Akonadi/CollectionFetchJob>
#include <Akonadi/ItemModifyJob>
#include <klocale.h>

ItemChangedJob::ItemChangedJob(const Akonadi::Item& kolabItem, HandlerManager& handler, QObject* parent)
:   KJob(parent),
    mHandlerManager(handler),
    mKolabItem(kolabItem)
{

}

void ItemChangedJob::start()
{
    Akonadi::CollectionFetchJob *collectionFetchJob = new Akonadi::CollectionFetchJob(Akonadi::Collection(mKolabItem.storageCollectionId()), Akonadi::CollectionFetchJob::Base);
    connect(collectionFetchJob, SIGNAL(result(KJob*)), this, SLOT(onKolabCollectionFetched(KJob*)));
}

void ItemChangedJob::onKolabCollectionFetched(KJob* job)
{
    Akonadi::CollectionFetchJob *fetchjob = static_cast<Akonadi::CollectionFetchJob*>(job);
    if (job->error() || fetchjob->collections().isEmpty()) {
        kWarning() << "collection fetch job failed " << job->errorString() << fetchjob->collections().isEmpty();
        setError(KJob::UserDefinedError);
        setErrorText(job->errorText());
        emitResult();
        return;
    }
    const Akonadi::Collection imapCollection = kolabToImap(fetchjob->collections().first());
    mHandler = mHandlerManager.getHandler(imapCollection.id());
    if (!mHandler) {
        kWarning() << "Couldn't find a handler for the collection, but we should have one: " << imapCollection.id();
        setError(KJob::UserDefinedError);
        setErrorText(job->errorText());
        emitResult();
        return;
    }

    Akonadi::ItemFetchJob *itemFetchJob = new Akonadi::ItemFetchJob(kolabToImap(mKolabItem), this);
    connect(itemFetchJob, SIGNAL(result(KJob*)), SLOT(onImapItemFetchDone(KJob*)));
}

void ItemChangedJob::onImapItemFetchDone(KJob* job)
{
    if ( job->error() ) {
        kWarning() << job->error() << job->errorString();
        setError(KJob::UserDefinedError);
        setErrorText(job->errorText());
        emitResult();
        return;
    }

    Akonadi::ItemFetchJob *fetchJob = qobject_cast<Akonadi::ItemFetchJob*>(job);
    if (fetchJob->items().isEmpty()) { //The corresponding imap item hasn't been created yet
        kDebug() << "item is not yet created in imap resource";
        Akonadi::CollectionFetchJob *fetch =
        new Akonadi::CollectionFetchJob( Akonadi::Collection( mKolabItem.storageCollectionId() ),
                                        Akonadi::CollectionFetchJob::Base, this );
        connect( fetch, SIGNAL(result(KJob*)), SLOT(onCollectionFetchDone(KJob*)) );
    } else {
        kDebug() << "item is in imap resource";
        Akonadi::Item imapItem = fetchJob->items().first();

        if (!mHandler->toKolabFormat(mKolabItem, imapItem)) {
            kWarning() << "Failed to convert item to kolab format: " << mKolabItem.id();
            setError(KJob::UserDefinedError);
            setErrorText(i18n("Failed to convert item %1 to kolab format", mKolabItem.id()));
            emitResult();
            return;
        }
        Akonadi::ItemModifyJob *mjob = new Akonadi::ItemModifyJob(imapItem);
        connect(mjob, SIGNAL(result(KJob*)), SLOT(onItemModifyDone(KJob*)));
    }
}

void ItemChangedJob::onCollectionFetchDone(KJob *job)
{
    Akonadi::CollectionFetchJob *fetchJob = static_cast<Akonadi::CollectionFetchJob*>(job);
    if ( job->error() || fetchJob->collections().isEmpty() ) {
        kWarning() << "Collection fetch job failed" << fetchJob->errorString();
        setError(KJob::UserDefinedError);
        setErrorText(job->errorText());
        emitResult();
        return;
    }

    const Akonadi::Collection kolabCollection = fetchJob->collections().first();
    ItemAddedJob *itemAddedJob = new ItemAddedJob(mKolabItem, kolabCollection, *mHandler, this);
    connect(itemAddedJob, SIGNAL(result(KJob*)), SLOT(onItemAddedDone(KJob*)) );
    itemAddedJob->start();
}

void ItemChangedJob::onItemAddedDone(KJob* job)
{
    if (job->error()) {
        setError(KJob::UserDefinedError);
        setErrorText(job->errorText());
    }
    emitResult();
}

void ItemChangedJob::onItemModifyDone(KJob *job)
{
    if (job->error()) {
        setError(KJob::UserDefinedError);
        setErrorText(job->errorText());
    }
    emitResult();
}

Akonadi::Item ItemChangedJob::item() const
{
    return mKolabItem;
}
