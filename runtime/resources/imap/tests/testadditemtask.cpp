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

#include "additemtask.h"

#include "uidnextattribute.h"

#include <kmime/kmime_message.h>

class TestAddItemTask : public ImapTestBase
{
  Q_OBJECT

private slots:
  void shouldAppendMessage_data()
  {
    QTest::addColumn<Akonadi::Item>("item");
    QTest::addColumn<Akonadi::Collection>("collection");
    QTest::addColumn< QList<QByteArray> >("scenario");
    QTest::addColumn<QStringList>("callNames");

    Akonadi::Collection collection;
    Akonadi::Item item;
    QString messageContent;
    QList<QByteArray> scenario;
    QStringList callNames;

    collection = Akonadi::Collection( 1 );
    collection.setRemoteId( "/INBOX/Foo" );
    UidNextAttribute *uidNext = new UidNextAttribute;
    uidNext->setUidNext( 63 );
    collection.addAttribute( uidNext );

    item = Akonadi::Item( 2 );
    item.setParentCollection( collection );

    KMime::Message::Ptr message(new KMime::Message);

    messageContent = "From: ervin\nTo: someone\nSubject: foo\n\nSpeechless...";

    message->setContent(messageContent.toUtf8());
    message->parse();
    item.setPayload(message);

    scenario.clear();
    scenario << defaultPoolConnectionScenario()
             << "C: A000003 APPEND \"INBOX/Foo\"  {55}\r\n"+message->encodedContent(true)
             << "S: A000003 OK append done [ APPENDUID 1239890035 66 ]";

    callNames.clear();
    callNames << "itemChangeCommitted";

    QTest::newRow( "trivial case" ) << item << collection << scenario << callNames;



    message = KMime::Message::Ptr(new KMime::Message);

    messageContent = "From: ervin\nTo: someone\nSubject: foo\nMessage-ID: <42.4242.foo@bar.org>\n\nSpeechless...";

    message->setContent(messageContent.toUtf8());
    message->parse();
    item.setPayload(message);

    scenario.clear();
    scenario << defaultPoolConnectionScenario()
             << "C: A000003 APPEND \"INBOX/Foo\"  {90}\r\n"+message->encodedContent(true)
             << "S: A000003 OK append done"
             << "C: A000004 SELECT \"INBOX/Foo\""
             << "S: A000004 OK select done"
             << "C: A000005 UID SEARCH HEADER Message-ID <42.4242.foo@bar.org>"
             << "S: * SEARCH 66"
             << "S: A000005 OK search done";

    callNames.clear();
    callNames << "itemChangeCommitted";

    QTest::newRow( "no APPENDUID, message contained Message-ID" ) << item << collection << scenario << callNames;



    message = KMime::Message::Ptr(new KMime::Message);

    messageContent = "From: ervin\nTo: someone\nSubject: foo\n\nSpeechless...";

    message->setContent(messageContent.toUtf8());
    message->parse();
    item.setPayload(message);

    scenario.clear();
    scenario << defaultPoolConnectionScenario()
             << "C: A000003 APPEND \"INBOX/Foo\"  {55}\r\n"+message->encodedContent(true)
             << "S: A000003 OK append done"
             << "C: A000004 SELECT \"INBOX/Foo\""
             << "S: A000004 OK select done"
             << "C: A000005 UID SEARCH NEW UID 63:*"
             << "S: * SEARCH 66"
             << "S: A000005 OK search done";

    callNames.clear();
    callNames << "itemChangeCommitted";

    QTest::newRow( "no APPENDUID, message didn't contain Message-ID" ) << item << collection << scenario << callNames;
  }

  void shouldAppendMessage()
  {
    QFETCH( Akonadi::Item, item );
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
    state->setItem( item );
    AddItemTask *task = new AddItemTask( state );
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

QTEST_KDEMAIN_CORE( TestAddItemTask )

#include "testadditemtask.moc"
