/*
    SPDX-FileCopyrightText: 2026 Noham Devillers <noham.devillers@enioka.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "pimdataprovidercalendar.h"
#include "calendaradaptor.h"

#include <QDBusConnection>

namespace Akonadi
{
PimDataProviderCalendar::PimDataProviderCalendar(const QString &serverId, const QString &calendarId, QObject *parent)
    : QObject(parent)
    , mServerId(serverId)
    , mCalendarId(calendarId)
{
    new CalendarAdaptor(this);

    const QString path = QStringLiteral("/org/pim/dataprovider/Calendar/Servers/") + serverId + QLatin1Char('/') + calendarId;
    QDBusConnection::sessionBus().registerObject(path, this, QDBusConnection::ExportAdaptors);
}

PimDataProviderCalendar::~PimDataProviderCalendar() = default;

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

QStringList PimDataProviderCalendar::getEvent(const QString &itemId) const
{
    return {};
}

QStringList PimDataProviderCalendar::getEvents() const
{
    return {};
}

QStringList PimDataProviderCalendar::getEventsRange(qulonglong startTime, qulonglong endTime) const
{
    return {};
}

} // namespace Akonadi

#include "moc_pimdataprovidercalendar.cpp"
