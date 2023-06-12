/*
    SPDX-FileCopyrightText: 2023 Laurent Montel <montel@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QWidget>

class NewMailHistoryNotificationWidget : public QWidget
{
    Q_OBJECT
public:
    explicit NewMailHistoryNotificationWidget(QWidget *parent = nullptr);
    ~NewMailHistoryNotificationWidget() override;
};
