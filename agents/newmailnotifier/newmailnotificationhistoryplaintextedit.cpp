/*
    SPDX-FileCopyrightText: 2023 Laurent Montel <montel@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "newmailnotificationhistoryplaintextedit.h"

#include <KLocalizedString>
#include <KStandardAction>

#include <QMenu>

NewMailNotificationHistoryPlainTextEdit::NewMailNotificationHistoryPlainTextEdit(QWidget *parent)
    : KPIMTextEdit::PlainTextEditorWidget(parent)
{
}

NewMailNotificationHistoryPlainTextEdit::~NewMailNotificationHistoryPlainTextEdit() = default;

#if 0
void NewMailNotificationHistoryPlainTextEdit::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu *popup = createStandardContextMenu();
    if (popup) {
        popup->addSeparator();
        QAction *clearAllAction = KStandardAction::clear(this, &NewMailNotificationHistoryPlainTextEdit::clear, popup);
        popup->addAction(clearAllAction);
    }
    popup->exec(event->globalPos());
    delete popup;
}
#endif
