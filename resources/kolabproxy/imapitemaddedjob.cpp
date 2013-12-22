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
#include "imapitemaddedjob.h"
#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemFetchScope>
#include <Akonadi/ItemModifyJob>
#include <Akonadi/ItemCreateJob>
#include <Akonadi/CollectionFetchJob>

ImapItemAddedJob::ImapItemAddedJob(const Akonadi::Item &imapItem, const Akonadi::Collection &imapCollection, KolabHandler &handler, QObject* parent)
    :KJob(parent),
    mHandler(handler),
    mImapItem(imapItem),
    mKolabCollection(imapToKolab(imapCollection)),
    mImapCollection(imapCollection)
{

}

void ImapItemAddedJob::start()
{
    //TODO: slow, would be nice if ItemCreateJob would work with a Collection
    //      having only the remoteId set
    Akonadi::CollectionFetchJob *job =
        new Akonadi::CollectionFetchJob(mKolabCollection, Akonadi::CollectionFetchJob::Base, this );
    connect( job, SIGNAL(result(KJob*)), this, SLOT(onCollectionFetchDone(KJob*)) );
}

void ImapItemAddedJob::onCollectionFetchDone( KJob *job )
{
    if ( job->error() ) {
        kWarning( ) << "Error on collection fetch:" << job->errorText();
        return;
    }
    Akonadi::Collection::List collections =
        qobject_cast<Akonadi::CollectionFetchJob*>(job)->collections();
    Q_ASSERT(collections.size() == 1);
    mKolabCollection = collections.first();

        const Akonadi::Item::List newItems = mHandler.translateItems(Akonadi::Item::List() << mImapItem);
        if (newItems.isEmpty()) {
            setError(KJob::UserDefinedError);
            setErrorText("Failed to translate item");
            emitResult();
            return;
        }
        mTranslatedItem = newItems.first();

        const QString &gid = mHandler.extractGid(mTranslatedItem);
        if (!gid.isEmpty()) {
            //We have a gid, let's see if this item already exists
            Akonadi::Item item;
            item.setGid(gid);
            Akonadi::ItemFetchJob *itemFetchJob = new Akonadi::ItemFetchJob(item);
            itemFetchJob->fetchScope().fetchFullPayload(false);
            itemFetchJob->fetchScope().setFetchModificationTime(false);
            itemFetchJob->fetchScope().setAncestorRetrieval(Akonadi::ItemFetchScope::Parent);
            connect(itemFetchJob, SIGNAL(result(KJob*)), this, SLOT(onItemFetchJobDone(KJob*)));
        } else {
            kDebug() << "no gid, creating directly";
            Akonadi::ItemCreateJob *cjob = new Akonadi::ItemCreateJob(mTranslatedItem, mKolabCollection);
            connect( cjob, SIGNAL(result(KJob*)), this, SLOT(itemCreatedDone(KJob*)) );
        }
    }

    Akonadi::Item ImapItemAddedJob::getTranslatedItem()
    {
        return Akonadi::Item();
    }

    void ImapItemAddedJob::onItemFetchJobDone(KJob *job)
    {
        Akonadi::ItemFetchJob * const fetchJob = static_cast<Akonadi::ItemFetchJob*>(job);

        Akonadi::Item::List conflictingItems;
        foreach (const Akonadi::Item &item, fetchJob->items()) {
            Q_ASSERT(item.parentCollection().isValid());
            //The same object may be in other collection, just not in this one.
            if (item.parentCollection().id() != mKolabCollection.id()) {
                continue;
            }
            conflictingItems << item;
        }
        if (conflictingItems.size() > 1) {
            kWarning() << "Multiple conflicting items detected in col " << mKolabCollection.id() << ", this should never happen: ";
            foreach (const Akonadi::Item &item, conflictingItems) {
                kWarning() << "Conflicting kolab item: " << item.id();
            }
        }
        if (!conflictingItems.isEmpty()) {
            //This is a conflict
            const Akonadi::Item conflictingKolabItem = conflictingItems.first();
            mTranslatedItem.setId(conflictingKolabItem.id());
            imapToKolab(mImapItem, mTranslatedItem);
            //TODO ensure the modifyjob doesn't collide with a removejob due to the original imap item vanishing.
            kDebug() << "conflict, modifying existing item: " << conflictingKolabItem.id();
            Akonadi::ItemModifyJob *modJob = new Akonadi::ItemModifyJob(mTranslatedItem, this);
            modJob->disableRevisionCheck();
            connect(modJob, SIGNAL(result(KJob*)), this, SLOT(itemCreatedDone(KJob*)));
        } else {
            kDebug() << "creating new item";
            Akonadi::ItemCreateJob *cjob = new Akonadi::ItemCreateJob(mTranslatedItem, mKolabCollection, this);
            connect(cjob, SIGNAL(result(KJob*)), this, SLOT(itemCreatedDone(KJob*)));
        }
}

void ImapItemAddedJob::itemCreatedDone(KJob *job)
{
    if (job->error()) {
        kWarning() << "Error on creating item:" << job->errorText();
        kWarning() << "imap item: " << mImapItem.id() << " in collection " << mImapCollection.id();
    }
    emitResult();
}
