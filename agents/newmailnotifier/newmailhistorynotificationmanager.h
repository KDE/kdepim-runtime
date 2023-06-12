/*
    SPDX-FileCopyrightText: 2023 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#pragma once

#include <QObject>

class NewMailHistoryNotificationManager : public QObject
{
    Q_OBJECT
public:
    explicit NewMailHistoryNotificationManager(QObject *parent = nullptr);
    ~NewMailHistoryNotificationManager() override;

    static NewMailHistoryNotificationManager *self();
};
