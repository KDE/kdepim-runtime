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

#include "movecollectiontask.h"
#include <QTest>
class TestMoveCollectionTask : public ImapTestBase
{
    Q_OBJECT

private Q_SLOTS:
    void shouldRenameMailBox_data()
    {
        QTest::addColumn<Akonadi::Collection>("collection");
        QTest::addColumn<Akonadi::Collection>("source");
        QTest::addColumn<Akonadi::Collection>("target");
        QTest::addColumn< QList<QByteArray> >("scenario");
        QTest::addColumn<QStringList>("callNames");

        Akonadi::Collection root;
        Akonadi::Collection inbox;
        Akonadi::Collection collection;
        Akonadi::Collection source;
        Akonadi::Collection target;
        QList<QByteArray> scenario;
        QStringList callNames;

        root = createCollectionChain(QString());
        inbox = createCollectionChain(QStringLiteral("/INBOX"));

        source = Akonadi::Collection(3);
        source.setRemoteId(QStringLiteral("/Foo"));
        source.setParentCollection(inbox);

        collection = Akonadi::Collection(10);
        collection.setRemoteId(QStringLiteral("/Baz"));
        collection.setParentCollection(source);

        target = Akonadi::Collection(4);
        target.setRemoteId(QStringLiteral("/Bar"));
        target.setParentCollection(inbox);

        scenario.clear();
        scenario << defaultPoolConnectionScenario()
                 << "C: A000003 RENAME \"INBOX/Foo/Baz\" \"INBOX/Bar/Baz\""
                 << "S: A000003 OK rename done"
                 << "C: A000004 SUBSCRIBE \"INBOX/Bar/Baz\""
                 << "S: A000004 OK subscribe done";

        callNames.clear();
        callNames << QStringLiteral("collectionChangeCommitted");

        QTest::newRow("moving mailbox") << collection << source << target << scenario << callNames;

        {
            const Akonadi::Collection toplevel = createCollectionChain(QStringLiteral("/Bar"));

            scenario.clear();
            scenario << defaultPoolConnectionScenario()
                     << "C: A000003 RENAME \"Bar\" \"INBOX/Bar\""
                     << "S: A000003 OK rename done"
                     << "C: A000004 SUBSCRIBE \"INBOX/Bar\""
                     << "S: A000004 OK subscribe done";

            callNames.clear();
            callNames << QStringLiteral("collectionChangeCommitted");

            QTest::newRow("move mailbox from toplevel") << toplevel << root << inbox << scenario << callNames;
        }

        {
            const Akonadi::Collection toplevel = createCollectionChain(QStringLiteral("/INBOX/Bar"));

            scenario.clear();
            scenario << defaultPoolConnectionScenario()
                     << "C: A000003 RENAME \"INBOX/Bar\" \"Bar\""
                     << "S: A000003 OK rename done"
                     << "C: A000004 SUBSCRIBE \"Bar\""
                     << "S: A000004 OK subscribe done";

            callNames.clear();
            callNames << QStringLiteral("collectionChangeCommitted");

            QTest::newRow("move mailbox to toplevel") << toplevel << inbox << root << scenario << callNames;
        }

        // Same collections
        // The scenario changes though

        scenario.clear();
        scenario << defaultPoolConnectionScenario()
                 << "C: A000003 RENAME \"INBOX/Foo/Baz\" \"INBOX/Bar/Baz\""
                 << "S: A000003 OK rename done"
                 << "C: A000004 SUBSCRIBE \"INBOX/Bar/Baz\""
                 << "S: A000004 NO subscribe failed";

        callNames.clear();
        callNames << QStringLiteral("emitWarning") << QStringLiteral("collectionChangeCommitted");

        QTest::newRow("moving mailbox, subscribe fails") << collection << source << target << scenario << callNames;

        inbox = createCollectionChain(QStringLiteral(".INBOX"));

        source = Akonadi::Collection(3);
        source.setRemoteId(QStringLiteral(".Foo"));
        source.setParentCollection(inbox);

        collection = Akonadi::Collection(10);
        collection.setRemoteId(QStringLiteral(".Baz"));
        collection.setParentCollection(source);

        target = Akonadi::Collection(4);
        target.setRemoteId(QStringLiteral(".Bar"));
        target.setParentCollection(inbox);

        scenario.clear();
        scenario << defaultPoolConnectionScenario()
                 << "C: A000003 RENAME \"INBOX.Foo.Baz\" \"INBOX.Bar.Baz\""
                 << "S: A000003 OK rename done"
                 << "C: A000004 SUBSCRIBE \"INBOX.Bar.Baz\""
                 << "S: A000004 OK subscribe done";

        callNames.clear();
        callNames << QStringLiteral("collectionChangeCommitted");

        QTest::newRow("moving mailbox with non-standard separators") << collection << source << target << scenario << callNames;
    }

    void shouldRenameMailBox()
    {
        QFETCH(Akonadi::Collection, collection);
        QFETCH(Akonadi::Collection, source);
        QFETCH(Akonadi::Collection, target);
        QFETCH(QList<QByteArray>, scenario);
        QFETCH(QStringList, callNames);

        FakeServer server;
        server.setScenario(scenario);
        server.startAndWait();

        SessionPool pool(1);

        pool.setPasswordRequester(createDefaultRequester());
        QVERIFY(pool.connect(createDefaultAccount()));
        QVERIFY(waitForSignal(&pool, SIGNAL(connectDone(int,QString))));

        DummyResourceState::Ptr state = DummyResourceState::Ptr(new DummyResourceState);
        state->setCollection(collection);
        state->setSourceCollection(source);
        state->setTargetCollection(target);
        MoveCollectionTask *task = new MoveCollectionTask(state);
        task->start(&pool);

        QTRY_COMPARE(state->calls().count(), callNames.size());
        for (int i = 0; i < callNames.size(); i++) {
            QString command = QString::fromUtf8(state->calls().at(i).first);
            QVariant parameter = state->calls().at(i).second;

            if (command == QLatin1String("cancelTask") && callNames[i] != QLatin1String("cancelTask")) {
                qDebug() << "Got a cancel:" << parameter.toString();
            }

            QCOMPARE(command, callNames[i]);

            if (command == QLatin1String("cancelTask")) {
                QVERIFY(!parameter.toString().isEmpty());
            }
        }

        QVERIFY(server.isAllScenarioDone());

        server.quit();
    }
};

QTEST_GUILESS_MAIN(TestMoveCollectionTask)

#include "testmovecollectiontask.moc"
