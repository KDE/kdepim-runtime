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

QStringList NewMailHistoryNotificationManager::history() const
{
    return mHistory;
}

void NewMailHistoryNotificationManager::addHistory(const QString &str)
{
    mHistory += str;
    Q_EMIT historyAdded(str);
}

void NewMailHistoryNotificationManager::setHistory(const QStringList &newHistory)
{
    mHistory = newHistory;
}

void NewMailHistoryNotificationManager::clear()
{
    mHistory.clear();
}