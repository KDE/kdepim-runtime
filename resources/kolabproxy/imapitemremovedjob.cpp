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
#include "imapitemremovedjob.h"
#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemFetchScope>
#include <Akonadi/ItemModifyJob>
#include <Akonadi/ItemCreateJob>
#include <Akonadi/CollectionFetchJob>
#include <Akonadi/ItemDeleteJob>

ImapItemRemovedJob::ImapItemRemovedJob(const Akonadi::Item& imapItem, QObject* parent)
    :KJob(parent),
    mImapItem(imapItem),
    mKolabItem(imapToKolab(imapItem))
{

}

void ImapItemRemovedJob::start()
{
    Akonadi::ItemFetchJob *fetchJob = new Akonadi::ItemFetchJob(mKolabItem, this);
    fetchJob->fetchScope().setFetchRemoteIdentification(true);
    fetchJob->fetchScope().setCacheOnly(true); //don't access the imap backend again
    connect(fetchJob, SIGNAL(result(KJob*)), this, SLOT(onItemFetchJobDone(KJob*)));
}

void ImapItemRemovedJob::onItemFetchJobDone(KJob* job)
{
    //This fetch job can fail if the kolab item has already been removed, so we ignore errors
    Akonadi::ItemFetchJob *fetchJob = static_cast<Akonadi::ItemFetchJob*>(job);
    Q_ASSERT(fetchJob->items().size() <= 1);
    if (fetchJob->items().isEmpty()) {
        emitResult();
        return;
    }
    const Akonadi::Item kolabItem = fetchJob->items().first();
    //Check if the currently relevant imap item was removed or just an outdated copy
    if (kolabItem.remoteId().toLongLong() == mImapItem.id()) {
        Akonadi::ItemDeleteJob *deleteJob = new Akonadi::ItemDeleteJob(kolabItem, this );
        connect(deleteJob, SIGNAL(result(KJob*)), this, SLOT(onDeleteDone(KJob*)));
    } else {
        emitResult();
    }
}

void ImapItemRemovedJob::onDeleteDone(KJob* job)
{
    if (job->error()) {
        kWarning() << "Delete job failed" << job->errorString();
    }
    emitResult();
}




