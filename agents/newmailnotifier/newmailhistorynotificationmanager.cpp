/*
    SPDX-FileCopyrightText: 2023 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "newmailhistorynotificationmanager.h"
#include <QDateTime>

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

void NewMailHistoryNotificationManager::addHistory(QString str)
{
    mHistory += QDateTime::currentDateTime().toString() + QLatin1Char('\n');
    mHistory += str.replace(QStringLiteral("<br>"), QStringLiteral("\n"));
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
