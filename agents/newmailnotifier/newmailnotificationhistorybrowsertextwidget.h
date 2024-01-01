/*
    SPDX-FileCopyrightText: 2023-2024 Laurent Montel <montel.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <TextCustomEditor/RichTextBrowserWidget>
class NewMailNotificationHistoryBrowserText;
class NewMailNotificationHistoryBrowserTextWidget : public TextCustomEditor::RichTextBrowserWidget
{
    Q_OBJECT
public:
    explicit NewMailNotificationHistoryBrowserTextWidget(NewMailNotificationHistoryBrowserText *editor, QWidget *parent = nullptr);
    ~NewMailNotificationHistoryBrowserTextWidget() override;

Q_SIGNALS:
    void clearHistory();
};
