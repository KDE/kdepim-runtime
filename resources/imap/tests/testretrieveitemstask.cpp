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

#include "retrieveitemstask.h"
#include "uidnextattribute.h"

#include <akonadi/cachepolicy.h>
#include <akonadi/collectionstatistics.h>
#include <akonadi/kmime/messageparts.h>

class TestRetrieveItemsTask : public ImapTestBase
{
  Q_OBJECT

private slots:
  void shouldIntrospectCollection_data()
  {
    QTest::addColumn<Akonadi::Collection>("collection");
    QTest::addColumn< QList<QByteArray> >("scenario");
    QTest::addColumn<QStringList>("callNames");
    QTest::addColumn<bool>("fastSync");

    Akonadi::Collection collection;
    QList<QByteArray> scenario;
    QStringList callNames;
    bool fastSync;

    collection = Akonadi::Collection( 1 );
    collection.setRemoteId( "/INBOX/Foo" );

    scenario.clear();
    scenario << defaultPoolConnectionScenario()
             << "C: A000003 SELECT \"INBOX/Foo\""
             << "S: A000003 OK select done"
             << "C: A000004 EXPUNGE"
             << "S: A000004 OK expunge done"
             << "C: A000005 SELECT \"INBOX/Foo\""
             << "S: * FLAGS (\\Answered \\Flagged \\Draft \\Deleted \\Seen)"
             << "S: * OK [ PERMANENTFLAGS (\\Answered \\Flagged \\Draft \\Deleted \\Seen) ]"
             << "S: * 1 EXISTS"
             << "S: * 0 RECENT"
             << "S: * OK [ UIDVALIDITY 1149151135  ]"
             << "S: * OK [ UIDNEXT 2471  ]"
             << "S: A000005 OK select done"
             << "C: A000006 FETCH 0:1 (RFC822.SIZE INTERNALDATE "
                "BODY.PEEK[HEADER.FIELDS (TO FROM MESSAGE-ID REFERENCES IN-REPLY-TO SUBJECT DATE)] "
                "FLAGS UID)"
             << "S: * 1 FETCH ( FLAGS (\\Seen) UID 2321 INTERNALDATE \"29-Jun-2010 15:26:42 +0200\" "
                "RFC822.SIZE 75 BODY[HEADER.FIELDS (TO FROM MESSAGE-ID REFERENCES IN-REPLY-TO SUBJECT DATE)] {69}\r\n"
                "From: Foo <foo@kde.org>\r\n"
                "To: Bar <bar@kde.org>\r\n"
                "Subject: Test Mail\r\n"
                "\r\n"
                " )"
             << "S: A000006 OK fetch done";

    callNames.clear();
    callNames << "applyCollectionChanges" << "itemsRetrieved" << "itemsRetrievalDone";

    fastSync = false;


    QTest::newRow( "first listing, connected IMAP" ) << collection << scenario << callNames << fastSync;

    Akonadi::CachePolicy policy;
    policy.setLocalParts( QStringList() << Akonadi::MessagePart::Envelope
                          << Akonadi::MessagePart::Header
                          << Akonadi::MessagePart::Body );

    collection = Akonadi::Collection( 1 );
    collection.setRemoteId( "/INBOX/Foo" );
    collection.setCachePolicy( policy );

    scenario.clear();
    scenario << defaultPoolConnectionScenario()
             << "C: A000003 SELECT \"INBOX/Foo\""
             << "S: A000003 OK select done"
             << "C: A000004 EXPUNGE"
             << "S: A000004 OK expunge done"
             << "C: A000005 SELECT \"INBOX/Foo\""
             << "S: * FLAGS (\\Answered \\Flagged \\Draft \\Deleted \\Seen)"
             << "S: * OK [ PERMANENTFLAGS (\\Answered \\Flagged \\Draft \\Deleted \\Seen) ]"
             << "S: * 1 EXISTS"
             << "S: * 0 RECENT"
             << "S: * OK [ UIDVALIDITY 1149151135  ]"
             << "S: * OK [ UIDNEXT 2471  ]"
             << "S: A000005 OK select done"
             << "C: A000006 FETCH 0:1 (RFC822.SIZE INTERNALDATE BODY.PEEK[] FLAGS UID)"
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
    callNames << "applyCollectionChanges" << "itemsRetrieved" << "itemsRetrievalDone";

    fastSync = false;


    QTest::newRow( "first listing, disconnected IMAP" ) << collection << scenario << callNames << fastSync;



    Akonadi::CollectionStatistics stats;
    stats.setCount( 1 );

    collection = Akonadi::Collection( 1 );
    collection.setRemoteId( "/INBOX/Foo" );
    collection.setCachePolicy( policy );
    collection.setStatistics( stats );

    scenario.clear();
    scenario << defaultPoolConnectionScenario()
             << "C: A000003 SELECT \"INBOX/Foo\""
             << "S: A000003 OK select done"
             << "C: A000004 EXPUNGE"
             << "S: A000004 OK expunge done"
             << "C: A000005 SELECT \"INBOX/Foo\""
             << "S: * FLAGS (\\Answered \\Flagged \\Draft \\Deleted \\Seen)"
             << "S: * OK [ PERMANENTFLAGS (\\Answered \\Flagged \\Draft \\Deleted \\Seen) ]"
             << "S: * 1 EXISTS"
             << "S: * 0 RECENT"
             << "S: * OK [ UIDVALIDITY 1149151135  ]"
             << "S: * OK [ UIDNEXT 2471  ]"
             << "S: A000005 OK select done"
             << "C: A000006 FETCH 1 (FLAGS UID)"
             << "S: * 1 FETCH ( FLAGS (\\Seen) UID 2321 )"
             << "S: A000006 OK fetch done";

    callNames.clear();
    callNames << "applyCollectionChanges" << "itemsRetrieved" << "itemsRetrievalDone";

    fastSync = false;


    QTest::newRow( "second listing, full sync" ) << collection << scenario << callNames << fastSync;


    collection = Akonadi::Collection( 1 );
    collection.setRemoteId( "/INBOX/Foo" );
    collection.setCachePolicy( policy );
    collection.setStatistics( stats );

    scenario.clear();
    scenario << defaultPoolConnectionScenario()
             << "C: A000003 SELECT \"INBOX/Foo\""
             << "S: A000003 OK select done"
             << "C: A000004 EXPUNGE"
             << "S: A000004 OK expunge done"
             << "C: A000005 SELECT \"INBOX/Foo\""
             << "S: * FLAGS (\\Answered \\Flagged \\Draft \\Deleted \\Seen)"
             << "S: * OK [ PERMANENTFLAGS (\\Answered \\Flagged \\Draft \\Deleted \\Seen) ]"
             << "S: * 1 EXISTS"
             << "S: * 0 RECENT"
             << "S: * OK [ UIDVALIDITY 1149151135  ]"
             << "S: * OK [ UIDNEXT 2471  ]"
             << "S: A000005 OK select done";

    callNames.clear();
    callNames << "applyCollectionChanges" << "itemsRetrievedIncremental" << "itemsRetrievalDone";

    fastSync = true;


    QTest::newRow( "second listing, fast sync" ) << collection << scenario << callNames << fastSync;



    collection = Akonadi::Collection( 1 );
    collection.setRemoteId( "/INBOX/Foo" );
    collection.setCachePolicy( policy );
    collection.setStatistics( stats );

    scenario.clear();
    scenario << defaultPoolConnectionScenario()
             << "C: A000003 SELECT \"INBOX/Foo\""
             << "S: A000003 OK select done"
             << "C: A000004 EXPUNGE"
             << "S: A000004 OK expunge done"
             << "C: A000005 SELECT \"INBOX/Foo\""
             << "S: * FLAGS (\\Answered \\Flagged \\Draft \\Deleted \\Seen)"
             << "S: * OK [ PERMANENTFLAGS (\\Answered \\Flagged \\Draft \\Deleted \\Seen) ]"
             << "S: * 0 EXISTS"
             << "S: * 0 RECENT"
             << "S: * OK [ UIDVALIDITY 1149151135  ]"
             << "S: * OK [ UIDNEXT 2471  ]"
             << "S: A000005 OK select done";

    callNames.clear();
    callNames << "applyCollectionChanges" << "itemsRetrievalDone";

    fastSync = false;

    QTest::newRow( "third listing, full sync, empty folder" ) << collection << scenario << callNames << fastSync;


    collection.attribute<UidNextAttribute>( Akonadi::Collection::AddIfMissing )->setUidNext( 2470 );
    stats.setCount( 5 );
    collection.setStatistics( stats );
    scenario.clear();
    scenario << defaultPoolConnectionScenario()
             << "C: A000003 SELECT \"INBOX/Foo\""
             << "S: A000003 OK select done"
             << "C: A000004 EXPUNGE"
             << "S: A000004 OK expunge done"
             << "C: A000005 SELECT \"INBOX/Foo\""
             << "S: * FLAGS (\\Answered \\Flagged \\Draft \\Deleted \\Seen)"
             << "S: * OK [ PERMANENTFLAGS (\\Answered \\Flagged \\Draft \\Deleted \\Seen) ]"
             << "S: * 5 EXISTS"
             << "S: * 0 RECENT"
             << "S: * OK [ UIDVALIDITY 1149151135  ]"
             << "S: * OK [ UIDNEXT 2471  ]"
             << "S: A000005 OK select done"
             << "C: A000006 FETCH 4:5 (RFC822.SIZE INTERNALDATE BODY.PEEK[] FLAGS UID)"
             << "S: * 1 FETCH ( FLAGS (\\Seen) UID 2471 INTERNALDATE \"29-Jun-2010 15:26:42 +0200\" "
                "RFC822.SIZE 75 BODY[] {75}\r\n"
                "From: Foo <foo@kde.org>\r\n"
                "To: Bar <bar@kde.org>\r\n"
                "Subject: Test Mail\r\n"
                "\r\n"
                "Test\r\n"
                " )"
             << "S: A000006 OK fetch done";
    callNames.clear();
    callNames << "applyCollectionChanges" << "itemsRetrievedIncremental" << "itemsRetrievalDone";
    fastSync = true;

    QTest::newRow( "uidnext mismatch with recovery attempt" ) << collection << scenario << callNames << fastSync;
  }

