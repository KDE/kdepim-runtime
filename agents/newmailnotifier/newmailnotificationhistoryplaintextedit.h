/*
    SPDX-FileCopyrightText: 2023 Laurent Montel <montel@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <KPIMTextEdit/PlainTextEditorWidget>
#include <QPlainTextEdit>

class NewMailNotificationHistoryPlainTextEdit : public KPIMTextEdit::PlainTextEditorWidget
{
    Q_OBJECT
public:
    explicit NewMailNotificationHistoryPlainTextEdit(QWidget *parent = nullptr);
    ~NewMailNotificationHistoryPlainTextEdit() override;

Q_SIGNALS:
    void clearHistory();
};
