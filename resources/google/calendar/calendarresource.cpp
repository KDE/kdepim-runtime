/*
    Copyright (C) 2011-2013  Daniel Vrátil <dvratil@redhat.com>

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

#include "calendarresource.h"
#include "defaultreminderattribute.h"
#include "settings.h"
#include "settingsdialog.h"
#include "googlecalendarresource_debug.h"

#include <AkonadiCore/Attribute>
#include <AkonadiCore/AttributeFactory>
#include <AkonadiCore/CollectionModifyJob>
#include <AkonadiCore/EntityDisplayAttribute>
#include <AkonadiCore/CollectionColorAttribute>
#include <AkonadiCore/ItemFetchJob>
#include <AkonadiCore/ItemFetchScope>
#include <AkonadiCore/ItemModifyJob>
#include <Akonadi/Calendar/BlockAlarmsAttribute>
#include <AkonadiCore/CachePolicy>
#include <AkonadiCore/VectorHelper>

#include <KCalCore/Calendar>
#include <KCalCore/Attendee>
#include <KCalCore/ICalFormat>
#include <KCalCore/FreeBusy>

#include <KLocalizedString>
#include <QDialog>
#include <QIcon>

#include <KGAPI/Calendar/Calendar>
#include <KGAPI/Calendar/CalendarCreateJob>
#include <KGAPI/Calendar/CalendarDeleteJob>
#include <KGAPI/Calendar/CalendarFetchJob>
#include <KGAPI/Calendar/CalendarModifyJob>
#include <KGAPI/Calendar/Event>
#include <KGAPI/Calendar/EventCreateJob>
#include <KGAPI/Calendar/EventDeleteJob>
#include <KGAPI/Calendar/EventFetchJob>
#include <KGAPI/Calendar/EventModifyJob>
#include <KGAPI/Calendar/EventMoveJob>
#include <KGAPI/Calendar/FreeBusyQueryJob>
#include <KGAPI/Tasks/Task>
#include <KGAPI/Tasks/TaskCreateJob>
#include <KGAPI/Tasks/TaskDeleteJob>
#include <KGAPI/Tasks/TaskFetchJob>
#include <KGAPI/Tasks/TaskModifyJob>
#include <KGAPI/Tasks/TaskMoveJob>
#include <KGAPI/Tasks/TaskList>
#include <KGAPI/Tasks/TaskListCreateJob>
#include <KGAPI/Tasks/TaskListDeleteJob>
#include <KGAPI/Tasks/TaskListFetchJob>
#include <KGAPI/Tasks/TaskListModifyJob>

#include <KGAPI/Account>

#define ROOT_COLLECTION_REMOTEID QStringLiteral("RootCollection")
#define CALENDARS_PROPERTY "_KGAPI2CalendarPtr"
#define TASK_PROPERTY "_KGAPI2::TaskPtr"

Q_DECLARE_METATYPE(KGAPI2::ObjectsList)
Q_DECLARE_METATYPE(KGAPI2::TaskPtr)

using namespace Akonadi;
using namespace KGAPI2;

CalendarResource::CalendarResource(const QString &id)
    : GoogleResource(id)
{
    AttributeFactory::registerAttribute< DefaultReminderAttribute >();
    updateResourceName();
}

CalendarResource::~CalendarResource()
{
}

GoogleSettings *CalendarResource::settings() const
{
    return Settings::self();
}

int CalendarResource::runConfigurationDialog(WId windowId)
{
    QScopedPointer<SettingsDialog> settingsDialog(new SettingsDialog(accountManager(), windowId, this));
    settingsDialog->setWindowIcon(QIcon::fromTheme(QStringLiteral("im-google")));

    return settingsDialog->exec();
}

void CalendarResource::updateResourceName()
{
    const QString accountName = Settings::self()->account();
    setName(i18nc("%1 is account name (user@gmail.com)", "Google Calendars and Tasks (%1)", accountName.isEmpty() ? i18n("not configured") : accountName));
}

QList< QUrl > CalendarResource::scopes() const
{
    const QList<QUrl> scopes = { Account::calendarScopeUrl(), Account::tasksScopeUrl()};

    return scopes;
}

void CalendarResource::retrieveItems(const Akonadi::Collection &collection)
{
    if (!canPerformTask()) {
        return;
    }

    // https://bugs.kde.org/show_bug.cgi?id=308122: we can only request changes in
    // max. last 25 days, otherwise we get an error.
    int lastSyncDelta = -1;
    if (!collection.remoteRevision().isEmpty()) {
        lastSyncDelta = QDateTime::currentDateTimeUtc().toTime_t() - collection.remoteRevision().toUInt();
    }

    KGAPI2::Job *job = nullptr;
    if (collection.contentMimeTypes().contains(KCalCore::Event::eventMimeType())) {
        EventFetchJob *fetchJob = new EventFetchJob(collection.remoteId(), account(), this);
        if (lastSyncDelta > -1 && lastSyncDelta < 25 * 24 * 3600) {
            fetchJob->setFetchOnlyUpdated(collection.remoteRevision().toULongLong());
        }
        if (!Settings::self()->eventsSince().isEmpty()) {
            const QDate date = QDate::fromString(Settings::self()->eventsSince(), Qt::ISODate);
            fetchJob->setTimeMin(QDateTime(date).toTime_t());
        }
        job = fetchJob;
    } else if (collection.contentMimeTypes().contains(KCalCore::Todo::todoMimeType())) {
        TaskFetchJob *fetchJob = new TaskFetchJob(collection.remoteId(), account(), this);
        if (lastSyncDelta > -1 && lastSyncDelta < 25 * 25 * 3600) {
            fetchJob->setFetchOnlyUpdated(collection.remoteRevision().toULongLong());
        }
        job = fetchJob;
    } else {
        itemsRetrieved(Item::List());
        return;
    }

    job->setProperty(COLLECTION_PROPERTY, QVariant::fromValue(collection));
    connect(job, &KGAPI2::Job::progress, this, &CalendarResource::emitPercent);
    connect(job, &KGAPI2::Job::finished, this, &CalendarResource::slotItemsRetrieved);
}

void CalendarResource::retrieveCollections()
{
    if (!canPerformTask()) {
        return;
    }

    CalendarFetchJob *fetchJob = new CalendarFetchJob(account(), this);
    connect(fetchJob, &EventFetchJob::finished, this, &CalendarResource::slotCalendarsRetrieved);
}

void CalendarResource::itemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection)
{
    if ((!collection.contentMimeTypes().contains(KCalCore::Event::eventMimeType())
         && !collection.contentMimeTypes().contains(KCalCore::Todo::todoMimeType()))
        || (!canPerformTask<KCalCore::Event::Ptr>(item, KCalCore::Event::eventMimeType())
            && !canPerformTask<KCalCore::Todo::Ptr>(item, KCalCore::Todo::todoMimeType()))) {
        return;
    }

    if (collection.parentCollection() == Akonadi::Collection::root()) {
        cancelTask(i18n("The top-level collection cannot contain any tasks or events"));
        return;
    }

    KGAPI2::Job *job = nullptr;
    if (item.hasPayload<KCalCore::Event::Ptr>()) {
        KCalCore::Event::Ptr event = item.payload<KCalCore::Event::Ptr>();
        EventPtr kevent(new Event(*event));
        auto cjob = new EventCreateJob(kevent, collection.remoteId(), account(), this);
        cjob->setSendUpdates(SendUpdatesPolicy::None);
        job = cjob;
    } else if (item.hasPayload<KCalCore::Todo::Ptr>()) {
        KCalCore::Todo::Ptr todo = item.payload<KCalCore::Todo::Ptr>();
        TaskPtr ktodo(new Task(*todo));
        ktodo->setUid(QLatin1String(""));

        if (!ktodo->relatedTo(KCalCore::Incidence::RelTypeParent).isEmpty()) {
            Akonadi::Item parentItem;
            parentItem.setGid(ktodo->relatedTo(KCalCore::Incidence::RelTypeParent));

            ItemFetchJob *fetchJob = new ItemFetchJob(parentItem, this);
            fetchJob->setProperty(ITEM_PROPERTY, QVariant::fromValue(item));
            fetchJob->setProperty(TASK_PROPERTY, QVariant::fromValue(ktodo));

            connect(fetchJob, &ItemFetchJob::finished, this, &CalendarResource::slotTaskAddedSearchFinished);
            return;
        } else {
            job = new TaskCreateJob(ktodo, collection.remoteId(), account(), this);
        }
    } else {
        cancelTask(i18n("Invalid payload type"));
        return;
    }

    job->setProperty(ITEM_PROPERTY, QVariant::fromValue(item));
    connect(job, &CreateJob::finished, this, &CalendarResource::slotCreateJobFinished);
}

void CalendarResource::itemChanged(const Akonadi::Item &item, const QSet< QByteArray > &partIdentifiers)
{
    Q_UNUSED(partIdentifiers);

    if (!canPerformTask<KCalCore::Event::Ptr>(item, KCalCore::Event::eventMimeType())
        && !canPerformTask<KCalCore::Todo::Ptr>(item, KCalCore::Todo::todoMimeType())) {
        return;
    }

    KGAPI2::Job *job = nullptr;
    if (item.hasPayload<KCalCore::Event::Ptr>()) {
        KCalCore::Event::Ptr event = item.payload<KCalCore::Event::Ptr>();
        EventPtr kevent(new Event(*event));

        auto mjob = new EventModifyJob(kevent, item.parentCollection().remoteId(), account(), this);
        mjob->setSendUpdates(SendUpdatesPolicy::None);
        connect(mjob, &EventModifyJob::finished, this, &CalendarResource::slotGenericJobFinished);
        job = mjob;
    } else if (item.hasPayload<KCalCore::Todo::Ptr>()) {
        KCalCore::Todo::Ptr todo = item.payload<KCalCore::Todo::Ptr>();
        //FIXME unused ktodo ?
        //TaskPtr ktodo(new Task(*todo));
        QString parentUid = todo->relatedTo(KCalCore::Incidence::RelTypeParent);
        job = new TaskMoveJob(item.remoteId(), item.parentCollection().remoteId(), parentUid, account(), this);
        job->setProperty(ITEM_PROPERTY, QVariant::fromValue(item));
        connect(job, &TaskMoveJob::finished, this, &CalendarResource::slotModifyTaskReparentFinished);
    } else {
        cancelTask(i18n("Invalid payload type"));
        return;
    }

    job->setProperty(ITEM_PROPERTY, QVariant::fromValue(item));
}

void CalendarResource::itemRemoved(const Akonadi::Item &item)
{
    if (!canPerformTask()) {
        return;
    }

    if (item.mimeType() == KCalCore::Event::eventMimeType()) {
        KGAPI2::Job *job = new EventDeleteJob(item.remoteId(), item.parentCollection().remoteId(), account(), this);
        job->setProperty(ITEM_PROPERTY, QVariant::fromValue(item));
        connect(job, &EventDeleteJob::finished, this, &CalendarResource::slotGenericJobFinished);
    } else if (item.mimeType() == KCalCore::Todo::todoMimeType()) {
        /* Google always automatically removes tasks with all their subtasks. In KOrganizer
         * by default we only remove the item we are given. For this reason we have to first
         * fetch all tasks, find all sub-tasks for the task being removed and detach them
         * from the task. Only then the task can be safely removed. */
        ItemFetchJob *fetchJob = new ItemFetchJob(item.parentCollection());
        fetchJob->setAutoDelete(true);
        fetchJob->fetchScope().fetchFullPayload(true);
        fetchJob->setProperty(ITEM_PROPERTY, qVariantFromValue(item));
        connect(fetchJob, &ItemFetchJob::finished, this, &CalendarResource::slotRemoveTaskFetchJobFinished);
        fetchJob->start();
    } else {
        cancelTask(i18n("Invalid payload type. Expected event or todo, got %1", item.mimeType()));
    }
}

