/*
    SPDX-FileCopyrightText: 2026 Noham Devillers <noham.devillers@enioka.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <Akonadi/Monitor>
#include <KCalendarCore/MemoryCalendar>
#include <QObject>
#include <QString>
#include <QStringList>

namespace Akonadi
{
class PimDataProviderCalendar : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString id READ id)
    Q_PROPERTY(QString name READ name)
    Q_PROPERTY(QString color READ color)

public:
    explicit PimDataProviderCalendar(const QString &serverId, const Collection &collection, QObject *parent = nullptr);
    ~PimDataProviderCalendar() override;
    PimDataProviderCalendar(const PimDataProviderCalendar &) = delete;
    PimDataProviderCalendar &operator=(const PimDataProviderCalendar &) = delete;
    PimDataProviderCalendar(PimDataProviderCalendar &&) = delete;
    PimDataProviderCalendar &operator=(PimDataProviderCalendar &&) = delete;

    [[nodiscard]] QString id() const;
    /**
     * Return the display name of the calendar
     * This property is optional
     */
    [[nodiscard]] QString name() const;
    /**
     * Return the color of the calendar
     * The color is in the format HexArgb
     * This property is optional
     */
    [[nodiscard]] QString color() const;

    void setName(const QString &name);
    void setColor(const QString &color);

    /**
     * Return the event with the given id (with its exceptions)
     * The event is returned in the ical format
     */
    [[nodiscard]] QStringList getEvent(const QString &itemId) const;
    /**
     * Return all the events of the calendar
     * The events are returned in the ical format
     */
    [[nodiscard]] QStringList getEvents() const;
    /**
     * Return all the events of the calendar within the given time range
     * The events are returned in the ical format
     */
    [[nodiscard]] QStringList getEventsRange(quint64 startTime, quint64 endTime) const;

    void addEvent(const Item &item, const Collection &collection);
    void updateEvent(const Item &item, const QSet<QByteArray> &partIdentifiers);
    void removeEvent(const Item &item);

Q_SIGNALS:
    void eventAdded(const QString &icalEvent);
    void eventDeleted(const QString &icalEvent);
    void eventChanged(const QString &icalEvent);

private:
    void slotInitialEventsFetched(KJob *job);

    QString mServerId;
    QString mCalendarId;
    QString mName;
    QString mColor;

    KCalendarCore::MemoryCalendar::Ptr mCalendar;
    /**
     * Used to map the akonadi ID to the event UID
     * This is necessary because we don't get the payload, nor the remoteId of the item in the delete slot
     */
    QMap<Akonadi::Item::Id, QString> mEventUIDs;
};
} // namespace Akonadi
