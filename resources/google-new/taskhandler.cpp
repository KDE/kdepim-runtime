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
        const TaskPtr task = object.dynamicCast<Task>();

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
    KCalendarCore::Todo::Ptr todo = item.payload<KCalendarCore::Todo::Ptr>();
    TaskPtr task(new Task(*todo));
    const QString parentRemoteId = task->relatedTo(KCalendarCore::Incidence::RelTypeParent);
    qCDebug(GOOGLE_TASKS_LOG) << "Task added to list" << collection.remoteId() << "with parent" << parentRemoteId;
    auto job = new TaskCreateJob(task, item.parentCollection().remoteId(), m_settings->accountPtr(), this);
    job->setParentItem(parentRemoteId);
    connect(job, &KGAPI2::Job::finished, [this, item](KGAPI2::Job *job){
                if (!m_resource->handleError(job)) {
                    return;
                }
                Item newItem = item;
                const TaskPtr task = qobject_cast<TaskCreateJob *>(job)->items().first().dynamicCast<Task>();
                qCDebug(GOOGLE_TASKS_LOG) << "Task added";
                newItem.setRemoteId(task->uid());
                newItem.setRemoteRevision(task->etag());
                newItem.setGid(task->uid());
                m_resource->changeCommitted(newItem);
                newItem.setPayload<KCalendarCore::Todo::Ptr>(task.dynamicCast<KCalendarCore::Todo>());
                new ItemModifyJob(newItem, this);
                emitReadyStatus();
            });
}

void TaskHandler::itemChanged(const Item &item, const QSet< QByteArray > &partIdentifiers)
{
    Q_UNUSED(partIdentifiers);
    Q_EMIT status(AgentBase::Running, i18nc("@info:status", "Changing task in list '%1'", item.parentCollection().displayName()));
    qCDebug(GOOGLE_TASKS_LOG) << "Changing task" << item.remoteId();

    KCalendarCore::Todo::Ptr todo = item.payload<KCalendarCore::Todo::Ptr>();
    const QString parentUid = todo->relatedTo(KCalendarCore::Incidence::RelTypeParent);
    TaskPtr task(new Task(*todo));
    // First we move it to a new parent, if there is
    auto job = new TaskMoveJob(item.remoteId(), item.parentCollection().remoteId(), parentUid, m_settings->accountPtr(), this);
    connect(job, &TaskMoveJob::finished, [this, task, item](KGAPI2::Job* job){
                if (!m_resource->handleError(job)) {
                    return;
                }
                auto newJob = new TaskModifyJob(task, item.parentCollection().remoteId(), job->account(), this);
                newJob->setProperty(ITEM_PROPERTY, QVariant::fromValue(item));
                connect(newJob, &KGAPI2::Job::finished, m_resource, &GoogleResource::slotGenericJobFinished);
            });
}

void TaskHandler::itemsRemoved(const Item::List &items)
{
    Q_EMIT status(AgentBase::Running, i18ncp("@info:status", "Removing %1 tasks", "Removing %1 task", items.count()));
    qCDebug(GOOGLE_TASKS_LOG) << "Removing" << items.count() << "tasks";
    /* Google always automatically removes tasks with all their subtasks. In KOrganizer
     * by default we only remove the item we are given. For this reason we have to first
     * fetch all tasks, find all sub-tasks for the task being removed and detach them
     * from the task. Only then the task can be safely removed. */
    // TODO: what if items belong to different collections?
    auto job = new ItemFetchJob(items.first().parentCollection());
    job->fetchScope().fetchFullPayload(true);
    connect(job, &ItemFetchJob::finished, [this, items](KJob *job){
                if (job->error()) {
                    m_resource->cancelTask(i18n("Failed to delete task: %1", job->errorString()));
                    return;
                }
                const Item::List fetchedItems = qobject_cast<ItemFetchJob *>(job)->items();
                Item::List detachItems;
                for (const Item &fetchedItem : fetchedItems) {
                    auto todo = fetchedItem.payload<KCalendarCore::Todo::Ptr>();
                    const QString parentId = todo->relatedTo(KCalendarCore::Incidence::RelTypeParent);
                    if (parentId.isEmpty()) {
                        continue;
                    }
                    for (const Item &item : items) {
                        if (item.remoteId() == parentId) {
                            Item newItem = item;
                            qCDebug(GOOGLE_TASKS_LOG) << "Detaching child" << item.remoteId() << "from" << parentId;
                            todo->setRelatedTo(QString(), KCalendarCore::Incidence::RelTypeParent);
                            newItem.setPayload<KCalendarCore::Todo::Ptr>(todo);
                            detachItems << newItem;
                        }
                    }
                }
                /* If there are no items do detach, then delete the task right now */
                if (detachItems.isEmpty()) {
                    slotDoRemoveTasks(items);
                    return;
                }
                qCDebug(GOOGLE_TASKS_LOG) << "Modifying" << detachItems.count() << "children...";
                /* Send modify request to detach all the sub-tasks from the task that is about to be
                 * removed. */
                auto modifyJob = new ItemModifyJob(detachItems);
                connect(modifyJob, &ItemModifyJob::finished, [this, items](KJob *job){
                            if (job->error()) {
                                m_resource->cancelTask(i18n("Failed to delete tasks:", job->errorString()));
                            }
                            slotDoRemoveTasks(items);
                        });
            });
}

void TaskHandler::slotDoRemoveTasks(const Item::List &items)
{
    // Make sure account is still valid
    if (!m_resource->canPerformTask()) {
        return;
    }
    QStringList taskIds;
    taskIds.reserve(items.count());
    std::transform(items.cbegin(), items.cend(), std::back_inserter(taskIds),
            [](const Item &item){
                return item.remoteId();
            });

    /* Now finally we can safely remove the task we wanted to */
    // TODO: what if tasks are deleted from different collections?
    auto job = new TaskDeleteJob(taskIds, items.first().parentCollection().remoteId(), m_settings->accountPtr(), this);
    job->setProperty(ITEMS_PROPERTY, QVariant::fromValue(items));
    connect(job, &TaskDeleteJob::finished, m_resource, &GoogleResource::slotGenericJobFinished);
}

void TaskHandler::itemsMoved(const Item::List &item, const Collection &collectionSource, const Collection &collectionDestination)
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
