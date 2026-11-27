/*
    SPDX-FileCopyrightText: 2026 Noham Devillers <noham.devillers@enioka.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "pimdataproviderserver.h"
#include "pimdataprovider_debug.h"
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
{
    new ServerAdaptor(this);

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
        const QString path = QStringLiteral("/org/pim/dataprovider/Calendar/Servers/") + encodeForDBusPath(mServerId) + QStringLiteral("/Calendar/")
            + encodeForDBusPath(calendar->id());
        QVariantMap calendarMap;
        calendarMap.insert(QStringLiteral("path"), path);
        calendarMap.insert(QStringLiteral("id"), calendar->id());
        calendarMap.insert(QStringLiteral("name"), calendar->name());
        calendarMap.insert(QStringLiteral("color"), calendar->color());
        result.append(calendarMap);
    }
    return result;
}

void PimDataProviderServer::addCalendar(const Collection &collection)
{
    // prevent creating a service for the root collection of the resource
    if (collection.resource() == collection.remoteId()) {
        return;
    }

    if (mCalendars.contains(collection.remoteId())) {
        return;
    }

    const QString calendarId = collection.remoteId();

    QString name = collection.name();
    auto displayNameAttribute = collection.attribute<Akonadi::EntityDisplayAttribute>();
    if (displayNameAttribute && !displayNameAttribute->displayName().isEmpty()) {
        name = displayNameAttribute->displayName();
    }
    QString color;
    auto colorAttribute = collection.attribute<Akonadi::CollectionColorAttribute>();
    if (colorAttribute && colorAttribute->color().isValid()) {
        color = colorAttribute->color().name(QColor::HexArgb);
    }

    auto const calendar = std::make_shared<PimDataProviderCalendar>(mServerId, collection);
    calendar->setName(name);
    calendar->setColor(color);
    mCalendars.insert(calendarId, calendar);

    const QString path = QStringLiteral("/org/pim/dataprovider/Calendar/Servers/") + encodeForDBusPath(mServerId) + QStringLiteral("/Calendar/")
        + encodeForDBusPath(calendar->id());
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

void PimDataProviderServer::updateCalendar(const Collection &collection)
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
        color = colorAttribute->color().name(QColor::HexArgb);
    }

    calendar->setName(name);
    calendar->setColor(color);

    const QString calendarId = calendar->id();
    const QString path = QStringLiteral("/org/pim/dataprovider/Calendar/Servers/") + encodeForDBusPath(mServerId) + QStringLiteral("/Calendar/")
        + encodeForDBusPath(calendar->id());
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

void PimDataProviderServer::removeCalendar(const Collection &collection)
{
    if (auto calendar = mCalendars.take(collection.remoteId())) {
        const QString calendarId = calendar->id();
        Q_EMIT calendarDeleted(mServerId, calendarId);
    }
}
void PimDataProviderServer::addEvent(const Item &item, const Collection &collection)
{
    if (auto const calendar = mCalendars.value(collection.remoteId())) {
        calendar->addEvent(item, collection);
    }
}

void PimDataProviderServer::updateEvent(const Item &item, const QSet<QByteArray> &partIdentifiers)
{
    if (auto const calendar = mCalendars.value(item.parentCollection().remoteId())) {
        calendar->updateEvent(item, partIdentifiers);
    }
}

void PimDataProviderServer::removeEvent(const Item &item)
{
    if (auto const calendar = mCalendars.value(item.parentCollection().remoteId())) {
        calendar->removeEvent(item);
    }
}
void PimDataProviderServer::moveEvent(const Item &item, const Collection &collectionSource, const Collection &collectionDestination)
{
    auto const sourceCalendar = mCalendars.value(collectionSource.remoteId());
    auto const destinationCalendar = mCalendars.value(collectionDestination.remoteId());
    if (sourceCalendar && destinationCalendar) {
        sourceCalendar->removeEvent(item);
        destinationCalendar->addEvent(item, collectionDestination);
    }
}

void PimDataProviderServer::slotInitialCalendarsFetched(KJob *job)
{
    if (job->error()) {
        qCWarning(PIMDATAPROVIDER_LOG()) << "Error fetching initial calendars: " << job->errorString();
        return;
    }
    const auto fetchJob = qobject_cast<CollectionFetchJob *>(job);
    if (!fetchJob) {
        qCWarning(PIMDATAPROVIDER_LOG()) << "Error fetching initial calendars : could not get the result of the CollectionFetchJob";
        return;
    }
    const Collection::List collections = fetchJob->collections();
    for (const Collection &collection : collections) {
        const QString calendarId = collection.remoteId();
        // prevent creating a service for the root collection of the resource
        if (collection.resource() != calendarId) {
            QString name = collection.name();
            auto displayNameAttribute = collection.attribute<Akonadi::EntityDisplayAttribute>();
            if (displayNameAttribute && !displayNameAttribute->displayName().isEmpty()) {
                name = displayNameAttribute->displayName();
            }
            QString color;
            auto colorAttribute = collection.attribute<Akonadi::CollectionColorAttribute>();
            if (colorAttribute && colorAttribute->color().isValid()) {
                color = colorAttribute->color().name(QColor::HexArgb);
            }

            auto const calendar = std::make_shared<PimDataProviderCalendar>(mServerId, collection);
            calendar->setName(name);
            calendar->setColor(color);
            mCalendars.insert(calendarId, calendar);
        }
    }
    const QString path = QStringLiteral("/org/pim/dataprovider/Calendar/Servers/") + encodeForDBusPath(mServerId);
    QDBusConnection::sessionBus().registerObject(path, this, QDBusConnection::ExportAdaptors);
}

} // namespace Akonadi

#include "moc_pimdataproviderserver.cpp"
