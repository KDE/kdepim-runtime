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

#include "kolabresource.h"

#include <resourcestateinterface.h>
#include <resourcestate.h>
#include <timestampattribute.h>
#include <retrieveitemstask.h>
#include <collectionannotationsattribute.h>

#include <KLocalizedString>
#include <Akonadi/CollectionFetchJob>
#include <Akonadi/CollectionFetchScope>
#include <KLocale>

#include "kolabretrievecollectionstask.h"
#include "kolabresourcestate.h"
#include "kolabhelpers.h"

KolabResource::KolabResource(const QString& id)
    :ImapResource(id)
{
    //Load translations from imap resource
    KGlobal::locale()->insertCatalog(QLatin1String("akonadi_imap_resource"));
}

KolabResource::~KolabResource()
{

}

QString KolabResource::defaultName() const
{
    return i18n("Kolab Resource");
}

ResourceStateInterface::Ptr KolabResource::createResourceState(const TaskArguments &args)
{
    return ResourceStateInterface::Ptr(new KolabResourceState(this, args));
}

void KolabResource::retrieveCollections()
{
    emit status(AgentBase::Running, i18nc("@info:status", "Retrieving folders"));

    setKeepLocalCollectionChanges(QSet<QByteArray>() << "CONTENTMIMETYPES" << "AccessRights");
    startTask(new KolabRetrieveCollectionsTask(createResourceState(TaskArguments()), this));
}

void KolabResource::retrieveItems(const Akonadi::Collection &col)
{
    //The collection that we receive was fetched when the task was scheduled, it is therefore possible that it is outdated.
    //We refetch the collection since we rely on up-to-date annotations.
    //FIXME: because this is async and not part of the resourcetask, it can't be killed. ResourceBase should just provide an up-to date copy of the collection.
    Akonadi::CollectionFetchJob *fetchJob = new Akonadi::CollectionFetchJob(col, Akonadi::CollectionFetchJob::Base, this);
    fetchJob->fetchScope().setAncestorRetrieval(Akonadi::CollectionFetchScope::All);
    fetchJob->fetchScope().setIncludeStatistics(true);
    fetchJob->fetchScope().setIncludeUnsubscribed(true);
    connect(fetchJob, SIGNAL(result(KJob*)), this, SLOT(onItemRetrievalCollectionFetchDone(KJob*)));
}

void KolabResource::onItemRetrievalCollectionFetchDone(KJob *job)
{
    if (job->error()) {
        kWarning() << "Failed to retrieve collection before RetrieveItemsTask: " << job->errorString();
        cancelTask(i18n("Failed to retrieve items."));
        return;
    }

    Akonadi::CollectionFetchJob *fetchJob = static_cast<Akonadi::CollectionFetchJob*>(job);
    Q_ASSERT(fetchJob->collections().size() == 1);
    const Akonadi::Collection col = fetchJob->collections().first();

    //This is the only part that differs form the imap resource: We make sure the annotations are up-to date before synchronizing
    //HACK avoid infinite recursions, the metadatatask should be scheduled at most once per retrieveItemsJob
    static QSet<Akonadi::Collection::Id> updatedCollections;
    if (!updatedCollections.contains(col.id()) &&
        (!col.attribute<TimestampAttribute>() ||
        col.attribute<TimestampAttribute>()->timestamp() < QDateTime::currentDateTime().addSecs(-60).toTime_t())) {
        updatedCollections.insert(col.id());
        synchronizeCollectionAttributes(col.id());
        deferTask();
        return;
    }
    updatedCollections.remove(col.id());

    setItemStreamingEnabled(true);

    RetrieveItemsTask *task = new RetrieveItemsTask( createResourceState(TaskArguments(col)), this);
    connect(task, SIGNAL(status(int,QString)), SIGNAL(status(int,QString)));
    connect(this, SIGNAL(retrieveNextItemSyncBatch(int)), task, SLOT(onReadyForNextBatch(int)));
    startTask(task);
}

void KolabResource::itemAdded(const Akonadi::Item& item, const Akonadi::Collection& collection)
{
    bool ok = true;
    const Akonadi::Item imapItem = KolabHelpers::translateToImap(item, ok);
    if (!ok) {
        kWarning() << "Failed to convert item";
        cancelTask();
        return;
    }
    ImapResource::itemAdded(imapItem, collection);
}

void KolabResource::itemChanged(const Akonadi::Item& item, const QSet< QByteArray >& parts)
{
    bool ok = true;
    const Akonadi::Item imapItem = KolabHelpers::translateToImap(item, ok);
    if (!ok) {
        kWarning() << "Failed to convert item";
        cancelTask();
        return;
    }
    ImapResource::itemChanged(imapItem, parts);
}

void KolabResource::itemsMoved(const Akonadi::Item::List& items, const Akonadi::Collection& source, const Akonadi::Collection& destination)
{
    bool ok = true;
    const Akonadi::Item::List imapItems = KolabHelpers::translateToImap(items, ok);
    if (!ok) {
        kWarning() << "Failed to convert item";
        cancelTask();
        return;
    }
    ImapResource::itemsMoved(imapItems, source, destination);
}

static Akonadi::Collection updateAnnotations(const Akonadi::Collection &collection)
{
    //Set the annotations on new folders
    const QByteArray kolabType = KolabHelpers::kolabTypeForMimeType(collection.contentMimeTypes());
    if (!kolabType.isEmpty()) {
        Akonadi::Collection col = collection;
        Akonadi::CollectionAnnotationsAttribute *attr = col.attribute<Akonadi::CollectionAnnotationsAttribute>(Akonadi::Collection::AddIfMissing);
        QMap<QByteArray, QByteArray> annotations = attr->annotations();
        KolabHelpers::setFolderTypeAnnotation(annotations, kolabType);
        attr->setAnnotations(annotations);
        return col;
    }
    return collection;
}

void KolabResource::collectionAdded(const Akonadi::Collection& collection, const Akonadi::Collection& parent)
{
    //Set the annotations on new folders
    const Akonadi::Collection col = updateAnnotations(collection);
    //TODO we need to save the collections as well if the annotations have changed
    //or we simply don't have the annotations locally, which perhaps is also not required?
    ImapResource::collectionAdded(col, parent);
}

void KolabResource::collectionChanged(const Akonadi::Collection& collection, const QSet< QByteArray >& parts)
{
    //Update annotations if necessary
    const Akonadi::Collection col = updateAnnotations(collection);
    //TODO we need to save the collections as well if the annotations have changed
    ImapResource::collectionChanged(col, parts);
}

AKONADI_RESOURCE_MAIN( KolabResource )
