/*
 * SPDX-FileCopyrightText: 2020 Shashwat Jolly <shashwat.jolly@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "taskhandler.h"

TaskHandler::TaskHandler(EteSyncResource *resource) : CalendarTaskBaseHandler(resource)
{
}

const QString TaskHandler::mimeType()
{
    return KCalendarCore::Todo::todoMimeType();
}

const QString TaskHandler::etesyncCollectionType()
{
    return QStringLiteral(ETESYNC_COLLECTION_TYPE_TASKS);
}