  void shouldIntrospectCollection()
  {
    QFETCH( Akonadi::Collection, collection );
    QFETCH( QList<QByteArray>, scenario );
    QFETCH( QStringList, callNames );
    QFETCH( bool, fastSync );

    FakeServer server;
    server.setScenario( scenario );
    server.startAndWait();

    SessionPool pool( 1 );

    pool.setPasswordRequester( createDefaultRequester() );
    QVERIFY( pool.connect( createDefaultAccount() ) );
    QVERIFY( waitForSignal( &pool, SIGNAL(connectDone(int,QString)) ) );

    DummyResourceState::Ptr state = DummyResourceState::Ptr(new DummyResourceState);
    state->setCollection( collection );
    RetrieveItemsTask *task = new RetrieveItemsTask( state );
    task->setFastSyncEnabled( fastSync );
    task->start( &pool );
    QTest::qWait( 100 );

    QCOMPARE( state->calls().count(), callNames.size() );
    for (int i=0; i<callNames.size(); i++) {
      QString command = QString::fromUtf8(state->calls().at(i).first);
      QVariant parameter = state->calls().at(i).second;

      if ( command=="cancelTask" && callNames[i]!="cancelTask" ) {
        kDebug() << "Got a cancel:" << parameter.toString();
      }

      QCOMPARE( command, callNames[i] );

      if ( command == "cancelTask" ) {
        QVERIFY( !parameter.toString().isEmpty() );
      }
    }

    QVERIFY( server.isAllScenarioDone() );

    server.quit();
  }
};

QTEST_KDEMAIN_CORE( TestRetrieveItemsTask )

#include "testretrieveitemstask.moc"
