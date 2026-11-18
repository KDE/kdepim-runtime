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
    explicit PimDataProviderCalendar(const QString &serverId, const QString &calendarId, const Collection &collection, QObject *parent = nullptr);
    ~PimDataProviderCalendar() override;

    [[nodiscard]] QString id() const;
    [[nodiscard]] QString name() const;
    [[nodiscard]] QString color() const;

    void setName(const QString &name);
    void setColor(const QString &color);

    [[nodiscard]] QStringList getEvent(const QString &itemId) const;
    [[nodiscard]] QStringList getEvents() const;
    [[nodiscard]] QStringList getEventsRange(quint64 startTime, quint64 endTime) const;

Q_SIGNALS:
    void eventAdded(const QString &icalEvent);
    void eventDeleted(const QString &icalEvent);
    void eventChanged(const QString &icalEvent);

private:
    void slotItemAdded(const Item &item, const Collection &collection);
    void slotItemChanged(const Item &item, const QSet<QByteArray> &partIdentifiers);
    void slotItemRemoved(const Item &item);

    void slotInitialCalendarsFetched(KJob *job);

    QString mServerId;
    QString mCalendarId;
    QString mName;
    QString mColor;

    Monitor *mMonitor = nullptr;
    KCalendarCore::MemoryCalendar::Ptr mCalendar;
};
} // namespace Akonadi
