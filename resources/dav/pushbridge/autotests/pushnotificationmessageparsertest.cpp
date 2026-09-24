/*
SPDX-FileCopyrightText: 2026 Benjamin Port <benjamin.port@enioka.com>
SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "../pushnotificationmessageparser.h"

#include "../../../../../../../../../../usr/include/qt6/QtTest/qtestcase.h"

#include <QByteArray>
#include <QObject>
#include <QTest>

using namespace Qt::Literals;

class PushNotificationMessageParserTest : public QObject
{
    Q_OBJECT

private:
    QByteArray readFile(const QString &fileName)
    {
        QByteArray bytes;
        QFile file(QDir(QString::fromLatin1(TEST_DATA_DIR)).filePath(fileName));
        auto result = file.open(QIODevice::ReadOnly);
        return file.readAll();
    }

private Q_SLOTS:
    void testInvalidPayload()
    {
        PushNotificationMessageParser parser("<"_ba);
        QVERIFY(!parser.isValid);
    }

    void testContentUpdatePayload()
    {
        PushNotificationMessageParser parser(readFile("contentUpdate.xml"_L1));
        QVERIFY(parser.isValid);
        QVERIFY(parser.isContentUpdate);
        QVERIFY(!parser.isPropertyUpdate);
        QVERIFY(!parser.isVapidKeyUpdate);
        QCOMPARE(parser.topic, "O7M1nQ7cKkKTKsoS_j6Z3w"_L1);
        QCOMPARE(parser.syncToken, "http://example.com/sync/10"_L1);
    }

    void testPropertyUpdate()
    {
        PushNotificationMessageParser parser(readFile("propertyUpdate.xml"_L1));
        QVERIFY(parser.isValid);
        QVERIFY(parser.isPropertyUpdate);
        QVERIFY(!parser.isContentUpdate);
        QVERIFY(!parser.isVapidKeyUpdate);
        QCOMPARE(parser.topic, "O7M1nQ7cKkKTKsoS_j6Z3w"_L1);
        QVERIFY(parser.syncToken.isEmpty());
    }

    void testVapidKeyUpdate()
    {
        PushNotificationMessageParser parser(readFile("vapidKeyUpdate.xml"_L1));
        QVERIFY(parser.isValid);
        QVERIFY(parser.isVapidKeyUpdate);
        QVERIFY(!parser.isContentUpdate);
        QVERIFY(!parser.isPropertyUpdate);
        QVERIFY(parser.topic.isEmpty());
        QVERIFY(parser.syncToken.isEmpty());
    }
};

QTEST_GUILESS_MAIN(PushNotificationMessageParserTest)

#include "pushnotificationmessageparsertest.moc"
