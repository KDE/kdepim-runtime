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
#include <AkonadiCore/ItemFetchJob>
#include <AkonadiCore/CollectionFetchJob>
#include <AkonadiCore/ItemModifyJob>
#include <KLocalizedString>

ItemChangedJob::ItemChangedJob(const Akonadi::Item& kolabItem, HandlerManager& handler, QObject* parent)
:   KJob(parent),
    mHandlerManager(handler),
    mKolabItem(kolabItem)
{

}

void ItemChangedJob::start()
{
    Akonadi::CollectionFetchJob *collectionFetchJob = new Akonadi::CollectionFetchJob(Akonadi::Collection(mKolabItem.storageCollectionId()), Akonadi::CollectionFetchJob::Base);
    connect(collectionFetchJob, &Akonadi::CollectionFetchJob::result, this, &ItemChangedJob::onKolabCollectionFetched);
}

void ItemChangedJob::onKolabCollectionFetched(KJob* job)
{
    Akonadi::CollectionFetchJob *fetchjob = static_cast<Akonadi::CollectionFetchJob*>(job);
    if (job->error() || fetchjob->collections().isEmpty()) {
        qWarning() << "collection fetch job failed " << job->errorString() << fetchjob->collections().isEmpty();
        setError(KJob::UserDefinedError);
        setErrorText(job->errorText());
        emitResult();
        return;
    }
    const Akonadi::Collection imapCollection = kolabToImap(fetchjob->collections().first());
    mHandler = mHandlerManager.getHandler(imapCollection.id());
    if (!mHandler) {
        qWarning() << "Couldn't find a handler for the collection, but we should have one: " << imapCollection.id();
        setError(KJob::UserDefinedError);
        setErrorText(job->errorText());
        emitResult();
        return;
    }

    Akonadi::ItemFetchJob *itemFetchJob = new Akonadi::ItemFetchJob(kolabToImap(mKolabItem), this);
    connect(itemFetchJob, &Akonadi::ItemFetchJob::result, this, &ItemChangedJob::onImapItemFetchDone);
}

void ItemChangedJob::onImapItemFetchDone(KJob* job)
{
    if ( job->error() ) {
        qWarning() << job->error() << job->errorString();
        setError(KJob::UserDefinedError);
        setErrorText(job->errorText());
        emitResult();
        return;
    }

    Akonadi::ItemFetchJob *fetchJob = qobject_cast<Akonadi::ItemFetchJob*>(job);
    if (fetchJob->items().isEmpty()) { //The corresponding imap item hasn't been created yet
        qDebug() << "item is not yet created in imap resource";
        Akonadi::CollectionFetchJob *fetch =
        new Akonadi::CollectionFetchJob( Akonadi::Collection( mKolabItem.storageCollectionId() ),
                                        Akonadi::CollectionFetchJob::Base, this );
        connect(fetch, &Akonadi::CollectionFetchJob::result, this, &ItemChangedJob::onCollectionFetchDone);
    } else {
        qDebug() << "item is in imap resource";
        Akonadi::Item imapItem = fetchJob->items().first();

        if (!mHandler->toKolabFormat(mKolabItem, imapItem)) {
            qWarning() << "Failed to convert item to Kolab format: " << mKolabItem.id();
            setError(KJob::UserDefinedError);
            setErrorText(i18n("Failed to convert item %1 to Kolab format", mKolabItem.id()));
            emitResult();
            return;
        }
        Akonadi::ItemModifyJob *mjob = new Akonadi::ItemModifyJob(imapItem);
        connect(mjob, &Akonadi::ItemModifyJob::result, this, &ItemChangedJob::onItemModifyDone);
    }
}

void ItemChangedJob::onCollectionFetchDone(KJob *job)
{
    Akonadi::CollectionFetchJob *fetchJob = static_cast<Akonadi::CollectionFetchJob*>(job);
    if ( job->error() || fetchJob->collections().isEmpty() ) {
        qWarning() << "Collection fetch job failed" << fetchJob->errorString();
        setError(KJob::UserDefinedError);
        setErrorText(job->errorText());
        emitResult();
        return;
    }

    const Akonadi::Collection kolabCollection = fetchJob->collections().first();
    ItemAddedJob *itemAddedJob = new ItemAddedJob(mKolabItem, kolabCollection, *mHandler, this);
    connect(itemAddedJob, &ItemAddedJob::result, this, &ItemChangedJob::onItemAddedDone);
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
