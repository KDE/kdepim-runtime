/*
    SPDX-FileCopyrightText: 2026 Noham Devillers <noham.devillers@enioka.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "pimdataprovidercalendar.h"
#include "calendaradaptor.h"
#include "pimdataprovider_debug.h"
#include "pimdataprovider_types.h"

#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemFetchScope>
#include <KCalendarCore/ICalFormat>
#include <QDBusConnection>

namespace Akonadi
{
PimDataProviderCalendar::PimDataProviderCalendar(const QString &serverId, const Collection &collection, QObject *parent)
    : QObject(parent)
    , mServerId(serverId)
    , mCalendarId(collection.remoteId())
    , mCalendar(KCalendarCore::MemoryCalendar::Ptr::create(QTimeZone::systemTimeZone()))
{
    new CalendarAdaptor(this);

    // Get the list of already registered events
    auto job = new Akonadi::ItemFetchJob(collection, this);
    job->fetchScope().fetchFullPayload(true);
    connect(job, &KJob::result, this, &PimDataProviderCalendar::slotInitialEventsFetched);
}

PimDataProviderCalendar::~PimDataProviderCalendar()
{
    const QString path = QStringLiteral("/org/pim/dataprovider/Calendar/Servers/") + encodeForDBusPath(mServerId) + QStringLiteral("/Calendar/")
        + encodeForDBusPath(mCalendarId);
    QDBusConnection::sessionBus().unregisterObject(path);
}

QString PimDataProviderCalendar::id() const
{
    return mCalendarId;
}

QString PimDataProviderCalendar::name() const
{
    return mName;
}

QString PimDataProviderCalendar::color() const
{
    return mColor;
}

void PimDataProviderCalendar::setName(const QString &name)
{
    mName = name;
}

void PimDataProviderCalendar::setColor(const QString &color)
{
    mColor = color;
}

QStringList PimDataProviderCalendar::getEvent(const QString &itemId) const
{
    QStringList result;
    KCalendarCore::ICalFormat formatter;

    auto event = mCalendar->event(itemId);
    if (!event) {
        return {};
    }

    result.append(formatter.toICalString(event));

    // Add event exceptions
    const auto instances = mCalendar->instances(event);
    for (const auto &instance : instances) {
        if (instance != event) {
            result.append(formatter.toICalString(instance));
        }
    }
    return result;
}

QStringList PimDataProviderCalendar::getEvents() const
{
    QStringList result;
    KCalendarCore::ICalFormat formatter;
    const KCalendarCore::Event::List events = mCalendar->rawEvents();
    result.reserve(events.size());

    for (const auto &event : events) {
        result.append(formatter.toICalString(event));
    }
    return result;
}

QStringList PimDataProviderCalendar::getEventsRange(quint64 startTime, quint64 endTime) const
{
    QStringList result;
    KCalendarCore::ICalFormat formatter;

    const QDateTime startDt = QDateTime::fromSecsSinceEpoch(static_cast<qint64>(startTime), QTimeZone::systemTimeZone());
    const QDateTime endDt = QDateTime::fromSecsSinceEpoch(static_cast<qint64>(endTime), QTimeZone::systemTimeZone());

    const KCalendarCore::Event::List events = mCalendar->events(startDt.date(), endDt.date(), startDt.timeZone());
    result.reserve(events.size());

    for (const auto &event : events) {
        // mCalendar->events does not go above day precision when filtering events by date
        // so we have to apply a second filter ourselves for more precise filtering
        if (event->dtEnd() >= startDt && event->dtStart() <= endDt) {
            result.append(formatter.toICalString(event));
        }
    }
    return result;
}

void PimDataProviderCalendar::addEvent(const Akonadi::Item &item, const Akonadi::Collection &collection)
{
    Q_UNUSED(collection);
    if (!item.hasPayload<KCalendarCore::Event::Ptr>()) {
        return;
    }

    auto event = item.payload<KCalendarCore::Event::Ptr>();
    // We may get an itemChanged signal at the same time, or before the itemAdded signal (especially when reconnecting to a DAV server).
    // To prevent problems, only add the item if it does not already exist in the calendar.
    if (mCalendar->event(event->uid())) {
        return;
    }
    mCalendar->addEvent(event);
    mEventUIDs.insert(item.id(), event->uid());

    KCalendarCore::ICalFormat formatter;
    const QString ical = formatter.toICalString(event);
    Q_EMIT eventAdded(ical);
}

void PimDataProviderCalendar::updateEvent(const Akonadi::Item &item, const QSet<QByteArray> &partIdentifiers)
{
    Q_UNUSED(partIdentifiers);
    if (!item.hasPayload<KCalendarCore::Event::Ptr>()) {
        return;
    }

    auto modifiedEvent = item.payload<KCalendarCore::Event::Ptr>();

    // We may get an itemChanged signal at the same time, or before the itemAdded signal (especially when reconnecting to a DAV server).
    // To prevent problems, we add the item every time here, and delete it before if it already exists in the calendar.
    if (auto oldEvent = mCalendar->event(modifiedEvent->uid())) {
        // No way to easily change an instance in the MemoryCalendar, so we remove old instance and insert updated instance
        mCalendar->deleteEvent(oldEvent);
    }
    mCalendar->addEvent(modifiedEvent);

    KCalendarCore::ICalFormat formatter;
    const QString ical = formatter.toICalString(modifiedEvent);
    Q_EMIT eventChanged(ical);
}

void PimDataProviderCalendar::removeEvent(const Akonadi::Item &item)
{
    auto const uid = mEventUIDs.take(item.id());
    if (auto const event = mCalendar->event(uid)) {
        KCalendarCore::ICalFormat formatter;
        const QString ical = formatter.toICalString(event);

        mCalendar->deleteEvent(mCalendar->event(event->uid()));
        Q_EMIT eventDeleted(ical);
    }
}

void PimDataProviderCalendar::slotInitialEventsFetched(KJob *job)
{
    if (job->error()) {
        qCWarning(PIMDATAPROVIDER_LOG) << "Error fetching initial events: " << job->errorString();
        return;
    }
    const auto fetchJob = qobject_cast<Akonadi::ItemFetchJob *>(job);
    if (!fetchJob) {
        qCWarning(PIMDATAPROVIDER_LOG()) << "Error fetching initial events : could not get the result of the ItemFetchJob";
        return;
    }
    const Akonadi::Item::List items = fetchJob->items();
    KCalendarCore::ICalFormat formatter;

    for (const Akonadi::Item &item : items) {
        if (item.hasPayload<KCalendarCore::Event::Ptr>()) {
            auto event = item.payload<KCalendarCore::Event::Ptr>();
            mCalendar->addEvent(event);
            mEventUIDs.insert(item.id(), event->uid());
        }
    }
    const QString path = QStringLiteral("/org/pim/dataprovider/Calendar/Servers/") + encodeForDBusPath(mServerId) + QStringLiteral("/Calendar/")
        + encodeForDBusPath(mCalendarId);
    QDBusConnection::sessionBus().registerObject(path, this, QDBusConnection::ExportAdaptors);

    QVariantMap calendarMap;
    calendarMap.insert(QStringLiteral("path"), path);
    calendarMap.insert(QStringLiteral("id"), mCalendarId);
    calendarMap.insert(QStringLiteral("name"), mName);
    calendarMap.insert(QStringLiteral("color"), mColor);
    Q_EMIT calendarFinishedLoading(calendarMap);
}

} // namespace Akonadi

#include "moc_pimdataprovidercalendar.cpp"
