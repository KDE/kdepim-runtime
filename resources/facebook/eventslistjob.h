/*
 *    Copyright (C) 2017 Daniel Vrátil <dvratil@kde.org>
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef FACEBOOK_EVENTSLISTJOB_H_
#define FACEBOOK_EVENTSLISTJOB_H_

#include "listjob.h"

#include <KCalendarCore/Incidence>

class EventsListJob : public ListJob
{
    Q_OBJECT

public:
    explicit EventsListJob(const QString &identifier, const Akonadi::Collection &col, QObject *parent = nullptr);
    ~EventsListJob() override;

protected:
    Akonadi::Item handleResponse(const QJsonObject &data) override;

private:
    QDateTime parseDateTime(const QString &dt) const;
    bool shouldHaveAlarm(const Akonadi::Collection &collection) const;
    KCalendarCore::Incidence::Status parseStatus(const QJsonObject &data) const;
};

#endif
