/*
    SPDX-FileCopyrightText: 2023-2026 Laurent Montel <montel@kde.org>

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
    QVERIFY(w.joinHistory().isEmpty());
}

void NewMailNotificationHistoryManagerTest::generateHtmlFromUniqueEmail()
{
    NewMailNotificationHistoryManager w;
    w.setTestModeEnabled(true);
    NewMailNotificationHistoryManager::HistoryMailInfo info;
    info.message = QStringLiteral("Foo bla");
    info.identifier = 45;
    w.addEmailInfoNotificationHistory(info);
    QString reference = QStringLiteral("<b> %1 </b><br>%2 <a href=\"openmail:%3\">[Show Mail]</a><br>")
                            .arg(QDate::currentDate().toString(), info.message, QString::number(info.identifier));
    QCOMPARE(w.joinHistory(), reference);

    info.message = QStringLiteral("Mail 2");
    info.identifier = 55;
    w.addEmailInfoNotificationHistory(info);

    const QString betweenTwoMail = QStringLiteral("<br>");
    reference += betweenTwoMail;
    reference += QStringLiteral("<b> %1 </b><br>%2 <a href=\"openmail:%3\">[Show Mail]</a><br>")
                     .arg(QDate::currentDate().toString(), info.message, QString::number(info.identifier));
    QCOMPARE(w.joinHistory(), reference);
}

void NewMailNotificationHistoryManagerTest::generateHtmlFromFolders()
{
    NewMailNotificationHistoryManager w;
    w.setTestModeEnabled(true);

    NewMailNotificationHistoryManager::HistoryFolderInfo info;
    info.message = QStringLiteral("Foo bla");
    info.identifier = 45;
    w.addFoldersInfoNotificationHistory({info});

    QString reference = QStringLiteral("<b> %1 </b><br>%2 <a href=\"openfolder:%3\">[Open Folder]</a><br>")
                            .arg(QDate::currentDate().toString(), info.message, QString::number(info.identifier));
    QCOMPARE(w.joinHistory(), reference);
}

void NewMailNotificationHistoryManagerTest::shouldDropOldestEntries()
{
    NewMailNotificationHistoryManager w;
    w.setTestModeEnabled(true);
    w.setMaximumHistorySize(2);

    NewMailNotificationHistoryManager::HistoryMailInfo info;
    for (int i = 1; i <= 4; ++i) {
        info.message = QStringLiteral("Mail %1").arg(i);
        info.identifier = i;
        w.addEmailInfoNotificationHistory(info);
    }

    // Only the last two notifications are kept, each one still carrying its own header.
    const QString entry = QStringLiteral("<b> %1 </b><br>Mail %2 <a href=\"openmail:%2\">[Show Mail]</a><br>");
    const QString reference = entry.arg(QDate::currentDate().toString(), QString::number(3)) + QStringLiteral("<br>")
        + entry.arg(QDate::currentDate().toString(), QString::number(4));
    QCOMPARE(w.joinHistory(), reference);
}

#include "moc_newmailnotificationhistorymanagertest.cpp"
