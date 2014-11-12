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

#include "kolabrelationresourcetask.h"

#include "kolabhelpers.h"

#include <akonadi/collectionfetchjob.h>
#include <akonadi/collectionfetchscope.h>

#include <KDE/KLocalizedString>

KolabRelationResourceTask::KolabRelationResourceTask(ResourceStateInterface::Ptr resource, QObject *parent)
    : ResourceTask(DeferIfNoSession, resource, parent)
    , mImapSession(0)
{
}

Akonadi::Collection KolabRelationResourceTask::relationCollection() const
{
    return mRelationCollection;
}

void KolabRelationResourceTask::doStart(KIMAP::Session *session)
{
    mImapSession = session;

    // need to find the configuration collection.

    Akonadi::Collection topLevelCollection;
    topLevelCollection.setRemoteId(rootRemoteId());
    topLevelCollection.setParentCollection(Akonadi::Collection::root());

    Akonadi::CollectionFetchJob *fetchJob = new Akonadi::CollectionFetchJob(topLevelCollection, Akonadi::CollectionFetchJob::Recursive);
    fetchJob->fetchScope().setResource(resourceState()->resourceIdentifier());
    fetchJob->fetchScope().setContentMimeTypes(QStringList() << KolabHelpers::getMimeType(Kolab::ConfigurationType));
    fetchJob->fetchScope().setAncestorRetrieval(Akonadi::CollectionFetchScope::All);
    fetchJob->fetchScope().setListFilter(Akonadi::CollectionFetchScope::NoFilter);
    connect(fetchJob, SIGNAL(result(KJob*)), this, SLOT(onCollectionFetchResult(KJob*)));
}

void KolabRelationResourceTask::onCollectionFetchResult(KJob *job)
{
    if (job->error() == 0) {
        Akonadi::CollectionFetchJob *fetchJob = qobject_cast<Akonadi::CollectionFetchJob*>(job);
        Q_ASSERT(fetchJob != 0);

        Q_FOREACH (const Akonadi::Collection &collection, fetchJob->collections()) {
            const QString mailBox = mailBoxForCollection(collection);
            if (!mailBox.isEmpty()) {
                mRelationCollection = collection;
                startRelationTask(mImapSession);
                return;
            }
        }
    }

    kDebug() << "Couldn't find collection for relations";
    cancelTask(i18n("No mailbox for storing relations available, cancelling tag operation."));
}
