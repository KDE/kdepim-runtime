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

#include "removecollectionrecursivetask.h"
#include "removecollectiontask.h"

class TestRemoveCollectionTask : public ImapTestBase
{
  Q_OBJECT

private slots:
  void shouldDeleteMailBox_data()
  {
    QTest::addColumn<Akonadi::Collection>("collection");
    QTest::addColumn< QList<QByteArray> >("scenario");
    QTest::addColumn<QStringList>("callNames");

    Akonadi::Collection collection;
    QSet<QByteArray> parts;
    QString messageContent;
    QList<QByteArray> scenario;
    QStringList callNames;

    collection = Akonadi::Collection( 1 );
    collection.setRemoteId( "/INBOX/Foo" );

    scenario.clear();
    scenario << defaultPoolConnectionScenario()
             << "C: A000003 DELETE \"INBOX/Foo\""
             << "S: A000003 OK delete done";

    callNames.clear();
    callNames << "changeProcessed";

    QTest::newRow( "normal case" ) << collection << scenario << callNames;


    // We keep the same collection

    scenario.clear();
    scenario << defaultPoolConnectionScenario()
             << "C: A000003 DELETE \"INBOX/Foo\""
             << "S: A000003 NO delete failed";

    callNames.clear();
    callNames << "changeProcessed" << "emitWarning" << "synchronizeCollectionTree";

    QTest::newRow( "delete failed" ) << collection << scenario << callNames;
  }

  void shouldDeleteMailBox()
  {
    QFETCH( Akonadi::Collection, collection );
    QFETCH( QList<QByteArray>, scenario );
    QFETCH( QStringList, callNames );

    FakeServer server;
    server.setScenario( scenario );
    server.startAndWait();

    SessionPool pool( 1 );

    pool.setPasswordRequester( createDefaultRequester() );
    QVERIFY( pool.connect( createDefaultAccount() ) );
    QVERIFY( waitForSignal( &pool, SIGNAL(connectDone(int,QString)) ) );

    DummyResourceState::Ptr state = DummyResourceState::Ptr(new DummyResourceState);
    state->setCollection( collection );
    RemoveCollectionTask *task = new RemoveCollectionTask( state );
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

  void shouldDeleteMailBoxRecursive_data()
  {
    QTest::addColumn<Akonadi::Collection>("collection");
    QTest::addColumn< QList<QByteArray> >("scenario");
    QTest::addColumn<QStringList>("callNames");

    Akonadi::Collection collection;
    QList<QByteArray> scenario;
    QStringList callNames;

    collection = Akonadi::Collection( 1 );
    collection.setRemoteId( "/INBOX/test1" );

    scenario.clear();
    scenario << defaultPoolConnectionScenario()
             << "C: A000003 LSUB \"\" *"
             << "S: * LSUB ( \\HasChildren ) / INBOX"
             << "S: * LSUB ( \\HasChildren ) / INBOX/test1"
             << "S: * LSUB ( ) / INBOX/test1/test2"
             << "S: A000003 OK Completed ( 0.000 secs 26 calls )"
             << "C: A000004 SELECT \"INBOX/test1/test2\""
             << "S: * FLAGS ( \\Answered \\Flagged \\Draft \\Deleted \\Seen )"
             << "S: * OK [ PERMANENTFLAGS ( \\Answered \\Flagged \\Draft \\Deleted \\Seen \\* )  ]"
             << "S: * 1 EXISTS"
             << "S: * 0 RECENT"
             << "S: * OK [ UNSEEN 1  ]"
             << "S: * OK [ UIDVALIDITY 1292857898  ]"
             << "S: * OK [ UIDNEXT 2  ]"
             << "S: A000004 OK Completed [ READ-WRITE  ]"
             << "C: A000005 STORE 1:* +FLAGS (\\DELETED)"
             << "S: * 1 FETCH ( FLAGS (\\Deleted) ) "
             << "S: A000005 OK Completed"
             << "C: A000006 EXPUNGE"
             << "S: * 1 EXPUNGE"
             << "S: * 0 EXISTS"
             << "S: * 0 RECENT"
             << "S: A000006 OK Completed"
             << "C: A000007 DELETE \"INBOX/test1/test2\""
             << "S: * 0 EXISTS"
             << "S: * 0 RECENT"
             << "S: A000007 OK Completed"
             << "C: A000008 SELECT \"INBOX/test1\""
             << "S: * FLAGS ( \\Answered \\Flagged \\Draft \\Deleted \\Seen )"
             << "S: * OK [ PERMANENTFLAGS ( \\Answered \\Flagged \\Draft \\Deleted \\Seen \\* )  ]"
             << "S: * 1 EXISTS"
             << "S: * 1 RECENT"
             << "S: * OK [ UIDVALIDITY 1292857888  ]"
             << "S: * OK [ UIDNEXT 2  ]"
             << "S: A000008 OK Completed [ READ-WRITE  ]"
             << "C: A000009 STORE 1:* +FLAGS (\\DELETED)"
             << "S: * 1 FETCH ( FLAGS (\\Recent \\Deleted \\Seen) )"
             << "S: A000009 OK Completed"
             << "C: A000010 EXPUNGE"
             << "S: * 1 EXPUNGE"
             << "S: * 0 EXISTS"
             << "S: * 0 RECENT"
             << "S: A000010 OK Completed"
             << "C: A000011 DELETE \"INBOX/test1\""
             << "S: * 0 EXISTS"
             << "S: * 0 RECENT"
             << "S: A000011 OK Completed";

    callNames.clear();
    callNames << "changeProcessed";

    QTest::newRow( "normal case" ) << collection << scenario << callNames;
  }

  void shouldDeleteMailBoxRecursive()
  {
    QFETCH( Akonadi::Collection, collection );
    QFETCH( QList<QByteArray>, scenario );
    QFETCH( QStringList, callNames );

    FakeServer server;
    server.setScenario( scenario );
    server.startAndWait();

    SessionPool pool( 1 );

    pool.setPasswordRequester( createDefaultRequester() );
    QVERIFY( pool.connect( createDefaultAccount() ) );
    QVERIFY( waitForSignal( &pool, SIGNAL(connectDone(int,QString)) ) );

    DummyResourceState::Ptr state = DummyResourceState::Ptr(new DummyResourceState);
    state->setCollection( collection );
    RemoveCollectionRecursiveTask *task = new RemoveCollectionRecursiveTask( state );
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

QTEST_KDEMAIN_CORE( TestRemoveCollectionTask )

#include "testremovecollectiontask.moc"
