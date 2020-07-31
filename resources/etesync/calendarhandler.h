/*
 * Copyright (C) 2020 by Shashwat Jolly <shashwat.jolly@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
    typedef std::unique_ptr<CalendarHandler> Ptr;
    explicit CalendarHandler(EteSyncResource *resource);

protected:
    const QString mimeType();
    const QString etesyncCollectionType();
};

#endif
