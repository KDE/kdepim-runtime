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

#include "moveitemtask.h"
#include "uidnextattribute.h"

#include <kmime/kmime_message.h>

Q_DECLARE_METATYPE(QSet<QByteArray>)

class TestMoveItemTask : public ImapTestBase
{
  Q_OBJECT

private slots:
  void shouldCopyAndDeleteMessage_data()
  {
    QTest::addColumn<Akonadi::Item>("item");
    QTest::addColumn<Akonadi::Collection>("source");
    QTest::addColumn<Akonadi::Collection>("target");
    QTest::addColumn< QList<QByteArray> >("scenario");
    QTest::addColumn<QStringList>("callNames");

    Akonadi::Item item;
    Akonadi::Collection source;
    Akonadi::Collection target;
    QList<QByteArray> scenario;
    QStringList callNames;

    item = Akonadi::Item( 1 );
    item.setRemoteId( "5" );
    source = Akonadi::Collection( 2 );
    source.setRemoteId( "/INBOX/Foo" );
    target = Akonadi::Collection( 3 );
    target.setRemoteId( "/INBOX/Bar" );

    scenario.clear();
    scenario << defaultPoolConnectionScenario()
             << "C: A000003 SELECT \"INBOX/Foo\""
             << "S: A000003 OK select done"
             << "C: A000004 UID COPY 5 \"INBOX/Bar\""
             << "S: A000004 OK copy [ COPYUID 1239890035 5 65 ]"
             << "C: A000005 UID STORE 5 +FLAGS (\\Deleted)"
             << "S: A000005 OK store done";

    callNames.clear();
    callNames << "itemChangeCommitted";

    QTest::newRow( "moving mail" ) << item << source << target << scenario << callNames;




    // Same item and collections
    // The scenario changes though

    scenario.clear();
    scenario << defaultPoolConnectionScenario()
             << "C: A000003 SELECT \"INBOX/Foo\""
             << "S: A000003 OK select done"
             << "C: A000004 UID COPY 5 \"INBOX/Bar\""
             << "S: A000004 OK copy [ COPYUID 1239890035 5 65 ]"
             << "C: A000005 UID STORE 5 +FLAGS (\\Deleted)"
             << "S: A000005 NO store failed";

    callNames.clear();
    callNames << "emitWarning" << "itemChangeCommitted";

    QTest::newRow( "moving mail, store fails" ) << item << source << target << scenario << callNames;
  }

  void shouldCopyAndDeleteMessage()
  {
    QFETCH( Akonadi::Item, item );
    QFETCH( Akonadi::Collection, source );
    QFETCH( Akonadi::Collection, target );
    QFETCH( QList<QByteArray>, scenario );
    QFETCH( QStringList, callNames );

    FakeServer server;
    server.setScenario( scenario );
    server.start();

    SessionPool pool( 1 );

    pool.setPasswordRequester( createDefaultRequester() );
    QVERIFY( pool.connect( createDefaultAccount() ) );
    QVERIFY( waitForSignal( &pool, SIGNAL(connectDone(int, QString)) ) );

    DummyResourceState::Ptr state = DummyResourceState::Ptr(new DummyResourceState);
    state->setItem( item );
    state->setSourceCollection( source );
    state->setTargetCollection( target );
    MoveItemTask *task = new MoveItemTask( state );
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

QTEST_KDEMAIN_CORE( TestMoveItemTask )

#include "testmoveitemtask.moc"
