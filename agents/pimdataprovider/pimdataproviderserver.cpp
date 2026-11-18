/*
    SPDX-FileCopyrightText: 2026 Noham Devillers <noham.devillers@enioka.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "pimdataproviderserver.h"
#include "pimdataprovidercalendar.h"
#include "serveradaptor.h"

#include <Akonadi/CollectionColorAttribute>
#include <Akonadi/CollectionFetchJob>
#include <Akonadi/CollectionFetchScope>
#include <Akonadi/EntityDisplayAttribute>
#include <KCalendarCore/Event>

#include <QColor>
#include <QDBusConnection>

namespace Akonadi
{
PimDataProviderServer::PimDataProviderServer(const QString &serverId, const QString &ressourceId, QObject *parent)
    : QObject(parent)
    , mServerId(serverId)
    , mMonitor(new Monitor(this))
{
    new ServerAdaptor(this);

    const QString path = QStringLiteral("/org/pim/dataprovider/Calendar/Servers/") + encodeForDBusPath(serverId);
    QDBusConnection::sessionBus().registerObject(path, this, QDBusConnection::ExportAdaptors);

    // Calendar management : we use the akonadi monitor
    mMonitor->setResourceMonitored(ressourceId.toLatin1(), true);
    connect(mMonitor, &Monitor::collectionAdded, this, &PimDataProviderServer::slotCalendarAdded);
    connect(mMonitor, qOverload<const Collection &>(&Monitor::collectionChanged), this, &PimDataProviderServer::slotCalendarUpdated);
    connect(mMonitor, &Monitor::collectionRemoved, this, &PimDataProviderServer::slotCalendarRemoved);

    // Get the list of already registered Calendars
    auto fetchJob = new CollectionFetchJob(Collection::root(), CollectionFetchJob::Recursive, this);
    fetchJob->fetchScope().setResource(ressourceId);
    fetchJob->fetchScope().setContentMimeTypes({KCalendarCore::Event::eventMimeType()});
    connect(fetchJob, &CollectionFetchJob::result, this, &PimDataProviderServer::slotInitialCalendarsFetched);
}

PimDataProviderServer::~PimDataProviderServer()
{
    const QString path = QStringLiteral("/org/pim/dataprovider/Calendar/Servers/") + encodeForDBusPath(mServerId);
    QDBusConnection::sessionBus().unregisterObject(path);
}

QString PimDataProviderServer::id() const
{
    return mServerId;
}

QList<QVariantMap> PimDataProviderServer::getCalendars() const
{
    QList<QVariantMap> result;
    result.reserve(mCalendars.size());
    for (const auto calendar : mCalendars) {
        const QString path = QStringLiteral("/org/pim/dataprovider/Calendar/Servers/") + mServerId + QLatin1Char('/Calendar/') + calendar->id();
        QVariantMap calendarMap;
        calendarMap.insert(QStringLiteral("path"), path);
        calendarMap.insert(QStringLiteral("id"), calendar->id());
        calendarMap.insert(QStringLiteral("name"), calendar->name());
        calendarMap.insert(QStringLiteral("color"), calendar->color());
        result.append(calendarMap);
    }
    return result;
}

void PimDataProviderServer::slotCalendarAdded(const Collection &collection)
{
    // prevent creating a service for the root collection of the resource
    if (collection.resource() == collection.remoteId()) {
        return;
    }

    if (mCalendars.contains(collection.remoteId())) {
        return;
    }

    const QString calendarId = collection.remoteId();
    auto calendar = new PimDataProviderCalendar(mServerId, calendarId, this);

    QString name = collection.name();
    auto displayNameAttribute = collection.attribute<Akonadi::EntityDisplayAttribute>();
    if (displayNameAttribute && !displayNameAttribute->displayName().isEmpty()) {
        name = displayNameAttribute->displayName();
    }
    QString color;
    auto colorAttribute = collection.attribute<Akonadi::CollectionColorAttribute>();
    if (colorAttribute && colorAttribute->color().isValid()) {
        color = colorAttribute->color().name(QColor::HexRgb);
    }

    calendar->setName(name);
    calendar->setColor(color);

    mCalendars.insert(calendarId, calendar);

    const QString path = QStringLiteral("/org/pim/dataprovider/Calendar/Servers/") + mServerId + QLatin1Char('/Calendar/') + calendarId;
    QVariantMap calendarMap;
    calendarMap.insert(QStringLiteral("path"), path);
    calendarMap.insert(QStringLiteral("id"), calendarId);
    calendarMap.insert(QStringLiteral("name"), name);
    calendarMap.insert(QStringLiteral("color"), color);

    PimDataProviderItem item;
    item.id = mServerId;
    item.data = calendarMap;

    Q_EMIT calendarCreated(item);
}

void PimDataProviderServer::slotCalendarUpdated(const Collection &collection)
{
    auto calendar = mCalendars.value(collection.remoteId());
    if (!calendar) {
        return;
    }

    QString name = collection.name();
    auto displayNameAttribute = collection.attribute<Akonadi::EntityDisplayAttribute>();
    if (displayNameAttribute && !displayNameAttribute->displayName().isEmpty()) {
        name = displayNameAttribute->displayName();
    }
    QString color;
    auto colorAttribute = collection.attribute<Akonadi::CollectionColorAttribute>();
    if (colorAttribute && colorAttribute->color().isValid()) {
        color = colorAttribute->color().name(QColor::HexRgb);
    }

    calendar->setName(name);
    calendar->setColor(color);

    const QString calendarId = calendar->id();
    const QString path = QStringLiteral("/org/pim/dataprovider/Calendar/Servers/") + mServerId + QLatin1Char('/Calendar/') + calendarId;
    QVariantMap calendarMap;
    calendarMap.insert(QStringLiteral("path"), path);
    calendarMap.insert(QStringLiteral("id"), calendarId);
    calendarMap.insert(QStringLiteral("name"), name);
    calendarMap.insert(QStringLiteral("color"), color);

    PimDataProviderItem item;
    item.id = mServerId;
    item.data = calendarMap;

    Q_EMIT calendarChanged(item);
}

void PimDataProviderServer::slotCalendarRemoved(const Collection &collection)
{
    if (auto calendar = mCalendars.take(collection.remoteId())) {
        const QString calendarId = calendar->id();
        calendar->deleteLater();
        Q_EMIT calendarDeleted(mServerId, calendarId);
    }
}

void PimDataProviderServer::slotInitialCalendarsFetched(KJob *job)
{
    if (job->error()) {
        return;
    }
    const auto fetchJob = qobject_cast<CollectionFetchJob *>(job);
    if (!fetchJob) {
        return;
    }
    const Collection::List collections = fetchJob->collections();
    for (const Collection &collection : collections) {
        const QString calendarId = collection.remoteId();
        // prevent creating a service for the root collection of the resource
        if (collection.resource() != calendarId) {
            auto calendar = new PimDataProviderCalendar(mServerId, calendarId, this);

            QString name = collection.name();
            auto displayNameAttribute = collection.attribute<Akonadi::EntityDisplayAttribute>();
            if (displayNameAttribute && !displayNameAttribute->displayName().isEmpty()) {
                name = displayNameAttribute->displayName();
            }
            QString color;
            auto colorAttribute = collection.attribute<Akonadi::CollectionColorAttribute>();
            if (colorAttribute && colorAttribute->color().isValid()) {
                color = colorAttribute->color().name(QColor::HexRgb);
            }

            calendar->setName(name);
            calendar->setColor(color);

            mCalendars.insert(calendarId, calendar);
        }
    }
}

} // namespace Akonadi

#include "moc_pimdataproviderserver.cpp"
