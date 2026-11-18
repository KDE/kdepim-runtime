/*
    SPDX-FileCopyrightText: 2026 Noham Devillers <noham.devillers@enioka.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "pimdataprovidercalendar.h"
#include "calendaradaptor.h"
#include "pimdataprovider_types.h"

#include <QDBusConnection>

namespace Akonadi
{
PimDataProviderCalendar::PimDataProviderCalendar(const QString &serverId, const QString &calendarId, QObject *parent)
    : QObject(parent)
    , mServerId(serverId)
    , mCalendarId(calendarId)
{
    new CalendarAdaptor(this);

    const QString path =
        QStringLiteral("/org/pim/dataprovider/Calendar/Servers/") + encodeForDBusPath(serverId) + QLatin1Char('/Calendar/') + encodeForDBusPath(calendarId);
    QDBusConnection::sessionBus().registerObject(path, this, QDBusConnection::ExportAdaptors);
}

PimDataProviderCalendar::~PimDataProviderCalendar()
{
    const QString path =
        QStringLiteral("/org/pim/dataprovider/Calendar/Servers/") + encodeForDBusPath(mServerId) + QLatin1Char('/Calendar/') + encodeForDBusPath(mCalendarId);
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
    return {};
}

QStringList PimDataProviderCalendar::getEvents() const
{
    return {};
}

QStringList PimDataProviderCalendar::getEventsRange(quint64 startTime, quint64 endTime) const
{
    return {};
}

} // namespace Akonadi

#include "moc_pimdataprovidercalendar.cpp"
