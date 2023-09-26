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

void NewMailNotificationHistoryManagerTest::generateHtmlFromUniqueEmail()
{
    NewMailNotificationHistoryManager w;
    w.setTestModeEnabled(true);
    NewMailNotificationHistoryManager::HistoryMailInfo info;
    info.message = QStringLiteral("Foo bla");
    info.identifier = 45;
    w.addEmailInfoNotificationHistory(info);
    QString reference = QStringLiteral("<b> %1 </b><br>%2 <a href=\"openMail:%3\">[Show Mail]</a>")
                            .arg(QDate::currentDate().toString())
                            .arg(info.message)
                            .arg(QString::number(info.identifier));
    QCOMPARE(w.history().join(QStringLiteral("<br>")), reference);

    info.message = QStringLiteral("Mail 2");
    info.identifier = 55;
    w.addEmailInfoNotificationHistory(info);

    const QString betweenTwoMail = QStringLiteral("<br><br><br>");
    reference += betweenTwoMail;
    reference += QStringLiteral("<b> %1 </b><br>%2 <a href=\"openMail:%3\">[Show Mail]</a>")
                     .arg(QDate::currentDate().toString())
                     .arg(info.message)
                     .arg(QString::number(info.identifier));
    QCOMPARE(w.history().join(QStringLiteral("<br>")), reference);
}

void NewMailNotificationHistoryManagerTest::generateHtmlFromFolders()
{
    NewMailNotificationHistoryManager w;
    w.setTestModeEnabled(true);

    NewMailNotificationHistoryManager::HistoryFolderInfo info;
    info.message = QStringLiteral("Foo bla");
    info.identifier = 45;
    w.addFoldersInfoNotificationHistory({info});

    QString reference = QStringLiteral("<b> %1 </b><br>%2 <a href=\"openFolder:%3\">[Open Folder]</a>")
                            .arg(QDate::currentDate().toString())
                            .arg(info.message)
                            .arg(QString::number(info.identifier));
    QCOMPARE(w.history().join(QStringLiteral("<br>")), reference);
}

#include "moc_newmailnotificationhistorymanagertest.cpp"
