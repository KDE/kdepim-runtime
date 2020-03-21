/*
    Copyright (C) 2011-2013  Daniel Vrátil <dvratil@redhat.com>
                  2020  Igor Poboiko <igor.poboiko@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "taskhandler.h"
#include "googleresource.h"
#include "googlesettings.h"

#include <AkonadiCore/CollectionColorAttribute>
#include <AkonadiCore/CollectionModifyJob>
#include <AkonadiCore/EntityDisplayAttribute>
#include <AkonadiCore/ItemModifyJob>
#include <AkonadiCore/ItemFetchJob>
#include <AkonadiCore/ItemFetchScope>
#include <Akonadi/Calendar/BlockAlarmsAttribute>

#include <KGAPI/Account>
#include <KGAPI/Tasks/TaskList>
#include <KGAPI/Tasks/TaskListCreateJob>
#include <KGAPI/Tasks/TaskListDeleteJob>
#include <KGAPI/Tasks/TaskListFetchJob>
#include <KGAPI/Tasks/TaskListModifyJob>
#include <KGAPI/Tasks/Task>
#include <KGAPI/Tasks/TaskCreateJob>
#include <KGAPI/Tasks/TaskDeleteJob>
#include <KGAPI/Tasks/TaskFetchJob>
#include <KGAPI/Tasks/TaskModifyJob>
#include <KGAPI/Tasks/TaskMoveJob>

#include <KCalendarCore/Todo>

#include "googletasks_debug.h"

#define TASK_PROPERTY "_KGAPI2::TaskPtr"


using namespace KGAPI2;
using namespace Akonadi;

Q_DECLARE_METATYPE(KGAPI2::TaskPtr)

QString TaskHandler::mimetype()
{
    return KCalendarCore::Todo::todoMimeType();
}

bool TaskHandler::canPerformTask(const Akonadi::Item &item)
{
    return m_resource->canPerformTask<KCalendarCore::Todo::Ptr>(item, mimetype());
}

void TaskHandler::retrieveCollections()
{
    Q_EMIT status(AgentBase::Running, i18nc("@info:status", "Retrieving task lists"));
    qCDebug(GOOGLE_TASKS_LOG) << "Retrieving tasks...";
    auto job = new TaskListFetchJob(m_settings->accountPtr(), this);
    connect(job, &KGAPI2::Job::finished, this, &TaskHandler::slotCollectionsRetrieved);
}

void TaskHandler::slotCollectionsRetrieved(KGAPI2::Job* job)
{
    if (!m_resource->handleError(job)) {
        return;
    }
    qCDebug(GOOGLE_TASKS_LOG) << "Task lists retrieved";

    const ObjectsList taskLists = qobject_cast<TaskListFetchJob *>(job)->items();
    const QStringList activeTaskLists = m_settings->taskLists();
    Collection::List collections;
    for (const ObjectPtr &object : taskLists) {
        const TaskListPtr &taskList = object.dynamicCast<TaskList>();
        qCDebug(GOOGLE_TASKS_LOG) << "Retrieved task list:" << taskList->uid();

        if (!activeTaskLists.contains(taskList->uid())) {
            qCDebug(GOOGLE_TASKS_LOG) << "Skipping, not subscribed";
            continue;
        }

        Collection collection;
        collection.setContentMimeTypes(QStringList() << KCalendarCore::Todo::todoMimeType());
        collection.setName(taskList->uid());
        collection.setParentCollection(m_resource->rootCollection());
        collection.setRemoteId(taskList->uid());
        collection.setRights(Collection::CanChangeCollection
                             |Collection::CanCreateItem
                             |Collection::CanChangeItem
                             |Collection::CanDeleteItem);

        EntityDisplayAttribute *attr = collection.attribute<EntityDisplayAttribute>(Collection::AddIfMissing);
        attr->setDisplayName(taskList->title());
        attr->setIconName(QStringLiteral("view-pim-tasks"));
        collections << collection;
    }

    Q_EMIT collectionsRetrieved(collections);
}

void TaskHandler::retrieveItems(const Collection &collection)
{
    Q_EMIT status(AgentBase::Running, i18nc("@info:status", "Retrieving tasks for list '%1'", collection.displayName()));
    qCDebug(GOOGLE_TASKS_LOG) << "Retrieving tasks for list" << collection.remoteId();
    // https://bugs.kde.org/show_bug.cgi?id=308122: we can only request changes in
    // max. last 25 days, otherwise we get an error.
    int lastSyncDelta = -1;
    if (!collection.remoteRevision().isEmpty()) {
        lastSyncDelta = QDateTime::currentDateTimeUtc().toSecsSinceEpoch() - collection.remoteRevision().toULongLong();
    }

    auto job = new TaskFetchJob(collection.remoteId(), m_settings->accountPtr(), this);
    if (lastSyncDelta > -1 && lastSyncDelta < 25 * 25 * 3600) {
        job->setFetchOnlyUpdated(collection.remoteRevision().toULongLong());
    }

    job->setProperty(COLLECTION_PROPERTY, QVariant::fromValue(collection));
    connect(job, &KGAPI2::Job::finished, this, &TaskHandler::slotItemsRetrieved);
}

void TaskHandler::slotItemsRetrieved(KGAPI2::Job *job)
{
    if (!m_resource->handleError(job)) {
        return;
    }
    Item::List changedItems, removedItems;

    const ObjectsList& objects = qobject_cast<FetchJob *>(job)->items();
    Collection collection = job->property(COLLECTION_PROPERTY).value<Collection>();
    bool isIncremental = (qobject_cast<TaskFetchJob *>(job)->fetchOnlyUpdated() > 0);
    qCDebug(GOOGLE_TASKS_LOG) << "Retrieved" << objects.count() << "tasks for list" << collection.remoteId();
    for (const auto &object : objects) {
        TaskPtr task = object.dynamicCast<Task>();

        Item item;
        item.setMimeType(KCalendarCore::Todo::todoMimeType());
        item.setParentCollection(collection);
        item.setRemoteId(task->uid());
        item.setRemoteRevision(task->etag());
        item.setPayload<KCalendarCore::Todo::Ptr>(task.dynamicCast<KCalendarCore::Todo>());

        if (task->deleted()) {
            qCDebug(GOOGLE_TASKS_LOG) << " - removed" << task->uid();
            removedItems << item;
        } else {
            qCDebug(GOOGLE_TASKS_LOG) << " - changed" << task->uid();
            changedItems << item;
        }
    }

    if (isIncremental) {
        m_resource->itemsRetrievedIncremental(changedItems, removedItems);
    } else {
        m_resource->itemsRetrieved(changedItems);
    }
    const QDateTime local(QDateTime::currentDateTime());
    const QDateTime UTC(local.toUTC());

    collection.setRemoteRevision(QString::number(UTC.toSecsSinceEpoch()));
    new CollectionModifyJob(collection, this);

    emitReadyStatus();
}

void TaskHandler::itemAdded(const Item &item, const Collection &collection)
{
    Q_EMIT status(AgentBase::Running, i18nc("@info:status", "Adding event to calendar '%1'", collection.displayName()));
    qCDebug(GOOGLE_TASKS_LOG) << "Task added to list" << collection.remoteId();
    KCalendarCore::Todo::Ptr todo = item.payload<KCalendarCore::Todo::Ptr>();
    TaskPtr ktodo(new Task(*todo));
    ktodo->setUid(QLatin1String(""));

    if (!ktodo->relatedTo(KCalendarCore::Incidence::RelTypeParent).isEmpty()) {
        Akonadi::Item parentItem;
        parentItem.setRemoteId(ktodo->relatedTo(KCalendarCore::Incidence::RelTypeParent));
        qCDebug(GOOGLE_TASKS_LOG) << "Fetching task parent" << parentItem.remoteId();

        auto job = new ItemFetchJob(parentItem, this);
        job->setCollection(collection);
        job->setProperty(ITEM_PROPERTY, QVariant::fromValue(item));
        job->setProperty(TASK_PROPERTY, QVariant::fromValue(ktodo));

        connect(job, &ItemFetchJob::finished, this, &TaskHandler::slotTaskAddedSearchFinished);
        return;
    } else {
        auto job = new TaskCreateJob(ktodo, collection.remoteId(), m_settings->accountPtr(), this);
        job->setProperty(ITEM_PROPERTY, QVariant::fromValue(item));
        connect(job, &KGAPI2::Job::finished, this, &TaskHandler::slotCreateJobFinished);
    }
}

void TaskHandler::slotTaskAddedSearchFinished(KJob* job)
{
    if (job->error()) {
        m_resource->cancelTask(i18n("Failed to add task: %1", job->errorString()));
        return;
    }
    ItemFetchJob *fetchJob = qobject_cast<ItemFetchJob *>(job);
    Item item = job->property(ITEM_PROPERTY).value<Item>();
    TaskPtr task = job->property(TASK_PROPERTY).value<TaskPtr>();

    Item::List items = fetchJob->items();
    qCDebug(GOOGLE_TASKS_LOG) << "Received" << items.count() << "parents for task";

    const QString tasksListId = item.parentCollection().remoteId();

    // Make sure account is still valid
    if (!m_resource->canPerformTask()) {
        return;
    }

    KGAPI2::Job *newJob = nullptr;
    // The parent is not known, so give up and just store the item in Google
    // without the information about parent.
    // TODO: this is not necessary
    if (items.isEmpty()) {
        task->setRelatedTo(QString(), KCalendarCore::Incidence::RelTypeParent);
        newJob = new TaskCreateJob(task, tasksListId, m_settings->accountPtr(), this);
    } else {
        Item matchedItem = items.first();
        qCDebug(GOOGLE_TASKS_LOG) << "Adding task with parent" << matchedItem.remoteId();

        task->setRelatedTo(matchedItem.remoteId(), KCalendarCore::Incidence::RelTypeParent);
        TaskCreateJob *createJob = new TaskCreateJob(task, tasksListId, m_settings->accountPtr(), this);
        createJob->setParentItem(matchedItem.remoteId());
        newJob = createJob;
    }

    newJob->setProperty(ITEM_PROPERTY, QVariant::fromValue(item));
    connect(newJob, &KGAPI2::Job::finished, this, &TaskHandler::slotCreateJobFinished);
}

void TaskHandler::slotCreateJobFinished(KGAPI2::Job* job)
{
    if (!m_resource->handleError(job)) {
        return;
    }

    Item item = job->property(ITEM_PROPERTY).value<Item>();
    ObjectsList objects = qobject_cast<CreateJob *>(job)->items();
    Q_ASSERT(objects.count() > 0);

    TaskPtr task = objects.first().dynamicCast<Task>();
    item.setRemoteId(task->uid());
    item.setRemoteRevision(task->etag());
    item.setGid(task->uid());
    item.setPayload<KCalendarCore::Todo::Ptr>(task.dynamicCast<KCalendarCore::Todo>());
    m_resource->changeCommitted(item);
    emitReadyStatus();
}

void TaskHandler::itemChanged(const Item &item, const QSet< QByteArray > &partIdentifiers)
{
    Q_UNUSED(partIdentifiers);
    Q_EMIT status(AgentBase::Running, i18nc("@info:status", "Changing task in list '%1'", item.parentCollection().displayName()));
    qCDebug(GOOGLE_TASKS_LOG) << "Changing task" << item.remoteId();

    KCalendarCore::Todo::Ptr todo = item.payload<KCalendarCore::Todo::Ptr>();
    TaskPtr ktodo(new Task(*todo));
    QString parentUid = todo->relatedTo(KCalendarCore::Incidence::RelTypeParent);
    auto job = new TaskMoveJob(item.remoteId(), item.parentCollection().remoteId(), parentUid, m_settings->accountPtr(), this);
    connect(job, &TaskMoveJob::finished, [ktodo, item, this](KGAPI2::Job* job){
                if (!m_resource->handleError(job)) {
                    return;
                }
                qCDebug(GOOGLE_TASKS_LOG) << "Move task" << item.remoteId() << "finished, modifying...";
                auto newJob = new TaskModifyJob(ktodo, item.parentCollection().remoteId(), job->account(), this);
                newJob->setProperty(ITEM_PROPERTY, QVariant::fromValue(item));
                connect(job, &KGAPI2::Job::finished, m_resource, &GoogleResource::slotGenericJobFinished);
            });
}

void TaskHandler::itemRemoved(const Item &item)
{
    Q_EMIT status(AgentBase::Running, i18nc("@info:status", "Removing task from list '%1'", item.parentCollection().displayName()));
    qCDebug(GOOGLE_TASKS_LOG) << "Removing task" << item.remoteId();
    /* Google always automatically removes tasks with all their subtasks. In KOrganizer
     * by default we only remove the item we are given. For this reason we have to first
     * fetch all tasks, find all sub-tasks for the task being removed and detach them
     * from the task. Only then the task can be safely removed. */
    auto job = new ItemFetchJob(item.parentCollection());
    job->setAutoDelete(true);
    job->fetchScope().fetchFullPayload(true);
    job->setProperty(ITEM_PROPERTY, QVariant::fromValue(item));
    connect(job, &ItemFetchJob::finished, this, &TaskHandler::slotRemoveTaskFetchJobFinished);
}