void CalendarResource::itemMoved(const Item &item, const Collection &collectionSource, const Collection &collectionDestination)
{
    if (!canPerformTask()) {
        return;
    }

    if (collectionDestination.parentCollection() == Akonadi::Collection::root()) {
        cancelTask(i18n("The top-level collection cannot contain any tasks or events"));
        return;
    }

    if (item.mimeType() != KCalCore::Event::eventMimeType()) {
        cancelTask(i18n("Moving tasks between task lists is not supported"));
        return;
    }

    KGAPI2::Job *job = new EventMoveJob(item.remoteId(), collectionSource.remoteId(),
                                        collectionDestination.remoteId(), account(),
                                        this);
    job->setProperty(ITEM_PROPERTY, QVariant::fromValue(item));
    connect(job, &EventMoveJob::finished, this, &CalendarResource::slotGenericJobFinished);
}

void CalendarResource::collectionAdded(const Collection &collection, const Collection &parent)
{
    Q_UNUSED(parent)

    if (!canPerformTask()) {
        return;
    }

    KGAPI2::Job *job = nullptr;
    if (collection.contentMimeTypes().contains(KCalCore::Event::eventMimeType())) {
        CalendarPtr calendar(new Calendar());
        calendar->setTitle(collection.displayName());
        calendar->setEditable(true);
        job = new CalendarCreateJob(calendar, account(), this);
    }
    if (collection.contentMimeTypes().contains(KCalCore::Todo::todoMimeType())) {
        TaskListPtr taskList(new TaskList());
        taskList->setTitle(collection.displayName());

        job = new TaskListCreateJob(taskList, account(), this);
    } else {
        cancelTask(i18n("Unknown collection mimetype"));
        return;
    }

    job->setProperty(COLLECTION_PROPERTY, QVariant::fromValue(collection));
    connect(job, &KGAPI2::Job::finished, this, &CalendarResource::slotGenericJobFinished);
}

