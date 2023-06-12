/*
    SPDX-FileCopyrightText: 2023 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "newmailhistorynotificationmanager.h"

NewMailHistoryNotificationManager::NewMailHistoryNotificationManager(QObject *parent)
    : QObject{parent}
{
}

NewMailHistoryNotificationManager::~NewMailHistoryNotificationManager() = default;

NewMailHistoryNotificationManager *NewMailHistoryNotificationManager::self()
{
    static NewMailHistoryNotificationManager s_self;
    return &s_self;
}