void TaskHandler::slotRemoveTaskFetchJobFinished(KJob* job)
{
    if (job->error()) {
        m_resource->cancelTask(i18n("Failed to delete task: %1", job->errorString()));
        return;
    }
    qCDebug(GOOGLE_TASKS_LOG) << "Item fetched, removing...";

    ItemFetchJob *fetchJob = qobject_cast<ItemFetchJob *>(job);
    Item removedItem = fetchJob->property(ITEM_PROPERTY).value<Item>();
    const Item::List items = fetchJob->items();

    Item::List detachItems;
    for (Item item : items) {
        if (!item.hasPayload<KCalendarCore::Todo::Ptr>()) {
            qCDebug(GOOGLE_TASKS_LOG) << "Item " << item.remoteId() << " does not have Todo payload";
            continue;
        }

        KCalendarCore::Todo::Ptr todo = item.payload<KCalendarCore::Todo::Ptr>();
        /* If this item is child of the item we want to remove then add it to detach list */
        if (todo->relatedTo(KCalendarCore::Incidence::RelTypeParent) == removedItem.remoteId()) {
            todo->setRelatedTo(QString(), KCalendarCore::Incidence::RelTypeParent);
            item.setPayload(todo);
            detachItems << item;
        }
    }

    /* If there are no items do detach, then delete the task right now */
    if (detachItems.isEmpty()) {
        slotDoRemoveTask(job);
        return;
    }

    /* Send modify request to detach all the sub-tasks from the task that is about to be
     * removed. */
    auto modifyJob = new ItemModifyJob(detachItems);
    modifyJob->setProperty(ITEM_PROPERTY, QVariant::fromValue(removedItem));
    modifyJob->setAutoDelete(true);
    connect(modifyJob, &ItemModifyJob::finished, this, &TaskHandler::slotDoRemoveTask);
}

