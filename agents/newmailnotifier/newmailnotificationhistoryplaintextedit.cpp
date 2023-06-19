/*
    SPDX-FileCopyrightText: 2023 Laurent Montel <montel@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "newmailnotificationhistoryplaintextedit.h"

#include <KLocalizedString>

#include <QMenu>

NewMailNotificationHistoryPlainTextEdit::NewMailNotificationHistoryPlainTextEdit(QWidget *parent)
    : QPlainTextEdit(parent)
{
}

NewMailNotificationHistoryPlainTextEdit::~NewMailNotificationHistoryPlainTextEdit() = default;

void NewMailNotificationHistoryPlainTextEdit::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu *popup = createStandardContextMenu();
    if (popup) {
        // TODO
    }
    popup->exec(event->globalPos());
    delete popup;
}
