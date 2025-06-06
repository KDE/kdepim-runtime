/*
    SPDX-FileCopyrightText: 2023-2025 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QObject>

class NewMailNotificationHistoryManagerTest : public QObject
{
    Q_OBJECT
public:
    explicit NewMailNotificationHistoryManagerTest(QObject *parent = nullptr);
    ~NewMailNotificationHistoryManagerTest() override = default;
private Q_SLOTS:
    void shouldHaveDefaultValues();
    void generateHtmlFromUniqueEmail();
    void generateHtmlFromFolders();
};