void CalendarResource::collectionChanged(const Collection &collection)
{
    if (!canPerformTask()) {
        return;
    }

    KGAPI2::Job *job = nullptr;
    if (collection.contentMimeTypes().contains(KCalCore::Event::eventMimeType())) {
        CalendarPtr calendar(new Calendar());
        calendar->setUid(collection.remoteId());
        calendar->setTitle(collection.displayName());
        calendar->setEditable(true);
        job = new CalendarModifyJob(calendar, account(), this);
    }
    if (collection.contentMimeTypes().contains(KCalCore::Todo::todoMimeType())) {
        TaskListPtr taskList(new TaskList());
        taskList->setUid(collection.remoteId());
        taskList->setTitle(collection.displayName());
        job = new TaskListModifyJob(taskList, account(), this);
    } else {
        cancelTask(i18n("Unknown collection mimetype"));
        return;
    }

    job->setProperty(COLLECTION_PROPERTY, QVariant::fromValue(collection));
    connect(job, &KGAPI2::Job::finished, this, &CalendarResource::slotGenericJobFinished);
}

void CalendarResource::collectionRemoved(const Collection &collection)
{
    if (!canPerformTask()) {
        return;
    }

    KGAPI2::Job *job = nullptr;
    if (collection.contentMimeTypes().contains(KCalCore::Event::eventMimeType())) {
        job = new CalendarDeleteJob(collection.remoteId(), account(), this);
    } else if (collection.contentMimeTypes().contains(KCalCore::Todo::todoMimeType())) {
        job = new TaskListDeleteJob(collection.remoteId(), account(), this);
    } else {
        cancelTask(i18n("Unknown collection mimetype"));
        return;
    }

    job->setProperty(COLLECTION_PROPERTY, QVariant::fromValue(collection));
    connect(job, &KGAPI2::Job::finished, this, &CalendarResource::slotGenericJobFinished);
}

