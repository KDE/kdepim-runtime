/*
    SPDX-FileCopyrightText: 2023 Laurent Montel <montel@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QPlainTextEdit>

class NewMailNotificationHistoryPlainTextEdit : public QPlainTextEdit
{
    Q_OBJECT
public:
    explicit NewMailNotificationHistoryPlainTextEdit(QWidget *parent = nullptr);
    ~NewMailNotificationHistoryPlainTextEdit() override;
};
