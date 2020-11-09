/*
   SPDX-FileCopyrightText: 2020 Daniel Vrátil <dvratil@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "imaptestbase.h"

#include "retrieveitemstask_qresync.h"
#include "uidnextattribute.h"
#include "highestmodseqattribute.h"
#include "uidvalidityattribute.h"

#include <AkonadiCore/CachePolicy>
#include <AkonadiCore/CollectionStatistics>
#include <Akonadi/KMime/MessageParts>
#include <QTest>

class TestRetrieveItemsQResyncTask : public ImapTestBase
{
    Q_OBJECT

private Q_SLOTS:
    void shouldFetchItems_data()
    {
        QTest::addColumn<Akonadi::Collection>("collection");
        QTest::addColumn<QList<QByteArray>>("scenario");
        QTest::addColumn<QStringList>("callNames");

        Akonadi::Collection collection;
        QList<QByteArray> scenario;
        QStringList callNames;

        collection = createCollectionChain(QStringLiteral("/INBOX/Foo"));
        //collection.attribute<UidValidityAttribute>(Akonadi::Collection::AddIfMissing)->setUidValidity(1149151135);
        //collection.attribute<HighestModSeqAttribute>(Akonadi::Collection::AddIfMissing)->setHighestModSeq(123456);

        scenario.clear();
        scenario << defaultPoolConnectionScenario()
                 << "C: A000003 SELECT \"INBOX/Foo\" (CONDSTORE)"
                 << "S: * 1 EXISTS"
                 << "S: * 0 RECENT"
                 << "S: * OK [UIDVALIDITY 3857529045] UIDVALIDITY"
                 << "S: * OK [UIDNEXT 9] Predicted next UID"
                 << "S: * OK [HIGHESTMODSEQ 123456]"
                 << "S: * OK [UNSEEN 1] Message 1 is first unseen"
                 << "S: * FLAGS (\\Answered \\Flagged \\Draft \\Deleted \\Seen)"
                 << "S: * OK [PERMANENTFLAGS (\\Answered \\Flagged \\Draft \\Deleted \\Seen \\*)] Permanent flags"
                 << "S: A000003 OK [READ-WRITE]"
                 << "C: A000004 EXPUNGE"
                 << "S: A000004 OK expunge done"
                 << "C: A000005 UID SEARCH UID 1:9"
                 << "S: * SEARCH 7"
                 << "S: A000005 OK search done"
                 << "C: A000006 UID FETCH 7 (RFC822.SIZE INTERNALDATE "
            "BODY.PEEK[HEADER] "
            "FLAGS UID)"
                 << "S: * 1 FETCH ( FLAGS (\\Seen) UID 7 INTERNALDATE \"29-Jun-2010 15:26:42 +0200\" "
            "RFC822.SIZE 75 BODY[HEADER] {69}\r\n"
            "From: Foo <foo@kde.org>\r\n"
            "To: Bar <bar@kde.org>\r\n"
            "Subject: Test Mail\r\n"
            "\r\n"
            " )"
                 << "S: A000006 OK fetch done";

        callNames.clear();
        callNames << QStringLiteral("itemsRetrieved")
                  << QStringLiteral("itemsRetrievalDone")
                  << QStringLiteral("applyCollectionChanges");

        QTest::newRow("first listing, connected IMAP") << collection << scenario << callNames;

        scenario.clear();
        scenario << defaultPoolConnectionScenario()
                 << "C: A000003 SELECT \"INBOX/Foo\" (CONDSTORE)"
                 << "S: * FLAGS (\\Answered \\Flagged \\Draft \\Deleted \\Seen)"
                 << "S: * OK [ PERMANENTFLAGS (\\Answered \\Flagged \\Draft \\Deleted \\Seen) ]"
                 << "S: * 1 EXISTS"
                 << "S: * 0 RECENT"
                 << "S: * OK [ UIDVALIDITY 1149151135  ]"
                 << "S: * OK [ UIDNEXT 9  ]"
                 << "S: * OK [ HIGHESTMODSEQ 123456 ]"
                 << "S: A000003 OK [READ-ONLY] select done"
                 << "C: A000004 UID SEARCH UID 1:9"
                 << "S: * SEARCH 7"
                 << "S: A000004 OK search done"
                 << "C: A000005 UID FETCH 7 (RFC822.SIZE INTERNALDATE "
            "BODY.PEEK[HEADER] "
            "FLAGS UID)"
                 << "S: * 1 FETCH ( FLAGS (\\Seen) UID 7 INTERNALDATE \"29-Jun-2010 15:26:42 +0200\" "
            "RFC822.SIZE 75 BODY[HEADER] {69}\r\n"
            "From: Foo <foo@kde.org>\r\n"
            "To: Bar <bar@kde.org>\r\n"
            "Subject: Test Mail\r\n"
            "\r\n"
            " )"
                 << "S: A000005 OK fetch done";
        callNames.clear();
        callNames << QStringLiteral("itemsRetrieved")
                  << QStringLiteral("itemsRetrievalDone")
                  << QStringLiteral("applyCollectionChanges");

        QTest::newRow("retrieval from read-only mailbox (no expunge)") << collection << scenario << callNames;

        Akonadi::CachePolicy policy;
        policy.setLocalParts(QStringList() << QLatin1String(Akonadi::MessagePart::Envelope)
                                           << QLatin1String(Akonadi::MessagePart::Header)
                                           << QLatin1String(Akonadi::MessagePart::Body));

        collection = createCollectionChain(QStringLiteral("/INBOX/Foo"));
        collection.setCachePolicy(policy);

        scenario.clear();
        scenario << defaultPoolConnectionScenario()
                 << "C: A000003 SELECT \"INBOX/Foo\" (CONDSTORE)"
                 << "S: * FLAGS (\\Answered \\Flagged \\Draft \\Deleted \\Seen)"
                 << "S: * OK [ PERMANENTFLAGS (\\Answered \\Flagged \\Draft \\Deleted \\Seen) ]"
                 << "S: * 1 EXISTS"
                 << "S: * 0 RECENT"
                 << "S: * OK [ UIDVALIDITY 1149151135  ]"
                 << "S: * OK [ UIDNEXT 9  ]"
                 << "S: * OK [ HIGHESTMODSEQ 123456 ]"
                 << "S: A000003 OK select done"
                 << "C: A000004 EXPUNGE"
                 << "S: A000004 OK expunge done"
                 << "C: A000005 UID SEARCH UID 1:9"
                 << "S: * SEARCH 1 2 3 4 5 6 7 8 9"
                 << "S: A000005 OK search done"
                 << "C: A000006 UID FETCH 1:9 (RFC822.SIZE INTERNALDATE BODY.PEEK[] FLAGS UID)"
                 << "S: * 1 FETCH ( FLAGS (\\Seen) UID 7 INTERNALDATE \"29-Jun-2010 15:26:42 +0200\" "
            "RFC822.SIZE 75 BODY[] {75}\r\n"
            "From: Foo <foo@kde.org>\r\n"
            "To: Bar <bar@kde.org>\r\n"
            "Subject: Test Mail\r\n"
            "\r\n"
            "Test\r\n"
            " )"
                 << "S: A000006 OK fetch done";

        callNames.clear();
        callNames << QStringLiteral("itemsRetrieved")
                  << QStringLiteral("itemsRetrievalDone")
                  << QStringLiteral("applyCollectionChanges");

        QTest::newRow("first listing, disconnected IMAP") << collection << scenario << callNames;

        Akonadi::CollectionStatistics stats;
        stats.setCount(1);

        collection = createCollectionChain(QStringLiteral("/INBOX/Foo"));
        collection.attribute<UidValidityAttribute>(Akonadi::Collection::AddIfMissing)->setUidValidity(1149151135);
        collection.attribute<UidNextAttribute>(Akonadi::Collection::AddIfMissing)->setUidNext(7);
        collection.attribute<HighestModSeqAttribute>(Akonadi::Collection::AddIfMissing)->setHighestModSeq(123456);
        collection.setCachePolicy(policy);
        collection.setStatistics(stats);

        scenario.clear();
        scenario << defaultPoolConnectionScenario()
                 << "C: A000003 SELECT \"INBOX/Foo\" (QRESYNC (1149151135 123456))"
                 << "S: * FLAGS (\\Answered \\Flagged \\Draft \\Deleted \\Seen)"
                 << "S: * OK [ PERMANENTFLAGS (\\Answered \\Flagged \\Draft \\Deleted \\Seen) ]"
                 << "S: * 2 EXISTS"
                 << "S: * 0 RECENT"
                 << "S: * OK [ UIDVALIDITY 1149151135  ]"
                 << "S: * OK [ UIDNEXT 7  ]"
                 << "S: * OK [ HIGHESTMODSEQ 123457 ]"
                 << "S: * VANISHED (EARLIER) 5"
                 << "S: * 2 FETCH (UID 6 FLAGS (\\Seen \\Answered) MODSEQ (123455))"
                 << "S: A000003 OK select done"
                 << "C: A000004 EXPUNGE"
                 << "S: A000004 OK [ HIGHESTMODSEQ 123457 ] expunge done";

        callNames.clear();
        callNames << QStringLiteral("itemsRetrievedIncremental") // vanished
                  << QStringLiteral("itemsRetrievedIncremental") // flags change
                  << QStringLiteral("itemsRetrievalDone")
                  << QStringLiteral("applyCollectionChanges");

       QTest::newRow("second listing, checking for flag changes") << collection << scenario << callNames;

        collection = createCollectionChain(QStringLiteral("/INBOX/Foo"));
        collection.attribute<UidValidityAttribute>(Akonadi::Collection::AddIfMissing)->setUidValidity(1149151135);
        collection.setCachePolicy(policy);
        stats.setCount(1);
        collection.setStatistics(stats);
        scenario.clear();
        scenario << defaultPoolConnectionScenario()
                 << "C: A000003 SELECT \"INBOX/Foo\" (CONDSTORE)"
                 << "S: * FLAGS (\\Answered \\Flagged \\Draft \\Deleted \\Seen)"
                 << "S: * OK [ PERMANENTFLAGS (\\Answered \\Flagged \\Draft \\Deleted \\Seen) ]"
                 << "S: * 2 EXISTS"
                 << "S: * 0 RECENT"
                 << "S: * OK [ UIDVALIDITY 1149151135  ]"
                 << "S: * OK [ UIDNEXT 9  ]"
                 << "S: * OK [ HIGHESTMODSEQ 123457 ]"
                 << "S: A000003 OK select done"
                 << "C: A000004 EXPUNGE"
                 << "S: A000004 OK expunge done"
                 << "C: A000005 UID SEARCH UID 1:9"
                 << "S: * FLAGS (\\Answered \\Flagged \\Draft \\Deleted \\Seen)"
                 << "S: * OK [ PERMANENTFLAGS (\\Answered \\Flagged \\Draft \\Deleted \\Seen) ]"
                 << "S: * 0 EXISTS"
                 << "S: * 0 RECENT"
                 << "S: * OK [ UIDVALIDITY 1149151135  ]"
                 << "S: * OK [ UIDNEXT 9  ]"
                 << "S: A000005 OK select done";

        callNames.clear();
        callNames << QStringLiteral("itemsRetrievalDone")
                  << QStringLiteral("applyCollectionChanges");

        QTest::newRow("third listing, full sync, empty folder") << collection << scenario << callNames;

        collection.attribute<UidNextAttribute>(Akonadi::Collection::AddIfMissing)->setUidNext(8);
        stats.setCount(4);
        collection.setStatistics(stats);
        collection.attribute<HighestModSeqAttribute>(Akonadi::Collection::AddIfMissing)->setHighestModSeq(123456788);
        scenario.clear();
        scenario << defaultPoolConnectionScenario()
                 << "C: A000003 SELECT \"INBOX/Foo\" (QRESYNC (1149151135 123456788))"
                 << "S: * FLAGS (\\Answered \\Flagged \\Draft \\Deleted \\Seen)"
                 << "S: * OK [ PERMANENTFLAGS (\\Answered \\Flagged \\Draft \\Deleted \\Seen) ]"
                 << "S: * 5 EXISTS"
                 << "S: * 0 RECENT"
                 << "S: * OK [ UIDVALIDITY 1149151135  ]"
                 << "S: * OK [ UIDNEXT 9  ]"
                 << "S: * OK [ HIGHESTMODSEQ 123456789 ]"
                 << "S: A000003 OK select done"
                 << "C: A000004 EXPUNGE"
                 << "S: A000004 OK [ HIGHESTMODSEQ 123456789 ] Expunged"
                 << "C: A000005 UID SEARCH UID 8:9"
                 << "S: * SEARCH 8"
                 << "S: A000005 OK search done"
                 << "C: A000006 UID FETCH 8 (RFC822.SIZE INTERNALDATE BODY.PEEK[] FLAGS UID)"
                 << "S: * 5 FETCH ( FLAGS (\\Seen) UID 8 INTERNALDATE \"29-Jun-2010 15:26:42 +0200\" "
            "RFC822.SIZE 75 BODY[] {75}\r\n"
            "From: Foo <foo@kde.org>\r\n"
            "To: Bar <bar@kde.org>\r\n"
            "Subject: Test Mail\r\n"
            "\r\n"
            "Test\r\n"
            " )"
                 << "S: A000006 OK fetch done";

        callNames.clear();
        callNames << QStringLiteral("itemsRetrievedIncremental")
                  << QStringLiteral("itemsRetrievalDone")
                  << QStringLiteral("applyCollectionChanges");

        //We know no messages have been removed, so we can do an incremental update
        QTest::newRow("uidnext changed, fetch new messages incrementally") << collection << scenario << callNames;

        collection = createCollectionChain(QStringLiteral("/INBOX/Foo"));
        collection.attribute<UidValidityAttribute>(Akonadi::Collection::AddIfMissing)->setUidValidity(1149151135);
        collection.setCachePolicy(policy);
        collection.attribute<UidNextAttribute>(Akonadi::Collection::AddIfMissing)->setUidNext(9);
        collection.attribute<HighestModSeqAttribute>(Akonadi::Collection::AddIfMissing)->setHighestModSeq(123456789);
        stats.setCount(5);
        collection.setStatistics(stats);
        scenario.clear();
        scenario << defaultPoolConnectionScenario(QList<QByteArray>() << "CONDSTORE")
                 << "C: A000003 SELECT \"INBOX/Foo\" (QRESYNC (1149151135 123456789))"
                 << "S: * 5 EXISTS"
                 << "S: * 0 RECENT"
                 << "S: * OK [ UIDVALIDITY 1149151135  ]"
                 << "S: * OK [ UIDNEXT 9  ]"
                 << "S: * OK [ HIGHESTMODSEQ 123456789 ]"
                 << "S: A000003 OK select done";
        callNames.clear();
        callNames << QStringLiteral("itemsRetrievedIncremental")
                  << QStringLiteral("itemsRetrievalDone")
                  << QStringLiteral("applyCollectionChanges");

        //No flags have changed
        QTest::newRow("highestmodseq test") << collection << scenario << callNames;


        collection = createCollectionChain(QStringLiteral("/INBOX/Foo"));
        collection.attribute<UidNextAttribute>(Akonadi::Collection::AddIfMissing)->setUidNext(9);
        collection.attribute<UidValidityAttribute>(Akonadi::Collection::AddIfMissing)->setUidValidity(3);
        collection.attribute<HighestModSeqAttribute>(Akonadi::Collection::AddIfMissing)->setHighestModSeq(123456789);
        collection.setCachePolicy(policy);
        stats.setCount(1);
        collection.setStatistics(stats);

        scenario.clear();
        scenario << defaultPoolConnectionScenario()
                 << "C: A000003 SELECT \"INBOX/Foo\" (QRESYNC (3 123456789))"
                 << "S: * FLAGS (\\Answered \\Flagged \\Draft \\Deleted \\Seen)"
                 << "S: * OK [ PERMANENTFLAGS (\\Answered \\Flagged \\Draft \\Deleted \\Seen) ]"
                 << "S: * 1 EXISTS"
                 << "S: * 0 RECENT"
                 << "S: * OK [ UIDVALIDITY 1149151135  ]"
                 << "S: * OK [ UIDNEXT 9  ]"
                 << "S: * OK [ HIGHESTMODSEQ 123456789 ]"
                 << "S: A000003 OK Sorry uidvalidity mismatch"
                 << "C: A000004 EXPUNGE"
                 << "S: A000004 OK expunge done"
                 << "C: A000005 UID SEARCH UID 1:9"
                 << "S: * SEARCH 1 2 3 4 5 6 7 8 9"
                 << "S: A000005 OK search done"
                 << "C: A000006 UID FETCH 1:9 (RFC822.SIZE INTERNALDATE BODY.PEEK[] FLAGS UID)"
                 << "S: * 1 FETCH ( FLAGS (\\Seen) UID 2321 INTERNALDATE \"29-Jun-2010 15:26:42 +0200\" "
            "RFC822.SIZE 75 BODY[] {75}\r\n"
            "From: Foo <foo@kde.org>\r\n"
            "To: Bar <bar@kde.org>\r\n"
            "Subject: Test Mail\r\n"
            "\r\n"
            "Test\r\n"
            " )"
                 << "S: A000006 OK fetch done";

        callNames.clear();
        callNames << QStringLiteral("itemsRetrieved")
                  << QStringLiteral("itemsRetrievalDone")
                  << QStringLiteral("applyCollectionChanges");

        QTest::newRow("uidvalidity changed") << collection << scenario << callNames;

        collection = createCollectionChain(QStringLiteral("/INBOX/Foo"));
        collection.attribute<UidNextAttribute>(Akonadi::Collection::AddIfMissing)->setUidNext(105);
        collection.attribute<UidValidityAttribute>(Akonadi::Collection::AddIfMissing)->setUidValidity(1149151135);
        collection.attribute<HighestModSeqAttribute>(Akonadi::Collection::AddIfMissing)->setHighestModSeq(123456789);
        collection.setCachePolicy(policy);
        stats.setCount(104);
        collection.setStatistics(stats);

        scenario.clear();
        scenario << defaultPoolConnectionScenario()
                 << "C: A000003 SELECT \"INBOX/Foo\" (QRESYNC (1149151135 123456789))"
                 << "S: * FLAGS (\\Answered \\Flagged \\Draft \\Deleted \\Seen)"
                 << "S: * OK [ PERMANENTFLAGS (\\Answered \\Flagged \\Draft \\Deleted \\Seen) ]"
                 << "S: * 119 EXISTS"
                 << "S: * 0 RECENT"
                 << "S: * OK [ UIDVALIDITY 1149151135  ]"
                 << "S: * OK [ UIDNEXT 120  ]"
                 << "S: * OK [ HIGHESTMODSEQ 123456799 ]"
                 << "S: A000003 OK select done"
                 << "C: A000004 EXPUNGE"
                 << "S: A000004 OK [ HIGHESTMODSEQ 123456799 ] Expunged."
                 << "C: A000005 UID SEARCH UID 105:120"
            //We asked for until 120 but only 119 is available (120 is uidnext)
                 << "S: * SEARCH 105 106 107 108 109 110 111 112 113 114 115 116 117 118 119"
                 << "S: A000005 OK search done"
                 << "C: A000006 UID FETCH 105:114 (RFC822.SIZE INTERNALDATE BODY.PEEK[] FLAGS UID)"
                 << "S: * 1 FETCH ( FLAGS (\\Seen) UID 105 INTERNALDATE \"29-Jun-2010 15:26:42 +0200\" "
            "RFC822.SIZE 75 BODY[] {75}\r\n"
            "From: Foo <foo@kde.org>\r\n"
            "To: Bar <bar@kde.org>\r\n"
            "Subject: Test Mail\r\n"
            "\r\n"
            "Test\r\n"
            " )"
            //9 more would follow but are excluded for clarity
                 << "S: A000006 OK fetch done"
                 << "C: A000007 UID FETCH 115:119 (RFC822.SIZE INTERNALDATE BODY.PEEK[] FLAGS UID)"
                 << "S: * 1 FETCH ( FLAGS (\\Seen) UID 115 INTERNALDATE \"29-Jun-2010 15:26:42 +0200\" "
            "RFC822.SIZE 75 BODY[] {75}\r\n"
            "From: Foo <foo@kde.org>\r\n"
            "To: Bar <bar@kde.org>\r\n"
            "Subject: Test Mail\r\n"
            "\r\n"
            "Test\r\n"
            " )"
            //4 more would follow but are excluded for clarity
                 << "S: A000007 OK fetch done";

        callNames.clear();
        callNames << QStringLiteral("itemsRetrievedIncremental")
                  << QStringLiteral("itemsRetrievedIncremental")
                  << QStringLiteral("itemsRetrievalDone")
                  << QStringLiteral("applyCollectionChanges");

        QTest::newRow("test batch processing") << collection << scenario << callNames;

        collection = createCollectionChain(QStringLiteral("/INBOX/Foo"));
        collection.attribute<UidValidityAttribute>(Akonadi::Collection::AddIfMissing)->setUidValidity(1149151135);
        collection.setCachePolicy(policy);
        collection.attribute<UidNextAttribute>(Akonadi::Collection::AddIfMissing)->setUidNext(9);
        collection.attribute<HighestModSeqAttribute>(Akonadi::Collection::AddIfMissing)->setHighestModSeq(123456780);
        stats.setCount(5);
        collection.setStatistics(stats);
        scenario.clear();
        scenario << defaultPoolConnectionScenario()
                 << "C: A000003 SELECT \"INBOX/Foo\" (QRESYNC (1149151135 123456780))"
                 << "S: * FLAGS (\\Answered \\Flagged \\Draft \\Deleted \\Seen)"
                 << "S: * OK [ PERMANENTFLAGS (\\Answered \\Flagged \\Draft \\Deleted \\Seen) ]"
                 << "S: * 4 EXISTS"
                 << "S: * 0 RECENT"
                 << "S: * OK [ UIDVALIDITY 1149151135 ]"
                 << "S: * OK [ UIDNEXT 9 ]"
                 << "S: * OK [ HIGHESTMODSEQ 123456789 ]"
                 << "S: * VANISHED (EARLIER) 1:8"
                 << "S: A000003 OK select done"
                 << "C: A000004 EXPUNGE"
                 << "S: A000004 OK [ HIGHESTMODSEQ 123456789 ] Expunged.";
        callNames.clear();
        callNames << QStringLiteral("itemsRetrievedIncremental")
                  << QStringLiteral("itemsRetrievalDone")
                  << QStringLiteral("applyCollectionChanges");

        //fetch only changed flags
        QTest::newRow("remote message deleted") << collection << scenario << callNames;


        collection = createCollectionChain(QStringLiteral("/INBOX/Foo"));
        collection.attribute<UidValidityAttribute>(Akonadi::Collection::AddIfMissing)->setUidValidity(1149151135);
        collection.setCachePolicy(policy);
        collection.attribute<UidNextAttribute>(Akonadi::Collection::AddIfMissing)->setUidNext(-1);
        collection.attribute<HighestModSeqAttribute>(Akonadi::Collection::AddIfMissing)->setHighestModSeq(123456789);
        stats.setCount(0);
        collection.setStatistics(stats);
        scenario.clear();
        scenario << defaultPoolConnectionScenario()
                 << "C: A000003 SELECT \"INBOX/Foo\" (QRESYNC (1149151135 123456789))"
                 << "S: * FLAGS (\\Answered \\Flagged \\Draft \\Deleted \\Seen)"
                 << "S: * OK [ PERMANENTFLAGS (\\Answered \\Flagged \\Draft \\Deleted \\Seen) ]"
                 << "S: * 1 EXISTS"
                 << "S: * 0 RECENT"
                 << "S: * OK [ UIDVALIDITY 1149151135  ]"
                 << "S: * OK [ HIGHESTMODSEQ 123456789 ]"
                 << "S: A000003 OK select done";

        callNames.clear();
        callNames << QStringLiteral("cancelTask");

        QTest::newRow("missing uidnext") << collection << scenario << callNames;
    }

    void shouldFetchItems()
    {
        QFETCH(Akonadi::Collection, collection);
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
        state->setServerCapabilities(pool.serverCapabilities());
        state->setCollection(collection);

        auto *task = new RetrieveItemsTaskQResync(state);
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

QTEST_GUILESS_MAIN(TestRetrieveItemsQResyncTask)

#include "testretrieveitemsqresynctask.moc"