void CalendarResource::slotCalendarsRetrieved(KGAPI2::Job *job)
{
    if (!handleError(job)) {
        return;
    }

    CalendarFetchJob *calendarJob = qobject_cast<CalendarFetchJob *>(job);
    ObjectsList objects = calendarJob->items();
    calendarJob->deleteLater();

    TaskListFetchJob *fetchJob = new TaskListFetchJob(job->account(), this);
    fetchJob->setProperty(CALENDARS_PROPERTY, QVariant::fromValue(objects));
    connect(fetchJob, &EventFetchJob::finished, this, &CalendarResource::slotCollectionsRetrieved);
}

void CalendarResource::slotCollectionsRetrieved(KGAPI2::Job *job)
{
    if (!handleError(job)) {
        return;
    }

    TaskListFetchJob *fetchJob = qobject_cast<TaskListFetchJob *>(job);
    ObjectsList calendars = fetchJob->property(CALENDARS_PROPERTY).value<ObjectsList>();
    ObjectsList taskLists = fetchJob->items();

    CachePolicy cachePolicy;
    if (Settings::self()->enableIntervalCheck()) {
        cachePolicy.setInheritFromParent(false);
        cachePolicy.setIntervalCheckTime(Settings::self()->intervalCheckTime());
    }

    m_rootCollection = Collection();
    m_rootCollection.setContentMimeTypes(QStringList() << Collection::mimeType());
    m_rootCollection.setRemoteId(ROOT_COLLECTION_REMOTEID);
    m_rootCollection.setName(fetchJob->account()->accountName());
    m_rootCollection.setParentCollection(Collection::root());
    m_rootCollection.setRights(Collection::CanCreateCollection);
    m_rootCollection.setCachePolicy(cachePolicy);

    EntityDisplayAttribute *attr = m_rootCollection.attribute<EntityDisplayAttribute>(Collection::AddIfMissing);
    attr->setDisplayName(fetchJob->account()->accountName());
    attr->setIconName(QStringLiteral("im-google"));

    m_collections[ ROOT_COLLECTION_REMOTEID ] = m_rootCollection;

    const QStringList activeCalendars = Settings::self()->calendars();
    for (const ObjectPtr &object : calendars) {
        const CalendarPtr &calendar = object.dynamicCast<Calendar>();

        if (!activeCalendars.contains(calendar->uid())) {
            continue;
        }

        Collection collection;
        collection.setContentMimeTypes(QStringList() << KCalCore::Event::eventMimeType());
        collection.setName(calendar->uid());
        collection.setParentCollection(m_rootCollection);
        collection.setRemoteId(calendar->uid());
        if (calendar->editable()) {
            collection.setRights(Collection::CanChangeCollection
                                 |Collection::CanCreateItem
                                 |Collection::CanChangeItem
                                 |Collection::CanDeleteItem);
        } else {
            collection.setRights(nullptr);
        }

        EntityDisplayAttribute *attr = collection.attribute<EntityDisplayAttribute>(Collection::AddIfMissing);
        attr->setDisplayName(calendar->title());
        attr->setIconName(QStringLiteral("view-calendar"));

        auto colorAttr = collection.attribute<CollectionColorAttribute>(Collection::AddIfMissing);
        colorAttr->setColor(calendar->backgroundColor());

        DefaultReminderAttribute *reminderAttr = collection.attribute<DefaultReminderAttribute>(Collection::AddIfMissing);
        reminderAttr->setReminders(calendar->defaultReminders());

        // Block email reminders, since Google sends them for us
        BlockAlarmsAttribute *blockAlarms = collection.attribute<BlockAlarmsAttribute>(Collection::AddIfMissing);
        blockAlarms->blockAlarmType(KCalCore::Alarm::Audio, false);
        blockAlarms->blockAlarmType(KCalCore::Alarm::Display, false);
        blockAlarms->blockAlarmType(KCalCore::Alarm::Procedure, false);

        m_collections[ collection.remoteId() ] = collection;
    }

    const QStringList activeTaskLists = Settings::self()->taskLists();
    for (const ObjectPtr &object : taskLists) {
        const TaskListPtr &taskList = object.dynamicCast<TaskList>();

        if (!activeTaskLists.contains(taskList->uid())) {
            continue;
        }

        Collection collection;
        collection.setContentMimeTypes(QStringList() << KCalCore::Todo::todoMimeType());
        collection.setName(taskList->uid());
        collection.setParentCollection(m_rootCollection);
        collection.setRemoteId(taskList->uid());
        collection.setRights(Collection::CanChangeCollection
                             |Collection::CanCreateItem
                             |Collection::CanChangeItem
                             |Collection::CanDeleteItem);

        EntityDisplayAttribute *attr = collection.attribute<EntityDisplayAttribute>(Collection::AddIfMissing);
        attr->setDisplayName(taskList->title());
        attr->setIconName(QStringLiteral("view-pim-tasks"));

        m_collections[ collection.remoteId() ] = collection;
    }

    collectionsRetrieved(Akonadi::valuesToVector(m_collections));

    job->deleteLater();
}

