/*
 * SPDX-FileCopyrightText: 2020 Shashwat Jolly <shashwat.jolly@gmail.com>
 * 
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "calendarhandler.h"

CalendarHandler::CalendarHandler(EteSyncResource *resource) : CalendarTaskBaseHandler(resource)
{
}

const QString CalendarHandler::mimeType()
{
    return KCalendarCore::Event::eventMimeType();
}
const QString CalendarHandler::etesyncCollectionType()
{
    return QStringLiteral(ETESYNC_COLLECTION_TYPE_CALENDAR);
}
