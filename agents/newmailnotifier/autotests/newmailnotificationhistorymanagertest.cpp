/*
    SPDX-FileCopyrightText: 2023 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "newmailnotificationhistorymanagertest.h"
#include "newmailnotificationhistorymanager.h"
#include <QTest>
QTEST_MAIN(NewMailNotificationHistoryManagerTest)
NewMailNotificationHistoryManagerTest::NewMailNotificationHistoryManagerTest(QObject *parent)
    : QObject{parent}
{
}

void NewMailNotificationHistoryManagerTest::shouldHaveDefaultValues()
{
    NewMailNotificationHistoryManager w;
    QVERIFY(w.history().isEmpty());
}
