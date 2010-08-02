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

#include "expungecollectiontask.h"

class TestExpungeCollectionTask : public ImapTestBase
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
             << "C: A000003 SELECT \"INBOX/Foo\""
             << "S: A000003 OK select done"
             << "C: A000004 EXPUNGE"
             << "S: A000004 OK expunge done";

    callNames.clear();
    callNames << "taskDone";

    QTest::newRow( "normal case" ) << collection << scenario << callNames;


    // We keep the same collection

    scenario.clear();
    scenario << defaultPoolConnectionScenario()
             << "C: A000003 SELECT \"INBOX/Foo\""
             << "S: A000003 NO select failed";

    callNames.clear();
    callNames << "cancelTask";

    QTest::newRow( "select failed" ) << collection << scenario << callNames;

    // We keep the same collection

    scenario.clear();
    scenario << defaultPoolConnectionScenario()
             << "C: A000003 SELECT \"INBOX/Foo\""
             << "S: A000003 OK select done"
             << "C: A000004 EXPUNGE"
             << "S: A000004 NO expunge failed";

    callNames.clear();
    callNames << "cancelTask";

    QTest::newRow( "expunge failed" ) << collection << scenario << callNames;
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
    QVERIFY( waitForSignal( &pool, SIGNAL(connectDone(int, QString)) ) );

    DummyResourceState::Ptr state = DummyResourceState::Ptr(new DummyResourceState);
    state->setCollection( collection );
    ExpungeCollectionTask *task = new ExpungeCollectionTask( state );
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

QTEST_KDEMAIN_CORE( TestExpungeCollectionTask )

#include "testexpungecollectiontask.moc"
