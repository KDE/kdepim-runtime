/*
    SPDX-FileCopyrightText: 2026 Noham Devillers <noham.devillers@enioka.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

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
    explicit PimDataProviderCalendar(const QString &serverId, const QString &calendarId, QObject *parent = nullptr);
    ~PimDataProviderCalendar() override;

    [[nodiscard]] QString id() const;
    [[nodiscard]] QString name() const;
    [[nodiscard]] QString color() const;

    [[nodiscard]] QStringList getEvent(const QString &itemId) const;
    [[nodiscard]] QStringList getEvents() const;
    [[nodiscard]] QStringList getEventsRange(qulonglong startTime, qulonglong endTime) const;

Q_SIGNALS:
    void eventAdded(const QString &icalEvent);
    void eventDeleted(const QString &icalEvent);
    void eventChanged(const QString &icalEvent);

private:
    QString mServerId;
    QString mCalendarId;
    QString mName;
    QString mColor;
};
} // namespace Akonadi