void TaskHandler::slotDoRemoveTask(KJob *job)
{
    if (job->error()) {
        m_resource->cancelTask(i18n("Failed to delete task: %1", job->errorString()));
        return;
    }

    // Make sure account is still valid
    if (!m_resource->canPerformTask()) {
        return;
    }

    Item item = job->property(ITEM_PROPERTY).value< Item >();

    /* Now finally we can safely remove the task we wanted to */
    auto deleteJob = new TaskDeleteJob(item.remoteId(), item.parentCollection().remoteId(), m_settings->accountPtr(), this);
    deleteJob->setProperty(ITEM_PROPERTY, QVariant::fromValue(item));
    connect(deleteJob, &TaskDeleteJob::finished, m_resource, &GoogleResource::slotGenericJobFinished);
}

void TaskHandler::itemMoved(const Item &item, const Collection &collectionSource, const Collection &collectionDestination)
{
    m_resource->cancelTask(i18n("Moving tasks between task lists is not supported"));
}

void TaskHandler::collectionAdded(const Akonadi::Collection &collection, const Akonadi::Collection &parent)
{
    Q_UNUSED(parent);
    Q_EMIT status(AgentBase::Running, i18nc("@info:status", "Creating new task list '%1'", collection.displayName()));
    qCDebug(GOOGLE_TASKS_LOG) << "Adding task list" << collection.displayName();
    TaskListPtr taskList(new TaskList());
    taskList->setTitle(collection.displayName());

    auto job = new TaskListCreateJob(taskList, m_settings->accountPtr(), this);
    job->setProperty(COLLECTION_PROPERTY, QVariant::fromValue(collection));
    connect(job, &KGAPI2::Job::finished, m_resource, &GoogleResource::slotGenericJobFinished);
}

