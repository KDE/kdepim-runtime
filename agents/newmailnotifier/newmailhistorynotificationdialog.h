/*
    SPDX-FileCopyrightText: 2023 Laurent Montel <montel@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#pragma once

#include <QDialog>
class NewMailHistoryNotificationWidget;
class NewMailHistoryNotificationDialog : public QDialog
{
    Q_OBJECT
public:
    explicit NewMailHistoryNotificationDialog(QWidget *parent = nullptr);
    ~NewMailHistoryNotificationDialog() override;

private:
    NewMailHistoryNotificationWidget *const mNewHistoryNotificationWidget;
};
