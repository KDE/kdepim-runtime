/*
    SPDX-FileCopyrightText: 2023-2025 Laurent Montel <montel@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QWidget>
class QCheckBox;
class NewMailNotificationHistoryBrowserTextWidget;
class NewMailNotificationHistoryWidget : public QWidget
{
    Q_OBJECT
public:
    explicit NewMailNotificationHistoryWidget(QWidget *parent = nullptr);
    ~NewMailNotificationHistoryWidget() override;

private:
    void slotHistoryAdded(const QString &str);
    void slotEnableChanged(bool clicked);
    NewMailNotificationHistoryBrowserTextWidget *const mTextBrowser;
    QCheckBox *const mEnabledHistory;
};
