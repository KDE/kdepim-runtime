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
#include "revertitemchangesjob.h"
#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemFetchScope>
#include <Akonadi/ItemModifyJob>
#include <Akonadi/ItemDeleteJob>

RevertItemChangesJob::RevertItemChangesJob(const Akonadi::Item& kolabItem, HandlerManager &handlerManager, QObject* parent)
    :KJob(parent),
    mHandlerManager(handlerManager),
    mKolabItem(kolabItem)
{

}

void RevertItemChangesJob::start()
{
    Akonadi::ItemFetchJob *itemFetchJob = new Akonadi::ItemFetchJob(kolabToImap(mKolabItem), this);
    itemFetchJob->fetchScope().fetchFullPayload();
    itemFetchJob->fetchScope().setAncestorRetrieval(Akonadi::ItemFetchScope::Parent);
    connect(itemFetchJob, SIGNAL(result(KJob*)), SLOT(onImapItemFetchDone(KJob*)));
}

void RevertItemChangesJob::onImapItemFetchDone(KJob* job)
{
    if ( job->error() ) {
        setError(KJob::UserDefinedError);
        setErrorText(job->errorText());
        emitResult();
        return;
    }

    Akonadi::ItemFetchJob *fetchJob = qobject_cast<Akonadi::ItemFetchJob*>(job);
    if (fetchJob->items().isEmpty()) { //The corresponding imap item hasn't been created yet
        kDebug() << "item is not yet created in imap resource, deleting the kolab item";
        Akonadi::ItemDeleteJob *deleteJob = new Akonadi::ItemDeleteJob(mKolabItem, this);
        connect(deleteJob, SIGNAL(result(KJob*)), SLOT(onItemModifyDone(KJob*)));
    } else {
        kDebug() << "reverting to state of imap item";
        Akonadi::Item imapItem = fetchJob->items().first();
        const KolabHandler::Ptr handler = mHandlerManager.getHandler(imapItem.parentCollection().id());
        if (!handler) {
            kWarning() << "No handler: " << imapItem.parentCollection().id();
            setError(KJob::UserDefinedError);
            emitResult();
            return;
        }
        const Akonadi::Item::List translatedItems = handler->translateItems(fetchJob->items());

        if (translatedItems.isEmpty()) {
            kWarning() << "Failed to reload item: " << mKolabItem.id();
            setError(KJob::UserDefinedError);
            emitResult();
            return;
        }
        Akonadi::Item kolabItem = translatedItems.first();
        kolabItem.setId(mKolabItem.id());
        Akonadi::ItemModifyJob *mjob = new Akonadi::ItemModifyJob(kolabItem);
        connect(mjob, SIGNAL(result(KJob*)), SLOT(onItemModifyDone(KJob*)));
    }
}

void RevertItemChangesJob::onItemModifyDone(KJob *job)
{
    if (job->error()) {
        setError(KJob::UserDefinedError);
        setErrorText(job->errorText());
    }
    emitResult();
}
