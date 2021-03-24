/*
 *    SPDX-FileCopyrightText: 2017 Daniel Vrátil <dvratil@kde.org>
 *
 *    SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

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

