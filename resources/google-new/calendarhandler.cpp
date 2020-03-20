/*
    Copyright (C) 2011-2013  Daniel Vrátil <dvratil@redhat.com>
                  2020       Igor Poboiko <igor.poboiko@gmail.com>

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

#include "calendarhandler.h"
#include "defaultreminderattribute.h"
#include "googleresource.h"
#include "googlesettings.h"
#include "kgapiversionattribute.h"

#include <AkonadiCore/CollectionColorAttribute>
#include <AkonadiCore/CollectionModifyJob>
#include <AkonadiCore/EntityDisplayAttribute>
#include <AkonadiCore/ItemModifyJob>
#include <Akonadi/Calendar/BlockAlarmsAttribute>

#include <KGAPI/Account>
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

#include <KCalendarCore/Calendar>
#include <KCalendarCore/FreeBusy>
#include <KCalendarCore/ICalFormat>

#include "googleresource_debug.h"

using namespace KGAPI2;
using namespace Akonadi;

static constexpr uint32_t KGAPIEventVersion = 1;

QString CalendarHandler::mimetype()
{
    return KCalendarCore::Event::eventMimeType();
}

bool CalendarHandler::canPerformTask(const Akonadi::Item &item)
{
    return m_resource->canPerformTask<KCalendarCore::Event::Ptr>(item, mimetype());
}

void CalendarHandler::retrieveCollections()
{
    Q_EMIT status(AgentBase::Running, i18nc("@info:status", "Retrieving calendars"));
    qCDebug(GOOGLE_LOG) << "Retrieving calendars...";
    auto job = new CalendarFetchJob(m_settings->accountPtr(), this);
    connect(job, &KGAPI2::Job::finished, this, &CalendarHandler::slotCollectionsRetrieved);

}

void CalendarHandler::slotCollectionsRetrieved(KGAPI2::Job* job)
{
    if (!m_resource->handleError(job)) {
        return;
    }
    qCDebug(GOOGLE_LOG) << "Calendars retrieved";

    const ObjectsList calendars = qobject_cast<CalendarFetchJob *>(job)->items();
    Collection::List collections;

    const QStringList activeCalendars = m_settings->calendars();
    for (const auto &object : calendars) {
        const CalendarPtr &calendar = object.dynamicCast<Calendar>();
        qCDebug(GOOGLE_LOG) << "Retrieved calendar:" << calendar->title() << "(" << calendar->uid() << ")";

        if (!activeCalendars.contains(calendar->uid())) {
            qCDebug(GOOGLE_LOG) << "Skipping, not subscribed";
            continue;
        }

        Collection collection;
        collection.setContentMimeTypes(QStringList() << mimetype());
        collection.setName(calendar->uid());
        collection.setParentCollection(m_resource->rootCollection());
        collection.setRemoteId(calendar->uid());
        if (calendar->editable()) {
            collection.setRights(Collection::CanChangeCollection
            |Collection::CanCreateItem
            |Collection::CanChangeItem
            |Collection::CanDeleteItem);
        } else {
            collection.setRights(Collection::ReadOnly);
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
        blockAlarms->blockAlarmType(KCalendarCore::Alarm::Audio, false);
        blockAlarms->blockAlarmType(KCalendarCore::Alarm::Display, false);
        blockAlarms->blockAlarmType(KCalendarCore::Alarm::Procedure, false);

        collections << collection;
    }

    Q_EMIT collectionsRetrieved(collections);
}

void CalendarHandler::retrieveItems(const Collection &collection)
{
    qCDebug(GOOGLE_LOG) << "Retrieving events for calendar" << collection.remoteId();
    // https://bugs.kde.org/show_bug.cgi?id=308122: we can only request changes in
    // max. last 25 days, otherwise we get an error.
    int lastSyncDelta = -1;
    if (!collection.remoteRevision().isEmpty()) {
        lastSyncDelta = QDateTime::currentDateTimeUtc().toSecsSinceEpoch() - collection.remoteRevision().toULongLong();
    }

    if (!collection.hasAttribute<KGAPIVersionAttribute>() || collection.attribute<KGAPIVersionAttribute>()->version() != KGAPIEventVersion) {
        lastSyncDelta = -1;
    }
    auto job = new EventFetchJob(collection.remoteId(), m_settings->accountPtr(), this);
    if (lastSyncDelta > -1 && lastSyncDelta < 25 * 24 * 3600) {
        job->setFetchOnlyUpdated(collection.remoteRevision().toULongLong());
    }
    if (!m_settings->eventsSince().isEmpty()) {
        const QDate date = QDate::fromString(m_settings->eventsSince(), Qt::ISODate);
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
        job->setTimeMin(QDateTime(date).toSecsSinceEpoch());
#else
        job->setTimeMin(QDateTime(date.startOfDay()).toSecsSinceEpoch());
#endif
    }

    job->setProperty(COLLECTION_PROPERTY, QVariant::fromValue(collection));
    connect(job, &KGAPI2::Job::finished, this, &CalendarHandler::slotItemsRetrieved);

    Q_EMIT status(AgentBase::Running, i18nc("@info:status", "Retrieving events for calendar '%1'", collection.displayName()));
}

void CalendarHandler::slotItemsRetrieved(KGAPI2::Job *job)
{
    if (!m_resource->handleError(job)) {
        return;
    }
    Item::List changedItems, removedItems;
    Collection collection = job->property(COLLECTION_PROPERTY).value<Collection>();
    DefaultReminderAttribute *attr = collection.attribute<DefaultReminderAttribute>();

    auto fetchJob = qobject_cast<EventFetchJob *>(job);
    const ObjectsList objects = fetchJob->items();
    bool isIncremental = (fetchJob->fetchOnlyUpdated() > 0);
    qCDebug(GOOGLE_LOG) << "Retrieved" << objects.count() << "events for calendar" << collection.remoteId();
    for (const ObjectPtr &object : objects) {
        const EventPtr event = object.dynamicCast<Event>();
        if (event->useDefaultReminders() && attr) {
            const KCalendarCore::Alarm::List alarms = attr->alarms(event.data());
            for (const KCalendarCore::Alarm::Ptr &alarm : alarms) {
                event->addAlarm(alarm);
            }
        }

        Item item;
        item.setMimeType(KCalendarCore::Event::eventMimeType());
        item.setParentCollection(collection);
        item.setRemoteId(event->id());
        item.setRemoteRevision(event->etag());
        item.setPayload<KCalendarCore::Event::Ptr>(event.dynamicCast<KCalendarCore::Event>());

        if (event->deleted()) {
            qCDebug(GOOGLE_LOG) << " - removed" << event->uid();
            removedItems << item;
        } else {
            qCDebug(GOOGLE_LOG) << " - changed" << event->uid();
            changedItems << item;
        }
    }

    if (!isIncremental) {
        collection.attribute<KGAPIVersionAttribute>(Collection::AddIfMissing)->setVersion(KGAPIEventVersion);
        m_resource->itemsRetrieved(changedItems);
    } else {
        m_resource->itemsRetrievedIncremental(changedItems, removedItems);
    }
    const QDateTime local(QDateTime::currentDateTime());
    const QDateTime UTC(local.toUTC());

    collection.setRemoteRevision(QString::number(UTC.toSecsSinceEpoch()));
    new CollectionModifyJob(collection, this);

    emitReadyStatus();
}

void CalendarHandler::itemAdded(const Item &item, const Collection &collection)
{
    Q_EMIT status(AgentBase::Running, i18nc("@info:status", "Adding event to calendar '%1'", collection.name()));
    qCDebug(GOOGLE_LOG) << "Event added to calendar" << collection.remoteId();
    KCalendarCore::Event::Ptr event = item.payload<KCalendarCore::Event::Ptr>();
    EventPtr kevent(new Event(*event));
    auto *job = new EventCreateJob(kevent, collection.remoteId(), m_settings->accountPtr(), this);
    job->setSendUpdates(SendUpdatesPolicy::None);
    job->setProperty(ITEM_PROPERTY, QVariant::fromValue(item));
    connect(job, &KGAPI2::Job::finished, this, &CalendarHandler::slotCreateJobFinished);
}

void CalendarHandler::slotCreateJobFinished(KGAPI2::Job* job)
{
    if (!m_resource->handleError(job)) {
        return;
    }

    Item item = job->property(ITEM_PROPERTY).value<Item>();

    ObjectsList objects = qobject_cast<CreateJob *>(job)->items();
    Q_ASSERT(objects.count() > 0);

    EventPtr event = objects.first().dynamicCast<Event>();
    qCDebug(GOOGLE_LOG) << "Event added";
    item.setRemoteId(event->id());
    item.setRemoteRevision(event->etag());
    item.setGid(event->uid());
    m_resource->changeCommitted(item);

    item.setPayload<KCalendarCore::Event::Ptr>(event.dynamicCast<KCalendarCore::Event>());
    new ItemModifyJob(item, this);

    emitReadyStatus();
}

void CalendarHandler::itemChanged(const Item &item, const QSet< QByteArray > &partIdentifiers)
{
    Q_UNUSED(partIdentifiers);
    Q_EMIT status(AgentBase::Running, i18nc("@info:status", "Changing event in calendar '%1'", item.parentCollection().displayName()));
    qCDebug(GOOGLE_LOG) << "Changing event" << item.remoteId();
    KCalendarCore::Event::Ptr event = item.payload<KCalendarCore::Event::Ptr>();
    EventPtr kevent(new Event(*event));
    auto job = new EventModifyJob(kevent, item.parentCollection().remoteId(), m_settings->accountPtr(), this);
    job->setSendUpdates(SendUpdatesPolicy::None);
    job->setProperty(ITEM_PROPERTY, QVariant::fromValue(item));
    connect(job, &EventModifyJob::finished, m_resource, &GoogleResource::slotGenericJobFinished);
}

void CalendarHandler::itemRemoved(const Item &item)
{
    Q_EMIT status(AgentBase::Running, i18nc("@info:status", "Removing event from calendar '%1'", item.parentCollection().displayName()));
    qCDebug(GOOGLE_LOG) << "Removing event" << item.remoteId();
    KGAPI2::Job *job = new EventDeleteJob(item.remoteId(), item.parentCollection().remoteId(), m_settings->accountPtr(), this);
    job->setProperty(ITEM_PROPERTY, QVariant::fromValue(item));
    connect(job, &EventDeleteJob::finished, m_resource, &GoogleResource::slotGenericJobFinished);

}

void CalendarHandler::itemMoved(const Item &item, const Collection &collectionSource, const Collection &collectionDestination)
{
    Q_EMIT status(AgentBase::Running, i18nc("@info:status", "Moving event from calendar '%1' to calendar '%2'", collectionSource.displayName(), collectionDestination.displayName()));
    qCDebug(GOOGLE_LOG) << "Moving" << item.remoteId() << "from" << collectionSource.remoteId() << "to" << collectionDestination.remoteId();
    KGAPI2::Job *job = new EventMoveJob(item.remoteId(), collectionSource.remoteId(), collectionDestination.remoteId(), m_settings->accountPtr(), this);
    job->setProperty(ITEM_PROPERTY, QVariant::fromValue(item));
    connect(job, &EventMoveJob::finished, m_resource, &GoogleResource::slotGenericJobFinished);
}

void CalendarHandler::collectionAdded(const Akonadi::Collection &collection, const Akonadi::Collection &parent)
{
    Q_UNUSED(parent);
    Q_EMIT status(AgentBase::Running, i18nc("@info:status", "Creating calendar '%1'", collection.displayName()));
    qCDebug(GOOGLE_LOG) << "Adding calendar" << collection.displayName();
    CalendarPtr calendar(new Calendar());
    calendar->setTitle(collection.displayName());
    calendar->setEditable(true);
    auto job = new CalendarCreateJob(calendar, m_settings->accountPtr(), this);
    job->setProperty(COLLECTION_PROPERTY, QVariant::fromValue(collection));
    connect(job, &KGAPI2::Job::finished, m_resource, &GoogleResource::slotGenericJobFinished);
}

void CalendarHandler::collectionChanged(const Akonadi::Collection &collection)
{
    Q_EMIT status(AgentBase::Running, i18nc("@info:status", "Changing calendar '%1'", collection.displayName()));
    qCDebug(GOOGLE_LOG) << "Changing calendar" << collection.remoteId();
    CalendarPtr calendar(new Calendar());
    calendar->setUid(collection.remoteId());
    calendar->setTitle(collection.displayName());
    calendar->setEditable(true);
    auto job = new CalendarModifyJob(calendar, m_settings->accountPtr(), this);
    job->setProperty(COLLECTION_PROPERTY, QVariant::fromValue(collection));
    connect(job, &KGAPI2::Job::finished, m_resource, &GoogleResource::slotGenericJobFinished);
}

void CalendarHandler::collectionRemoved(const Akonadi::Collection &collection)
{
    Q_EMIT status(AgentBase::Running, i18nc("@info:status", "Removing calendar '%1'", collection.displayName()));
    qCDebug(GOOGLE_LOG) << "Removing calendar" << collection.remoteId();
    auto job = new CalendarDeleteJob(collection.remoteId(), m_settings->accountPtr(), this);
    job->setProperty(COLLECTION_PROPERTY, QVariant::fromValue(collection));
    connect(job, &KGAPI2::Job::finished, m_resource, &GoogleResource::slotGenericJobFinished);
}

QDateTime CalendarHandler::lastCacheUpdate() const
{
    return QDateTime();
}

void CalendarHandler::canHandleFreeBusy(const QString &email) const
{
    if (m_resource->canPerformTask()) {
        m_resource->handlesFreeBusy(email, false);
        return;
    }

    auto job = new FreeBusyQueryJob(email,
                                    QDateTime::currentDateTimeUtc(),
                                    QDateTime::currentDateTimeUtc().addSecs(3600),
                                    m_settings->accountPtr(),
                                    const_cast<CalendarHandler*>(this));
    connect(job, &KGAPI2::Job::finished, [this](KGAPI2::Job *job){
                auto queryJob = qobject_cast<FreeBusyQueryJob *>(job);
                if (!m_resource->handleError(job, false)) {
                    m_resource->handlesFreeBusy(queryJob->id(), false);
                    return;
                }
                m_resource->handlesFreeBusy(queryJob->id(), true);
            });
}

void CalendarHandler::retrieveFreeBusy(const QString &email, const QDateTime &start, const QDateTime &end)
{
    if (m_resource->canPerformTask()) {
        m_resource->freeBusyRetrieved(email, QString(), false, QString());
        return;
    }

    auto job = new FreeBusyQueryJob(email, start, end, m_settings->accountPtr(), this);
    connect(job, &KGAPI2::Job::finished, [this](KGAPI2::Job *job) {
                auto queryJob = qobject_cast<FreeBusyQueryJob *>(job);

                if (!m_resource->handleError(job, false)) {
                    m_resource->freeBusyRetrieved(queryJob->id(), QString(), false, QString());
                    return;
                }

                KCalendarCore::FreeBusy::Ptr fb(new KCalendarCore::FreeBusy);
                fb->setUid(QStringLiteral("%1%2@google.com").arg(QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyyMMddTHHmmssZ"))));
                fb->setOrganizer(job->account()->accountName());
                fb->addAttendee(KCalendarCore::Attendee(QString(), queryJob->id()));
                // FIXME: is it really sort?
                fb->setDateTime(QDateTime::currentDateTimeUtc(), KCalendarCore::IncidenceBase::RoleSort);

                for (const auto range : queryJob->busy()) {
                    fb->addPeriod(range.busyStart, range.busyEnd);
                }

                KCalendarCore::ICalFormat format;
                const QString fbStr = format.createScheduleMessage(fb, KCalendarCore::iTIPRequest);

                m_resource->freeBusyRetrieved(queryJob->id(), fbStr, true, QString());
            });
}
