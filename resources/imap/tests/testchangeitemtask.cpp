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

#include "changeitemtask.h"
#include "uidnextattribute.h"

#include <kmime/kmime_message.h>

Q_DECLARE_METATYPE(QSet<QByteArray>)

class TestChangeItemTask : public ImapTestBase
{
  Q_OBJECT

private slots:
  void shouldAppendMessage_data()
  {
    QTest::addColumn<Akonadi::Item>("item");
    QTest::addColumn< QSet<QByteArray> >("parts");
    QTest::addColumn< QList<QByteArray> >("scenario");
    QTest::addColumn<QStringList>("callNames");

    Akonadi::Collection collection;
    Akonadi::Item item;
    QSet<QByteArray> parts;
    QString messageContent;
    QList<QByteArray> scenario;
    QStringList callNames;

    collection = Akonadi::Collection( 1 );
    collection.setRemoteId( "INBOX/Foo" );
    collection.addAttribute(new UidNextAttribute( 65 ));
    item = Akonadi::Item( 2 );
    item.setParentCollection( collection );
    item.setRemoteId( "5" );

    KMime::Message::Ptr message(new KMime::Message);

    messageContent = "From: ervin\nTo: someone\nSubject: foo\n\nSpeechless...";

    message->setContent(messageContent.toUtf8());
    message->parse();
    item.setPayload(message);

    parts.clear();
    parts << "PLD:RFC822";

    scenario.clear();
    scenario << defaultPoolConnectionScenario()
             << "C: A000003 APPEND \"INBOX/Foo\"  {55}\r\n"+message->encodedContent(true)
             << "S: A000003 OK append done [ APPENDUID 1239890035 65 ]"
             << "C: A000004 SELECT \"INBOX/Foo\""
             << "S: A000004 OK select done"
             << "C: A000005 UID STORE 5 +FLAGS (\\Deleted)"
             << "S: A000005 OK store done";

    callNames.clear();
    callNames << "applyCollectionChanges" << "changeCommitted";

    QTest::newRow( "modifying mail content" ) << item << parts << scenario << callNames;



    // collection unchanged for this test
    // item only gets a set of flags
    item.setFlags( QSet<QByteArray>() << "\\Foo" );

    parts.clear();
    parts << "FLAGS";

    scenario.clear();
    scenario << defaultPoolConnectionScenario()
             << "C: A000003 SELECT \"INBOX/Foo\""
             << "S: A000003 OK select done"
             << "C: A000004 UID STORE 5 FLAGS (\\Foo)"
             << "S: A000004 OK store done";

    callNames.clear();
    callNames << "changeProcessed";

    QTest::newRow( "modifying mail flags" ) << item << parts << scenario << callNames;
  }

  void shouldAppendMessage()
  {
    QFETCH( Akonadi::Item, item );
    QFETCH( QSet<QByteArray>, parts );
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
    state->setParts( parts );
    state->setItem( item );
    ChangeItemTask *task = new ChangeItemTask( state );
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

QTEST_KDEMAIN_CORE( TestChangeItemTask )

#include "testchangeitemtask.moc"
