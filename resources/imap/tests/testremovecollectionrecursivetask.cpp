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

class TestRemoveCollectionRecursiveTask : public ImapTestBase
{
  Q_OBJECT

  void shouldDeleteMailBoxRecursive_data()
  {
    QTest::addColumn<Akonadi::Collection>( "collection" );
    QTest::addColumn< QList<QByteArray> >( "scenario" );
    QTest::addColumn<QStringList>( "callNames" );

    Akonadi::Collection collection;
    QList<QByteArray> scenario;
    QStringList callNames;

    collection = createCollectionChain( QLatin1String("/INBOX/test1") );

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
             << "C: A000007 CLOSE"
             << "S: A000007 OK Completed"
             << "C: A000008 DELETE \"INBOX/test1/test2\""
             << "S: * 0 EXISTS"
             << "S: * 0 RECENT"
             << "S: A000008 OK Completed"
             << "C: A000009 SELECT \"INBOX/test1\""
             << "S: * FLAGS ( \\Answered \\Flagged \\Draft \\Deleted \\Seen )"
             << "S: * OK [ PERMANENTFLAGS ( \\Answered \\Flagged \\Draft \\Deleted \\Seen \\* )  ]"
             << "S: * 1 EXISTS"
             << "S: * 1 RECENT"
             << "S: * OK [ UIDVALIDITY 1292857888  ]"
             << "S: * OK [ UIDNEXT 2  ]"
             << "S: A000009 OK Completed [ READ-WRITE  ]"
             << "C: A000010 STORE 1:* +FLAGS (\\DELETED)"
             << "S: * 1 FETCH ( FLAGS (\\Recent \\Deleted \\Seen) )"
             << "S: A000010 OK Completed"
             << "C: A000011 EXPUNGE"
             << "S: * 1 EXPUNGE"
             << "S: * 0 EXISTS"
             << "S: * 0 RECENT"
             << "S: A000011 OK Completed"
             << "C: A000012 CLOSE"
             << "S: A000012 OK Completed"
             << "C: A000013 DELETE \"INBOX/test1\""
             << "S: * 0 EXISTS"
             << "S: * 0 RECENT"
             << "S: A000013 OK Completed";
    callNames.clear();
    callNames << "changeProcessed";

    QTest::newRow( "normal case" ) << collection << scenario << callNames;

    scenario.clear();
    scenario << defaultPoolConnectionScenario()
              << "C: A000003 LSUB \"\" *"
             << "S: * LSUB ( \\HasChildren ) / INBOX"
             << "S: * LSUB ( \\HasChildren ) / INBOX/test1"
             << "S: * LSUB ( ) / INBOX/test1/test2"
             << "S: A000003 OK Completed ( 0.000 secs 26 calls )";
    collection.setRemoteId( "/test1" );
    collection.setParentCollection( Akonadi::Collection::root() );
    callNames.clear();
    callNames << "changeProcessed" << "emitWarning" << "synchronizeCollectionTree";
    QTest::newRow( "invalid collection" ) << collection << scenario << callNames;

    collection = createCollectionChain( QLatin1String(".INBOX.test1") );
    scenario.clear();
    scenario << defaultPoolConnectionScenario()
             << "C: A000003 LSUB \"\" *"
             << "S: * LSUB ( \\HasChildren ) . INBOX"
             << "S: * LSUB ( \\HasChildren ) . INBOX.test1"
             << "S: * LSUB ( ) . INBOX.test1.test2"
             << "S: A000003 OK Completed ( 0.000 secs 26 calls )"
             << "C: A000004 SELECT \"INBOX.test1.test2\""
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
             << "C: A000007 CLOSE"
             << "S: A000007 OK Completed"
             << "C: A000008 DELETE \"INBOX.test1.test2\""
             << "S: * 0 EXISTS"
             << "S: * 0 RECENT"
             << "S: A000008 OK Completed"
             << "C: A000009 SELECT \"INBOX.test1\""
             << "S: * FLAGS ( \\Answered \\Flagged \\Draft \\Deleted \\Seen )"
             << "S: * OK [ PERMANENTFLAGS ( \\Answered \\Flagged \\Draft \\Deleted \\Seen \\* )  ]"
             << "S: * 1 EXISTS"
             << "S: * 1 RECENT"
             << "S: * OK [ UIDVALIDITY 1292857888  ]"
             << "S: * OK [ UIDNEXT 2  ]"
             << "S: A000009 OK Completed [ READ-WRITE  ]"
             << "C: A000010 STORE 1:* +FLAGS (\\DELETED)"
             << "S: * 1 FETCH ( FLAGS (\\Recent \\Deleted \\Seen) )"
             << "S: A000010 OK Completed"
             << "C: A000011 EXPUNGE"
             << "S: * 1 EXPUNGE"
             << "S: * 0 EXISTS"
             << "S: * 0 RECENT"
             << "S: A000011 OK Completed"
             << "C: A000012 CLOSE"
             << "S: A000012 OK Completed"
             << "C: A000013 DELETE \"INBOX.test1\""
             << "S: * 0 EXISTS"
             << "S: * 0 RECENT"
             << "S: A000013 OK Completed";
    callNames.clear();
    callNames << "changeProcessed";
    QTest::newRow( "non-standard separator" ) << collection << scenario << callNames;

    collection = createCollectionChain( QLatin1String(".INBOX.test1") );
    scenario.clear();
    scenario << defaultPoolConnectionScenario()
             << "C: A000003 LSUB \"\" *"
             << "S: * LSUB ( \\HasChildren ) . INBOX"
             << "S: * LSUB ( \\HasChildren ) . INBOX.test1"
             << "S: * LSUB ( ) . INBOX.test1.test2"
             << "S: A000003 OK Completed ( 0.000 secs 26 calls )"
             << "C: A000004 SELECT \"INBOX.test1.test2\""
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
             << "C: A000007 CLOSE"
             << "S: A000007 NO Close failed";
    callNames.clear();
    callNames << "changeProcessed" << "emitWarning" << "synchronizeCollectionTree";
    QTest::newRow( "close failed" ) << collection << scenario << callNames;
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

    DummyResourceState::Ptr state = DummyResourceState::Ptr( new DummyResourceState );
    state->setCollection( collection );
    RemoveCollectionRecursiveTask *task = new RemoveCollectionRecursiveTask( state );
    task->start( &pool );
    QEventLoop loop;
    connect( task, SIGNAL(destroyed(QObject*)), &loop, SLOT(quit()) );
    loop.exec();

    QCOMPARE( state->calls().count(), callNames.size() );
    for ( int i = 0; i < callNames.size(); i++ ) {
      QString command = QString::fromUtf8(state->calls().at( i ).first);
      QVariant parameter = state->calls().at( i ).second;

      if ( command == "cancelTask" && callNames[i] != "cancelTask" ) {
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

QTEST_KDEMAIN_CORE( TestRemoveCollectionRecursiveTask )

#include "testremovecollectionrecursivetask.moc"
