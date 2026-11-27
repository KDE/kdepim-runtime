/*
    SPDX-FileCopyrightText: 2026 Noham Devillers <noham.devillers@enioka.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "pimdataprovider_types.h"
#include "pimdataprovidercalendar.h"

#include <Akonadi/Monitor>
#include <QVariantMap>

namespace Akonadi
{
class PimDataProviderServer : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString id READ id)

public:
    explicit PimDataProviderServer(const QString &serverId, const QString &resourceId, QObject *parent = nullptr);
    ~PimDataProviderServer() override;
    PimDataProviderServer(const PimDataProviderServer &) = delete;
    PimDataProviderServer &operator=(const PimDataProviderServer &) = delete;
    PimDataProviderServer(PimDataProviderServer &&) = delete;
    PimDataProviderServer &operator=(PimDataProviderServer &&) = delete;

    [[nodiscard]] QString id() const;
    [[nodiscard]] QList<QVariantMap> getCalendars() const;

    void addCalendar(const Akonadi::Collection &collection);
    void updateCalendar(const Akonadi::Collection &collection);
    void removeCalendar(const Akonadi::Collection &collection);

    void addEvent(const Item &item, const Collection &collection);
    void updateEvent(const Item &item, const QSet<QByteArray> &partIdentifiers);
    void removeEvent(const Item &item);
    void moveEvent(const Item &item, const Collection &collectionSource, const Collection &collectionDestination);

Q_SIGNALS:
    void calendarCreated(const PimDataProviderItem &calendar);
    void calendarChanged(const PimDataProviderItem &calendar);
    void calendarDeleted(const QString &serverId, const QString &calendarId);

private:
    void slotInitialCalendarsFetched(KJob *job);

    QString mServerId;
    QMap<QString, std::shared_ptr<PimDataProviderCalendar>> mCalendars;
};
} // namespace Akonadi