void TaskHandler::collectionChanged(const Akonadi::Collection &collection)
{
    Q_EMIT status(AgentBase::Running, i18nc("@info:status", "Changing task list '%1'", collection.displayName()));
    qCDebug(GOOGLE_TASKS_LOG) << "Changing task list" << collection.remoteId();

    TaskListPtr taskList(new TaskList());
    taskList->setUid(collection.remoteId());
    taskList->setTitle(collection.displayName());
    auto job = new TaskListModifyJob(taskList, m_settings->accountPtr(), this);
    job->setProperty(COLLECTION_PROPERTY, QVariant::fromValue(collection));
    connect(job, &KGAPI2::Job::finished, m_resource, &GoogleResource::slotGenericJobFinished);
}

void TaskHandler::collectionRemoved(const Akonadi::Collection &collection)
{
    Q_EMIT status(AgentBase::Running, i18nc("@info:status", "Removing task list '%1'", collection.displayName()));
    qCDebug(GOOGLE_TASKS_LOG) << "Removing task list" << collection.remoteId();
    auto job = new TaskListDeleteJob(collection.remoteId(), m_settings->accountPtr(), this);
    job->setProperty(COLLECTION_PROPERTY, QVariant::fromValue(collection));
    connect(job, &KGAPI2::Job::finished, m_resource, &GoogleResource::slotGenericJobFinished);
}
