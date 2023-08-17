/*
    SPDX-FileCopyrightText: 2023 Laurent Montel <montel@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <TextCustomEditor/PlainTextEditorWidget>
class NewMailNotificationHistoryPlainTextEditor;
class NewMailNotificationHistoryPlainTextEdit : public TextCustomEditor::PlainTextEditorWidget
{
    Q_OBJECT
public:
    explicit NewMailNotificationHistoryPlainTextEdit(NewMailNotificationHistoryPlainTextEditor *editor, QWidget *parent = nullptr);
    ~NewMailNotificationHistoryPlainTextEdit() override;

Q_SIGNALS:
    void clearHistory();
};
