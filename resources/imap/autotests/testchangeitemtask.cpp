/*
   SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
   SPDX-FileContributor: Kevin Ottens <kevin@kdab.com>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "imaptestbase.h"

#include "changeitemtask.h"
#include "highestmodseqattribute.h"
#include "uidnextattribute.h"

#include <KMime/Message>

#include <QSignalSpy>
#include <QTest>

Q_DECLARE_METATYPE(QSet<QByteArray>)

class TestChangeItemTask : public ImapTestBase
{
    Q_OBJECT

private Q_SLOTS:
    void shouldAppendMessage_data()
    {
        QTest::addColumn<Akonadi::Item>("item");
        QTest::addColumn<Akonadi::Collection>("collection");
        QTest::addColumn<QSet<QByteArray>>("parts");
        QTest::addColumn<QList<QByteArray>>("scenario");
        QTest::addColumn<QStringList>("callNames");

        Akonadi::Collection collection;
        Akonadi::Item item;
        QSet<QByteArray> parts;
        QString messageContent;
        QList<QByteArray> scenario;
        QStringList callNames;

        collection = createCollectionChain(QStringLiteral("/INBOX/Foo"));
        collection.addAttribute(new UidNextAttribute(65));
        item = Akonadi::Item(2);
        item.setParentCollection(collection);
        item.setRemoteId(QStringLiteral("5"));

        std::shared_ptr<KMime::Message> message(new KMime::Message);

        messageContent = QStringLiteral("From: ervin\nTo: someone\nSubject: foo\n\nSpeechless...");

        message->setContent(messageContent.toUtf8());
        message->parse();
        item.setPayload(message);

        parts.clear();
        parts << "PLD:RFC822";

        scenario.clear();
        scenario << defaultPoolConnectionScenario() << "C: A000003 APPEND \"INBOX/Foo\" {55}\r\n" + message->encodedContent(KMime::NewlineType::CRLF)
                 << "S: A000003 OK append done [ APPENDUID 1239890035 65 ]"
                 << "C: A000004 SELECT \"INBOX/Foo\""
                 << "S: A000004 OK select done"
                 << "C: A000005 UID STORE 5 +FLAGS (\\Deleted)"
                 << "S: A000005 OK store done";

        callNames.clear();
        callNames << QStringLiteral("applyCollectionChanges") << QStringLiteral("itemChangeCommitted");

        QTest::newRow("modifying mail content") << item << collection << parts << scenario << callNames;

        collection = createCollectionChain(QStringLiteral("/INBOX/Foo"));
        collection.addAttribute(new UidNextAttribute(65));
        item = Akonadi::Item(2);
        item.setParentCollection(collection);
        item.setRemoteId(QStringLiteral("5"));

        message = std::shared_ptr<KMime::Message>(new KMime::Message);

        messageContent = QStringLiteral("From: ervin\nTo: someone\nSubject: foo\nMessage-ID: <42.4242.foo@bar.org>\n\nSpeechless...");

        message->setContent(messageContent.toUtf8());
        message->parse();
        item.setPayload(message);

        parts.clear();
        parts << "PLD:RFC822";

        scenario.clear();
        scenario << defaultPoolConnectionScenario() << "C: A000003 APPEND \"INBOX/Foo\" {90}\r\n" + message->encodedContent(KMime::NewlineType::CRLF)
                 << "S: A000003 OK append done"
                 << "C: A000004 SELECT \"INBOX/Foo\""
                 << "S: A000004 OK select done"
                 << "C: A000005 UID SEARCH HEADER Message-ID \"<42.4242.foo@bar.org>\""
                 << "S: * SEARCH 65"
                 << "S: A000005 OK search done"
                 << "C: A000006 UID STORE 5 +FLAGS (\\Deleted)"
                 << "S: A000006 OK store done";

        callNames.clear();
        callNames << QStringLiteral("applyCollectionChanges") << QStringLiteral("itemChangeCommitted");

        QTest::newRow("modifying mail content, no APPENDUID, message has Message-ID") << item << collection << parts << scenario << callNames;

        collection = createCollectionChain(QStringLiteral("/INBOX/Foo"));
        collection.addAttribute(new UidNextAttribute(65));
        item = Akonadi::Item(2);
        item.setParentCollection(collection);
        item.setRemoteId(QStringLiteral("5"));

        message = std::shared_ptr<KMime::Message>(new KMime::Message);

        messageContent = QStringLiteral("From: ervin\nTo: someone\nSubject: foo\n\nSpeechless...");

        message->setContent(messageContent.toUtf8());
        message->parse();
        item.setPayload(message);

        parts.clear();
        parts << "PLD:RFC822";

        scenario.clear();
        scenario << defaultPoolConnectionScenario() << "C: A000003 APPEND \"INBOX/Foo\" {55}\r\n" + message->encodedContent(KMime::NewlineType::CRLF)
                 << "S: A000003 OK append done"
                 << "C: A000004 SELECT \"INBOX/Foo\""
                 << "S: A000004 OK select done"
                 << "C: A000005 UID SEARCH NEW UID 65:*"
                 << "S: * SEARCH 65"
                 << "S: A000005 OK search done"
                 << "C: A000006 UID STORE 5 +FLAGS (\\Deleted)"
                 << "S: A000006 OK store done";

        callNames.clear();
        callNames << QStringLiteral("applyCollectionChanges") << QStringLiteral("itemChangeCommitted");

        QTest::newRow("modifying mail content, no APPENDUID, message has no Message-ID") << item << collection << parts << scenario << callNames;

        // collection unchanged for this test
        // item only gets a set of flags
        item.setFlags(QSet<QByteArray>() << "\\Foo");

        parts.clear();
        parts << "FLAGS";

        scenario.clear();
        scenario << defaultPoolConnectionScenario() << "C: A000003 SELECT \"INBOX/Foo\""
                 << "S: A000003 OK select done"
                 << "C: A000004 UID STORE 5 FLAGS (\\Foo)"
                 << "S: A000004 OK store done";

        callNames.clear();
        callNames << QStringLiteral("changeProcessed");

        QTest::newRow("modifying mail flags") << item << collection << parts << scenario << callNames;

        collection = createCollectionChain(QStringLiteral("/INBOX/Foo"));
        collection.attribute<HighestModSeqAttribute>(Akonadi::Collection::AddIfMissing)->setHighestModSeq(123456789);
        item = Akonadi::Item(2);
        item.setParentCollection(collection);
        item.setRemoteId(QStringLiteral("5"));
        item.setFlags(QSet<QByteArray>() << "\\Foo");

        parts.clear();
        parts << "FLAGS";

        scenario.clear();
        scenario << defaultPoolConnectionScenario(QList<QByteArray>() << "CONDSTORE") << "C: A000003 SELECT \"INBOX/Foo\""
                 << "S: A000003 OK select done"
                 << "C: A000004 UID STORE 5 (UNCHANGEDSINCE 123456789) FLAGS (\\Foo)"
                 << "S: * 1 FETCH (UID 5 MODSEQ (123456799))"
                 << "S: A000004 OK Conditional Store completed";

        callNames.clear();
        callNames << QStringLiteral("changeProcessed");
        QTest::newRow("CONDSTORE - modifying mail flags - changed message") << item << collection << parts << scenario << callNames;

        collection = createCollectionChain(QStringLiteral("/INBOX/Foo"));
        collection.attribute<HighestModSeqAttribute>(Akonadi::Collection::AddIfMissing)->setHighestModSeq(123456789);
        item = Akonadi::Item(2);
        item.setParentCollection(collection);
        item.setRemoteId(QStringLiteral("5"));
        item.setFlags(QSet<QByteArray>() << "\\Foo");

        parts.clear();
        parts << "FLAGS";

        scenario.clear();
        scenario << defaultPoolConnectionScenario(QList<QByteArray>() << "CONDSTORE") << "C: A000003 SELECT \"INBOX/Foo\""
                 << "S: A000003 OK select done"
                 << "C: A000004 UID STORE 5 (UNCHANGEDSINCE 123456789) FLAGS (\\Foo)"
                 << "S: A000004 OK [MODIFIED 5] Conditional STORE failed"
                 << "C: A000005 UID FETCH 5 (FLAGS UID)"
                 << "S: * 2 FETCH ( FLAGS (\\Seen) UID 5 )"
                 << "S: A000005 OK fetch done";

        callNames.clear();
        callNames << QStringLiteral("itemChangeCommitted");

        QTest::newRow("CONDSTORE - modifying mail flags - unchanged message") << item << collection << parts << scenario << callNames;
    }

    void shouldAppendMessage()
    {
        QFETCH(Akonadi::Item, item);
        QFETCH(Akonadi::Collection, collection);
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
        state->setItem(item);
        state->setCollection(collection);
        auto task = new ChangeItemTask(state);
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

QTEST_GUILESS_MAIN(TestChangeItemTask)

#include "testchangeitemtask.moc"
