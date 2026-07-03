/*
   SPDX-FileCopyrightText: 2026 Noham Devillers <noham.devillers@enioka.com>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "imaptestbase.h"

#include "changeitemsflagstask.h"
#include "highestmodseqattribute.h"

#include <QSignalSpy>
#include <QTest>
#include <akonadi/item.h>
#include <qset.h>
#include <qstringview.h>
#include <qtestcase.h>

Q_DECLARE_METATYPE(QSet<QByteArray>)

class TestChangeItemsFlagsTask : public ImapTestBase
{
    Q_OBJECT

private Q_SLOTS:
    void shouldChangeMessageFlags_data()
    {
        QTest::addColumn<Akonadi::Item::List>("items");
        QTest::addColumn<Akonadi::Collection>("collection");
        QTest::addColumn<QSet<QByteArray>>("addedFlags");
        QTest::addColumn<QSet<QByteArray>>("removedFlags");
        QTest::addColumn<QSet<QByteArray>>("parts");
        QTest::addColumn<QList<QByteArray>>("scenario");
        QTest::addColumn<QStringList>("callNames");

        Akonadi::Collection collection;
        Akonadi::Item::List items;
        Akonadi::Item item;
        QSet<QByteArray> addedFlags;
        QSet<QByteArray> removedFlags;
        QSet<QByteArray> parts;
        QString messageContent;
        QList<QByteArray> scenario;
        QStringList callNames;

        collection = createCollectionChain(QStringLiteral("/INBOX/Foo"));
        item = Akonadi::Item(2);
        item.setParentCollection(collection);
        item.setRemoteId(QStringLiteral("5"));
        items = Akonadi::Item::List() << item;

        item = Akonadi::Item(5);
        item.setParentCollection(collection);
        item.setRemoteId(QStringLiteral("7"));
        items << item;

        addedFlags.clear();
        addedFlags << "\\Foo";

        parts.clear();
        parts << "FLAGS";

        scenario.clear();
        scenario << defaultPoolConnectionScenario() << "C: A000003 SELECT \"INBOX/Foo\""
                 << "S: A000003 OK select done"
                 << "C: A000004 UID STORE 5,7 +FLAGS (\\Foo)"
                 << "S: A000004 OK store done";

        callNames.clear();
        callNames << QStringLiteral("changeProcessed");

        QTest::newRow("modifying mail flags") << items << collection << addedFlags << removedFlags << parts << scenario << callNames;

        collection = createCollectionChain(QStringLiteral("/INBOX/Foo"));
        collection.attribute<HighestModSeqAttribute>(Akonadi::Collection::AddIfMissing)->setHighestModSeq(123456789);
        item = Akonadi::Item(2);
        item.setParentCollection(collection);
        item.setRemoteId(QStringLiteral("5"));
        items = Akonadi::Item::List() << item;

        item = Akonadi::Item(5);
        item.setParentCollection(collection);
        item.setRemoteId(QStringLiteral("7"));
        items << item;

        addedFlags.clear();
        addedFlags << "\\Foo";

        removedFlags.clear();
        removedFlags << "\\Bar";

        parts.clear();
        parts << "FLAGS";

        scenario.clear();
        scenario << defaultPoolConnectionScenario(QList<QByteArray>() << "CONDSTORE") << "C: A000003 SELECT \"INBOX/Foo\""
                 << "S: A000003 OK select done"
                 << "C: A000004 UID STORE 5,7 (UNCHANGEDSINCE 123456789) +FLAGS (\\Foo)"
                 << "S: * 1 FETCH (UID 5 MODSEQ (123456799))"
                 << "S: * 2 FETCH (UID 7 MODSEQ (123456800))"
                 << "S: A000004 OK Conditional Store completed"
                 << "C: A000005 UID STORE 5,7 (UNCHANGEDSINCE 123456789) -FLAGS (\\Bar)"
                 << "S: * 1 FETCH (UID 5 MODSEQ (123456801))"
                 << "S: * 2 FETCH (UID 7 MODSEQ (123456802))"
                 << "S: A000005 OK Conditional Store completed";

        callNames.clear();
        callNames << QStringLiteral("changeProcessed");
        QTest::newRow("CONDSTORE - modifying mail flags - changed message")
            << items << collection << addedFlags << removedFlags << parts << scenario << callNames;

        collection = createCollectionChain(QStringLiteral("/INBOX/Foo"));
        collection.attribute<HighestModSeqAttribute>(Akonadi::Collection::AddIfMissing)->setHighestModSeq(123456789);
        item = Akonadi::Item(2);
        item.setParentCollection(collection);
        item.setRemoteId(QStringLiteral("5"));
        items = Akonadi::Item::List() << item;

        item = Akonadi::Item(5);
        item.setParentCollection(collection);
        item.setRemoteId(QStringLiteral("7"));
        items << item;

        addedFlags.clear();
        addedFlags << "\\Foo";

        removedFlags.clear();
        removedFlags << "\\Seen";

        parts.clear();
        parts << "FLAGS";

        scenario.clear();
        scenario << defaultPoolConnectionScenario(QList<QByteArray>() << "CONDSTORE") << "C: A000003 SELECT \"INBOX/Foo\""
                 << "S: A000003 OK select done"
                 << "C: A000004 UID STORE 5,7 (UNCHANGEDSINCE 123456789) +FLAGS (\\Foo)"
                 << "S: * 1 FETCH (UID 5 MODSEQ (123456799))"
                 << "S: A000004 OK [MODIFIED 7] Conditional STORE failed"
                 << "C: A000005 UID STORE 5,7 (UNCHANGEDSINCE 123456789) -FLAGS (\\Seen)"
                 << "S: * 1 FETCH (UID 5 MODSEQ (123456800))"
                 << "S: A000005 OK [MODIFIED 7] Conditional STORE failed"
                 << "C: A000006 UID FETCH 7 (FLAGS UID)"
                 << "S: * 3 FETCH ( FLAGS (\\Seen) UID 7)"
                 << "S: A000006 OK fetch done";

        callNames.clear();
        callNames << QStringLiteral("itemsChangesCommitted");

        QTest::newRow("CONDSTORE - modifying mail flags - unchanged message")
            << items << collection << addedFlags << removedFlags << parts << scenario << callNames;
    }

    void shouldChangeMessageFlags()
    {
        QFETCH(Akonadi::Item::List, items);
        QFETCH(Akonadi::Collection, collection);
        QFETCH(QSet<QByteArray>, addedFlags);
        QFETCH(QSet<QByteArray>, removedFlags);
        QFETCH(QSet<QByteArray>, parts);
        QFETCH(QList<QByteArray>, scenario);
        QFETCH(QStringList, callNames);

        FakeServer server;
        server.setScenario(scenario);
        server.startAndWait();

        SessionPool pool(1);

        pool.setPasswordRequester(createDefaultRequester());
        QVERIFY(pool.connect(createDefaultAccount()));
        QSignalSpy doneSpy(&pool, &SessionPool::connectDone);
        QVERIFY(doneSpy.wait());

        DummyResourceState::Ptr state = DummyResourceState::Ptr(new DummyResourceState);
        state->setServerCapabilities(pool.serverCapabilities());
        state->setParts(parts);
        state->setItems(items);
        state->setCollection(collection);
        state->setAddedFlags(addedFlags);
        state->setRemovedFlags(removedFlags);
        auto task = new ChangeItemsFlagsTask(state);
        task->start(&pool);

        QTRY_COMPARE(state->calls().count(), callNames.size());
        for (int i = 0; i < callNames.size(); i++) {
            QString command = QString::fromUtf8(state->calls().at(i).first);
            QVariant parameter = state->calls().at(i).second;

            if (command == QLatin1StringView("cancelTask") && callNames[i] != QLatin1StringView("cancelTask")) {
                qDebug() << "Got a cancel:" << parameter.toString();
            }

            QCOMPARE(command, callNames[i]);

            if (command == QLatin1StringView("cancelTask")) {
                QVERIFY(!parameter.toString().isEmpty());
            }
        }

        QVERIFY(server.isAllScenarioDone());

        server.quit();
    }
};

QTEST_GUILESS_MAIN(TestChangeItemsFlagsTask)

#include "testchangeitemsflagstask.moc"