void CalendarResource::slotItemsRetrieved(KGAPI2::Job *job)
{
    if (!handleError(job)) {
        return;
    }

    Item::List changedItems, removedItems;
    Collection collection = job->property(COLLECTION_PROPERTY).value<Collection>();
    DefaultReminderAttribute *attr = collection.attribute<DefaultReminderAttribute>();
    bool isIncremental = false;

    ObjectsList objects = qobject_cast<FetchJob *>(job)->items();
    if (collection.contentMimeTypes().contains(KCalCore::Event::eventMimeType())) {
        QMap< QString, EventPtr > recurrentEvents;

        isIncremental = (qobject_cast<EventFetchJob *>(job)->fetchOnlyUpdated() > 0);
        Q_FOREACH (const ObjectPtr &object, objects) {
            EventPtr event = object.dynamicCast<Event>();
            if (event->useDefaultReminders() && attr) {
                const KCalCore::Alarm::List alarms = attr->alarms(event.data());
                for (const KCalCore::Alarm::Ptr &alarm : alarms) {
                    event->addAlarm(alarm);
                }
            }

            Item item;
            item.setMimeType(KCalCore::Event::eventMimeType());
            item.setParentCollection(collection);
            item.setRemoteId(event->id());
            item.setRemoteRevision(event->etag());
            item.setPayload<KCalCore::Event::Ptr>(event.dynamicCast<KCalCore::Event>());

            if (event->deleted()) {
                removedItems << item;
            } else {
                changedItems << item;
            }
        }

        }
    } else if (collection.contentMimeTypes().contains(KCalCore::Todo::todoMimeType())) {
        isIncremental = (qobject_cast<TaskFetchJob *>(job)->fetchOnlyUpdated() > 0);

        Q_FOREACH (const ObjectPtr &object, objects) {
            TaskPtr task = object.dynamicCast<Task>();

            Item item;
            item.setMimeType(KCalCore::Todo::todoMimeType());
            item.setParentCollection(collection);
            item.setRemoteId(task->uid());
            item.setRemoteRevision(task->etag());
            item.setPayload<KCalCore::Todo::Ptr>(task.dynamicCast<KCalCore::Todo>());

            if (task->deleted()) {
                removedItems << item;
            } else {
                changedItems << item;
            }
        }
    }

    if (isIncremental) {
        itemsRetrievedIncremental(changedItems, removedItems);
    } else {
        itemsRetrieved(changedItems);
    }
    const QDateTime local(QDateTime::currentDateTime());
    const QDateTime UTC(local.toUTC());

    collection.setRemoteRevision(QString::number(UTC.toTime_t()));
    new CollectionModifyJob(collection, this);

    job->deleteLater();
}

