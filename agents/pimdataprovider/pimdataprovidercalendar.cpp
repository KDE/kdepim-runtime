/*
    SPDX-FileCopyrightText: 2026 Noham Devillers <noham.devillers@enioka.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "pimdataprovidercalendar.h"
#include "calendaradaptor.h"
#include "pimdataprovider_types.h"

#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemFetchScope>
#include <KCalendarCore/ICalFormat>
#include <QDBusConnection>

namespace Akonadi
{
PimDataProviderCalendar::PimDataProviderCalendar(const QString &serverId, const QString &calendarId, const Collection &collection, QObject *parent)
    : QObject(parent)
    , mServerId(serverId)
    , mCalendarId(calendarId)
    , mMonitor(new Monitor(this))
    , mCalendar(KCalendarCore::MemoryCalendar::Ptr::create(QTimeZone::systemTimeZone()))
{
    new CalendarAdaptor(this);

    const QString path =
        QStringLiteral("/org/pim/dataprovider/Calendar/Servers/") + encodeForDBusPath(serverId) + QStringLiteral("/Calendar/") + encodeForDBusPath(calendarId);
    QDBusConnection::sessionBus().registerObject(path, this, QDBusConnection::ExportAdaptors);

    // Event management : we use the akonadi monitor
    mMonitor->setCollectionMonitored(collection);
    mMonitor->setMimeTypeMonitored(KCalendarCore::Event::eventMimeType());
    mMonitor->itemFetchScope().fetchFullPayload(true);
    mMonitor->itemFetchScope().fetchAllAttributes(true);

    // 4. Connect change signals
    connect(mMonitor, &Monitor::itemAdded, this, &PimDataProviderCalendar::slotItemAdded);
    connect(mMonitor, &Monitor::itemChanged, this, &PimDataProviderCalendar::slotItemChanged);
    connect(mMonitor, &Monitor::itemRemoved, this, &PimDataProviderCalendar::slotItemRemoved);

    // Get the list of already registered events
    auto job = new Akonadi::ItemFetchJob(collection, this);
    job->fetchScope().fetchFullPayload(true);
    job->fetchScope().fetchAllAttributes(true);
    connect(job, &KJob::result, this, &PimDataProviderCalendar::slotInitialCalendarsFetched);
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

    if (event) {
        result.append(formatter.toICalString(event));

        // Add event exceptions
        const auto instances = mCalendar->instances(event);
        for (const auto &instance : instances) {
            if (instance != event) {
                result.append(formatter.toICalString(instance));
            }
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

void PimDataProviderCalendar::slotItemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection)
{
    Q_UNUSED(collection);
    if (!item.hasPayload<KCalendarCore::Event::Ptr>()) {
        return;
    }

    auto event = item.payload<KCalendarCore::Event::Ptr>();
    mCalendar->addEvent(event);

    KCalendarCore::ICalFormat formatter;
    const QString ical = formatter.toICalString(event);
    Q_EMIT eventAdded(ical);
}

void PimDataProviderCalendar::slotItemChanged(const Akonadi::Item &item, const QSet<QByteArray> &partIdentifiers)
{
    Q_UNUSED(partIdentifiers);
    if (!item.hasPayload<KCalendarCore::Event::Ptr>()) {
        return;
    }

    auto modifiedEvent = item.payload<KCalendarCore::Event::Ptr>();

    if (auto oldEvent = mCalendar->event(modifiedEvent->uid())) {
        // No way to easily change an instance in the MemoryCalendar, so we remove old instance and insert updated instance
        mCalendar->deleteEvent(oldEvent);
        mCalendar->addEvent(modifiedEvent);

        KCalendarCore::ICalFormat formatter;
        const QString ical = formatter.toICalString(modifiedEvent);
        Q_EMIT eventChanged(ical);
    }
}

void PimDataProviderCalendar::slotItemRemoved(const Akonadi::Item &item)
{
    if (!item.hasPayload<KCalendarCore::Event::Ptr>()) {
        return;
    }

    auto uid = item.payload<KCalendarCore::Event::Ptr>()->uid();

    if (auto event = mCalendar->event(uid)) {
        KCalendarCore::ICalFormat formatter;
        const QString ical = formatter.toICalString(event);

        mCalendar->deleteEvent(mCalendar->event(event->uid()));
        Q_EMIT eventDeleted(ical);
    }
}

void PimDataProviderCalendar::slotInitialCalendarsFetched(KJob *job)
{
    if (job->error()) {
        return;
    }
    const auto fetchJob = qobject_cast<Akonadi::ItemFetchJob *>(job);
    const Akonadi::Item::List items = fetchJob->items();
    KCalendarCore::ICalFormat formatter;

    for (const Akonadi::Item &item : items) {
        if (item.hasPayload<KCalendarCore::Event::Ptr>()) {
            auto event = item.payload<KCalendarCore::Event::Ptr>();
            mCalendar->addEvent(event);
        }
    }
}

} // namespace Akonadi

#include "moc_pimdataprovidercalendar.cpp"
