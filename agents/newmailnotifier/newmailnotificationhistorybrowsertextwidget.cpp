/*
    SPDX-FileCopyrightText: 2023-2024 Laurent Montel <montel.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "newmailnotificationhistorybrowsertextwidget.h"
#include "newmailnotificationhistorybrowsertext.h"

NewMailNotificationHistoryBrowserTextWidget::NewMailNotificationHistoryBrowserTextWidget(NewMailNotificationHistoryBrowserText *editor, QWidget *parent)
    : TextCustomEditor::RichTextBrowserWidget(editor, parent)
{
    connect(editor, &NewMailNotificationHistoryBrowserText::clearHistory, this, &NewMailNotificationHistoryBrowserTextWidget::clear);
}

NewMailNotificationHistoryBrowserTextWidget::~NewMailNotificationHistoryBrowserTextWidget() = default;

#include "moc_newmailnotificationhistorybrowsertextwidget.cpp"
