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

#include "setupserver.h"
#include <resourcestateinterface.h>
#include <resourcestate.h>
#include <retrieveitemstask.h>
#include <collectionannotationsattribute.h>
#include <changecollectiontask.h>
#include <akonadi/calendar/blockalarmsattribute.h>

#include <KWindowSystem>
#include <KLocalizedString>
#include <KLocale>

#include "kolabretrievetagstask.h"
#include "kolabresourcestate.h"
#include "kolabhelpers.h"
#include "kolabsettings.h"
#include "kolabaddtagtask.h"
#include "kolabchangeitemstagstask.h"
#include "kolabchangeitemsrelationstask.h"
#include "kolabchangetagtask.h"
#include "kolabremovetagtask.h"
#include "kolabretrievecollectionstask.h"
#include "kolabretrievetagstask.h"
#include "tracer.h"

KolabResource::KolabResource(const QString& id)
    :ImapResource(id)
{
    //Load translations from imap resource
    KGlobal::locale()->insertCatalog(QLatin1String("akonadi_imap_resource"));
    //Ensure we have up-to date metadata before attempting to sync folder
    setScheduleAttributeSyncBeforeItemSync(true);
    setKeepLocalCollectionChanges(QSet<QByteArray>() << "ENTITYDISPLAY" << Akonadi::BlockAlarmsAttribute().type());
}

KolabResource::~KolabResource()
{

}

Settings *KolabResource::settings() const
{
  if (m_settings == 0) {
    m_settings = new KolabSettings;
  }

  return m_settings;
}

void KolabResource::delayedInit()
{
    ImapResource::delayedInit();
    settings()->setRetrieveMetadataOnFolderListing(false);
    Q_ASSERT(!settings()->retrieveMetadataOnFolderListing());
}

QString KolabResource::defaultName()
{
    return i18n("Kolab Resource");
}

KDialog *KolabResource::createConfigureDialog(WId windowId)
{
  SetupServer *dlg = new SetupServer( this, windowId );
  KWindowSystem::setMainWindow( dlg, windowId );
  dlg->setWindowIcon( KIcon( QLatin1String("kolab") ) );
  connect(dlg, SIGNAL(finished(int)), this, SLOT(onConfigurationDone(int)));;
  return dlg;
}

ResourceStateInterface::Ptr KolabResource::createResourceState(const TaskArguments &args)
{
    return ResourceStateInterface::Ptr(new KolabResourceState(this, args));
}

void KolabResource::retrieveCollections()
{
    Trace();
    emit status(AgentBase::Running, i18nc("@info:status", "Retrieving folders"));

    startTask(new KolabRetrieveCollectionsTask(createResourceState(TaskArguments()), this));
    synchronizeTags();
    synchronizeRelations();
}

void KolabResource::itemAdded(const Akonadi::Item& item, const Akonadi::Collection& collection)
{
    Trace() << item.id() << collection.id();
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
    Trace() << item.id() << parts;
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
    Trace() << items.size() << source.id() << destination.id();
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
    Trace() << collection.id();
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
    Trace() << collection.id() << parent.id();
    //Set the annotations on new folders
    const Akonadi::Collection col = updateAnnotations(collection);
    //TODO we need to save the collections as well if the annotations have changed
    //or we simply don't have the annotations locally, which perhaps is also not required?
    ImapResource::collectionAdded(col, parent);
}

void KolabResource::collectionChanged(const Akonadi::Collection& collection, const QSet< QByteArray >& parts)
{
    Trace() << collection.id() << parts;
    //Update annotations if necessary
    const Akonadi::Collection col = updateAnnotations(collection);
    //TODO we need to save the collections as well if the annotations have changed
    emit status( AgentBase::Running, i18nc( "@info:status", "Updating folder '%1'", collection.name() ) );
    ChangeCollectionTask *task = new ChangeCollectionTask( createResourceState(TaskArguments(collection, parts)), this );
    task->syncEnabledState(true);
    startTask(task);
}

void KolabResource::tagAdded(const Akonadi::Tag &tag)
{
    Trace() << tag.id();
    KolabAddTagTask *task = new KolabAddTagTask(createResourceState(TaskArguments(tag)), this);
    startTask(task);
}

void KolabResource::tagChanged(const Akonadi::Tag &tag)
{
    Trace() << tag.id();
    KolabChangeTagTask *task = new KolabChangeTagTask(createResourceState(TaskArguments(tag)), QSharedPointer<TagConverter>(new TagConverter), this);
    startTask(task);
}

void KolabResource::tagRemoved(const Akonadi::Tag &tag)
{
    Trace() << tag.id();
    KolabRemoveTagTask *task = new KolabRemoveTagTask(createResourceState(TaskArguments(tag)), this);
    startTask(task);
}

void KolabResource::itemsTagsChanged(const Akonadi::Item::List &items, const QSet<Akonadi::Tag> &addedTags, const QSet<Akonadi::Tag> &removedTags)
{
    Trace() << items.size() << addedTags.size() << removedTags.size();
    KolabChangeItemsTagsTask *task = new KolabChangeItemsTagsTask(createResourceState(TaskArguments(items, addedTags, removedTags)), QSharedPointer<TagConverter>(new TagConverter), this);
    startTask(task);
}

void KolabResource::retrieveTags()
{
    Trace();
    KolabRetrieveTagTask *task = new KolabRetrieveTagTask(createResourceState(TaskArguments()), KolabRetrieveTagTask::RetrieveTags, this);
    startTask(task);
}

void KolabResource::retrieveRelations()
{
    Trace();
    KolabRetrieveTagTask *task = new KolabRetrieveTagTask(createResourceState(TaskArguments()), KolabRetrieveTagTask::RetrieveRelations, this);
    startTask(task);
}

void KolabResource::itemsRelationsChanged(const Akonadi::Item::List &items,
                                          const Akonadi::Relation::List &addedRelations,
                                          const Akonadi::Relation::List &removedRelations)
{
    Trace() << items.size() << addedRelations.size() << removedRelations.size();
    KolabChangeItemsRelationsTask *task = new KolabChangeItemsRelationsTask(createResourceState(TaskArguments(items, addedRelations, removedRelations)));
}

AKONADI_RESOURCE_MAIN( KolabResource )