void CalendarResource::slotModifyTaskReparentFinished(KGAPI2::Job *job)
{
    if (!handleError(job)) {
        return;
    }

    Item item = job->property(ITEM_PROPERTY).value<Item>();
    KCalCore::Todo::Ptr todo = item.payload<KCalCore::Todo::Ptr>();
    TaskPtr ktodo(new Task(*todo.data()));

    job = new TaskModifyJob(ktodo, item.parentCollection().remoteId(), job->account(), this);
    job->setProperty(ITEM_PROPERTY, QVariant::fromValue(item));
    connect(job, &KGAPI2::Job::finished, this, &CalendarResource::slotGenericJobFinished);
}

void CalendarResource::slotRemoveTaskFetchJobFinished(KJob *job)
{
    if (job->error()) {
        cancelTask(i18n("Failed to delete task: %1", job->errorString()));
        return;
    }

    ItemFetchJob *fetchJob = qobject_cast<ItemFetchJob *>(job);
    Item removedItem = fetchJob->property(ITEM_PROPERTY).value<Item>();

    Item::List detachItems;

    const Item::List items = fetchJob->items();
    for (Item item : items) {
        if (!item.hasPayload<KCalCore::Todo::Ptr>()) {
            qCDebug(GOOGLE_CALENDAR_LOG) << "Item " << item.remoteId() << " does not have Todo payload";
            continue;
        }

        KCalCore::Todo::Ptr todo = item.payload<KCalCore::Todo::Ptr>();
        /* If this item is child of the item we want to remove then add it to detach list */
        if (todo->relatedTo(KCalCore::Incidence::RelTypeParent) == removedItem.remoteId()) {
            todo->setRelatedTo(QString(), KCalCore::Incidence::RelTypeParent);
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
    ItemModifyJob *modifyJob = new ItemModifyJob(detachItems);
    modifyJob->setProperty(ITEM_PROPERTY, QVariant::fromValue(removedItem));
    modifyJob->setAutoDelete(true);
    connect(modifyJob, &ItemModifyJob::finished, this, &CalendarResource::slotDoRemoveTask);
}

void CalendarResource::slotDoRemoveTask(KJob *job)
{
    if (job->error()) {
        cancelTask(i18n("Failed to delete task: %1", job->errorString()));
        return;
    }

    // Make sure account is still valid
    if (!canPerformTask()) {
        return;
    }

    Item item = job->property(ITEM_PROPERTY).value< Item >();

    /* Now finally we can safely remove the task we wanted to */
    TaskDeleteJob *deleteJob = new TaskDeleteJob(item.remoteId(), item.parentCollection().remoteId(), account(), this);
    deleteJob->setProperty(ITEM_PROPERTY, QVariant::fromValue(item));
    connect(deleteJob, &TaskDeleteJob::finished, this, &CalendarResource::slotGenericJobFinished);
}

void CalendarResource::slotTaskAddedSearchFinished(KJob *job)
{
    ItemFetchJob *fetchJob = qobject_cast<ItemFetchJob *>(job);
    Item item = job->property(ITEM_PROPERTY).value<Item>();
    TaskPtr task = job->property(TASK_PROPERTY).value<TaskPtr>();

    Item::List items = fetchJob->items();
    qCDebug(GOOGLE_CALENDAR_LOG) << "Parent query returned" << items.count() << "results";

    const QString tasksListId = item.parentCollection().remoteId();

    // Make sure account is still valid
    if (!canPerformTask()) {
        return;
    }

    KGAPI2::Job *newJob = nullptr;
    // The parent is not known, so give up and just store the item in Google
    // without the information about parent.
    if (items.isEmpty()) {
        task->setRelatedTo(QString(), KCalCore::Incidence::RelTypeParent);
        newJob = new TaskCreateJob(task, tasksListId, account(), this);
    } else {
        Item matchedItem = items.first();

        task->setRelatedTo(matchedItem.remoteId(), KCalCore::Incidence::RelTypeParent);
        TaskCreateJob *createJob = new TaskCreateJob(task, tasksListId, account(), this);
        createJob->setParentItem(matchedItem.remoteId());
        newJob = createJob;
    }

    newJob->setProperty(ITEM_PROPERTY, QVariant::fromValue(item));
    connect(newJob, &KGAPI2::Job::finished, this, &CalendarResource::slotCreateJobFinished);
}

void CalendarResource::slotCreateJobFinished(KGAPI2::Job *job)
{
    if (!handleError(job)) {
        return;
    }

    Item item = job->property(ITEM_PROPERTY).value<Item>();

    CreateJob *createJob = qobject_cast<CreateJob *>(job);
    ObjectsList objects = createJob->items();
    Q_ASSERT(objects.count() > 0);

    if (item.mimeType() == KCalCore::Event::eventMimeType()) {
        EventPtr event = objects.first().dynamicCast<Event>();
        item.setRemoteId(event->id());
        item.setRemoteRevision(event->etag());
        item.setGid(event->uid());
        changeCommitted(item);
        item.setPayload<KCalCore::Event::Ptr>(event.dynamicCast<KCalCore::Event>());
        new ItemModifyJob(item, this);
    } else if (item.mimeType() == KCalCore::Todo::todoMimeType()) {
        TaskPtr task = objects.first().dynamicCast<Task>();
        item.setRemoteId(task->uid());
        item.setRemoteRevision(task->etag());
        item.setGid(task->uid());
        changeCommitted(item);
        item.setPayload<KCalCore::Todo::Ptr>(task.dynamicCast<KCalCore::Todo>());
        new ItemModifyJob(item, this);
    }
}

QDateTime CalendarResource::lastCacheUpdate() const
{
    return QDateTime();
}

void CalendarResource::canHandleFreeBusy(const QString &email) const
{
    if (!const_cast<CalendarResource *>(this)->canPerformTask()) {
        handlesFreeBusy(email, false);
        return;
    }

    auto job = new KGAPI2::FreeBusyQueryJob(email,
                                            QDateTime::currentDateTimeUtc(),
                                            QDateTime::currentDateTimeUtc().addSecs(3600),
                                            const_cast<CalendarResource *>(this)->account(),
                                            const_cast<CalendarResource *>(this));
    connect(job, &KGAPI2::Job::finished,
            this, &CalendarResource::slotCanHandleFreeBusyJobFinished);
}

void CalendarResource::slotCanHandleFreeBusyJobFinished(KGAPI2::Job *job)
{
    auto queryJob = qobject_cast<KGAPI2::FreeBusyQueryJob *>(job);

    if (!handleError(job, false)) {
        handlesFreeBusy(queryJob->id(), false);
        return;
    }

    handlesFreeBusy(queryJob->id(), true);
}

void CalendarResource::retrieveFreeBusy(const QString &email, const QDateTime &start, const QDateTime &end)
{
    if (!const_cast<CalendarResource *>(this)->canPerformTask()) {
        freeBusyRetrieved(email, QString(), false, QString());
        return;
    }

    auto job = new KGAPI2::FreeBusyQueryJob(email, start, end, account(), this);
    connect(job, &KGAPI2::Job::finished,
            this, &CalendarResource::slotRetrieveFreeBusyJobFinished);
}

void CalendarResource::slotRetrieveFreeBusyJobFinished(KGAPI2::Job *job)
{
    auto queryJob = qobject_cast<KGAPI2::FreeBusyQueryJob *>(job);

    if (!handleError(job, false)) {
        freeBusyRetrieved(queryJob->id(), QString(), false, QString());
        return;
    }

    KCalCore::FreeBusy::Ptr fb(new KCalCore::FreeBusy);
    fb->setUid(QStringLiteral("%1%2@google.com").arg(QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyyMMddTHHmmssZ"))));
    fb->setOrganizer(account()->accountName());
    fb->addAttendee(KCalCore::Attendee::Ptr(new KCalCore::Attendee(QString(), queryJob->id())));
    // FIXME: is it really sort?
    fb->setDateTime(QDateTime::currentDateTimeUtc(), KCalCore::IncidenceBase::RoleSort);

    Q_FOREACH (const KGAPI2::FreeBusyQueryJob::BusyRange &range, queryJob->busy()) {
        fb->addPeriod(range.busyStart, range.busyEnd);
    }

    KCalCore::ICalFormat format;
    const QString fbStr = format.createScheduleMessage(fb, KCalCore::iTIPRequest);

    freeBusyRetrieved(queryJob->id(), fbStr, true, QString());
}

AKONADI_RESOURCE_MAIN(CalendarResource)
