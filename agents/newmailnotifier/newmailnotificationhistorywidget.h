/*
    SPDX-FileCopyrightText: 2023 Laurent Montel <montel@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QWidget>
class NewMailNotificationHistoryPlainTextEdit;
class NewMailNotificationHistoryWidget : public QWidget
{
    Q_OBJECT
public:
    explicit NewMailNotificationHistoryWidget(QWidget *parent = nullptr);
    ~NewMailNotificationHistoryWidget() override;

private:
    void slotHistoryAdded(const QString &str);
    NewMailNotificationHistoryPlainTextEdit *const mPlainTextEdit;
};
