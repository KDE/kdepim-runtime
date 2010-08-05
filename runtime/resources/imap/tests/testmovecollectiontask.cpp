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

class TestMoveCollectionTask : public ImapTestBase
{
  Q_OBJECT

private slots:
  void shouldRenameMailBox_data()
  {
    QTest::addColumn<Akonadi::Collection>("collection");
    QTest::addColumn<Akonadi::Collection>("source");
    QTest::addColumn<Akonadi::Collection>("target");
    QTest::addColumn< QList<QByteArray> >("scenario");
    QTest::addColumn<QStringList>("callNames");

    Akonadi::Collection collection;
    Akonadi::Collection source;
    Akonadi::Collection target;
    QList<QByteArray> scenario;
    QStringList callNames;

    collection = Akonadi::Collection( 1 );
    collection.setRemoteId( "/Baz" );
    source = Akonadi::Collection( 2 );
    source.setRemoteId( "/INBOX/Foo" );
    target = Akonadi::Collection( 3 );
    target.setRemoteId( "/INBOX/Bar" );

    scenario.clear();
    scenario << defaultPoolConnectionScenario()
             << "C: A000003 RENAME \"INBOX/Foo/Baz\" \"INBOX/Bar/Baz\""
             << "S: A000003 OK rename done"
             << "C: A000004 SUBSCRIBE \"INBOX/Bar/Baz\""
             << "S: A000004 OK subscribe done";

    callNames.clear();
    callNames << "collectionChangeCommitted";

    QTest::newRow( "moving mailbox" ) << collection << source << target << scenario << callNames;




    // Same collections
    // The scenario changes though

    scenario.clear();
    scenario << defaultPoolConnectionScenario()
             << "C: A000003 RENAME \"INBOX/Foo/Baz\" \"INBOX/Bar/Baz\""
             << "S: A000003 OK rename done"
             << "C: A000004 SUBSCRIBE \"INBOX/Bar/Baz\""
             << "S: A000004 NO subscribe failed";

    callNames.clear();
    callNames << "emitWarning" << "collectionChangeCommitted";

    QTest::newRow( "moving mailbox, subscribe fails" ) << collection << source << target << scenario << callNames;
  }

  void shouldRenameMailBox()
  {
    QFETCH( Akonadi::Collection, collection );
    QFETCH( Akonadi::Collection, source );
    QFETCH( Akonadi::Collection, target );
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
    state->setSourceCollection( source );
    state->setTargetCollection( target );
    MoveCollectionTask *task = new MoveCollectionTask( state );
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

QTEST_KDEMAIN_CORE( TestMoveCollectionTask )

#include "testmovecollectiontask.moc"
