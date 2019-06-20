/*
   Copyright (c) 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
   Author: Kevin Ottens <kevin@kdab.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or ( at your option ) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include "imaptestbase.h"

#include "retrieveitemtask.h"
#include <QTest>
class TestRetrieveItemTask : public ImapTestBase
{
    Q_OBJECT

private Q_SLOTS:
    void shouldFetchMessage_data()
    {
        QTest::addColumn<Akonadi::Item>("item");
        QTest::addColumn<QString>("message");
        QTest::addColumn< QList<QByteArray> >("scenario");
        QTest::addColumn<QString>("callName");

        Akonadi::Collection collection;
        Akonadi::Item item;
        QString message;
        QList<QByteArray> scenario;

        collection = createCollectionChain(QStringLiteral("/INBOX/Foo"));
        item = Akonadi::Item(2);
        item.setParentCollection(collection);
        item.setRemoteId(QStringLiteral("42"));

        message = QStringLiteral("From: ervin\nTo: someone\nSubject: foo\n\nSpeechless...");

        scenario.clear();
        scenario << defaultPoolConnectionScenario()
                 << "C: A000003 SELECT \"INBOX/Foo\""
                 << "S: A000003 OK select done"
                 << "C: A000004 UID FETCH 42 (BODY.PEEK[] UID)"
                 << "S: * 10 FETCH (UID 42 BODY[] \"From: ervin\nTo: someone\nSubject: foo\n\nSpeechless...\")"
                 << "S: A000004 OK fetch done";
        QTest::newRow("normal case") << item << message << scenario << "itemRetrieved";

        scenario.clear();
        scenario << defaultPoolConnectionScenario()
                 << "C: A000003 SELECT \"INBOX/Foo\""
                 << "S: A000003 NO select fail";
        QTest::newRow("select fail") << item << message << scenario << "cancelTask";

        scenario.clear();
        scenario << defaultPoolConnectionScenario()
                 << "C: A000003 SELECT \"INBOX/Foo\""
                 << "S: A000003 OK select done"
                 << "C: A000004 UID FETCH 42 (BODY.PEEK[] UID)"
                 << "S: A000004 NO fetch failed";
        QTest::newRow("fetch fail") << item << message << scenario << "cancelTask";
    }

    void shouldFetchMessage()
    {
        QFETCH(Akonadi::Item, item);
        QFETCH(QString, message);
        QFETCH(QList<QByteArray>, scenario);
        QFETCH(QString, callName);

        FakeServer server;
        server.setScenario(scenario);
        server.startAndWait();

        SessionPool pool(1);

        pool.setPasswordRequester(createDefaultRequester());
        QVERIFY(pool.connect(createDefaultAccount()));
        QVERIFY(waitForSignal(&pool, SIGNAL(connectDone(int,QString))));

        DummyResourceState::Ptr state = DummyResourceState::Ptr(new DummyResourceState);
        state->setItem(item);
        RetrieveItemTask *task = new RetrieveItemTask(state);
        task->start(&pool);

        QTRY_COMPARE(state->calls().count(), 1);

        QString command = QString::fromUtf8(state->calls().first().first);
        if (command == QLatin1String("cancelTask") && callName != QLatin1String("cancelTask")) {
            qDebug() << "Got a cancel:" << state->calls().first().second.toString();
        }
        QCOMPARE(command, callName);

        QVariant parameter = state->calls().first().second;

        if (callName == QLatin1String("itemRetrieved")) {
            QCOMPARE(parameter.value<Akonadi::Item>().id(), item.id());
            QCOMPARE(parameter.value<Akonadi::Item>().remoteId(), item.remoteId());

            QString payload = QString::fromUtf8(parameter.value<Akonadi::Item>().payload<KMime::Message::Ptr>()->encodedContent());

            QCOMPARE(payload, message);
        } else if (callName == QLatin1String("cancelTask")) {
            QVERIFY(!parameter.toString().isEmpty());
        } else {
            QFAIL(QStringLiteral("Unexpected call type: %1").arg(callName).toUtf8().constData());
        }

        QVERIFY(server.isAllScenarioDone());

        server.quit();
    }
};

QTEST_GUILESS_MAIN(TestRetrieveItemTask)

#include "testretrieveitemtask.moc"
