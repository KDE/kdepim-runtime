/*
 * SPDX-FileCopyrightText: 2020 Shashwat Jolly <shashwat.jolly@gmail.com>
 * 
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef TASKHANDLER_H
#define TASKHANDLER_H

#include <KCalendarCore/Todo>

#include "calendartaskbasehandler.h"
#include "etesyncadapter.h"
#include "etesyncclientstate.h"

class EteSyncResource;

class TaskHandler : public CalendarTaskBaseHandler
{
    Q_OBJECT
public:
    TaskHandler(EteSyncResource *resource);

    const QString mimeType() override;

protected:
    const QString etesyncCollectionType() override;
};

#endif
