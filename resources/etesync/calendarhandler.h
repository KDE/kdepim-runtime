/*
 * SPDX-FileCopyrightText: 2020 Shashwat Jolly <shashwat.jolly@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef CALENDARHANDLER_H
#define CALENDARHANDLER_H

#include <KCalendarCore/Event>

#include "calendartaskbasehandler.h"
#include "etesyncadapter.h"
#include "etesyncclientstate.h"

class EteSyncResource;

class CalendarHandler : public CalendarTaskBaseHandler
{
    Q_OBJECT
public:
    explicit CalendarHandler(EteSyncResource *resource);

    const QString mimeType() override;

protected:
    const QString etesyncCollectionType() override;
};

#endif
