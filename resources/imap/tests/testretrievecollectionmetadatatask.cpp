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

#include "retrievecollectionmetadatatask.h"

#include <akonadi/collectionquotaattribute.h>
#include "collectionannotationsattribute.h"
#include "imapaclattribute.h"
#include "imapquotaattribute.h"

class TestRetrieveCollectionMetadataTask : public ImapTestBase
{
  Q_OBJECT

private slots:
  void shouldCollectionRetrieveMetadata_data()
  {
    QTest::addColumn<Akonadi::Collection>("collection");
    QTest::addColumn<QStringList>("capabilities");
    QTest::addColumn< QList<QByteArray> >("scenario");
    QTest::addColumn<QStringList>("callNames");

    Akonadi::Collection collection;
    QStringList capabilities;
    QList<QByteArray> scenario;
    QStringList callNames;

    collection = Akonadi::Collection( 1 );
    collection.setRemoteId( "/INBOX/Foo" );

    capabilities.clear();
    capabilities << "ANNOTATEMORE" << "ACL" << "QUOTA";

    scenario.clear();
    scenario << defaultPoolConnectionScenario()
             << "C: A000003 GETANNOTATION \"INBOX/Foo\" \"*\" \"value.shared\""
             << "S: * ANNOTATION INBOX/Foo /vendor/kolab/folder-test ( value.shared true )"
             << "S: A000003 OK annotations retrieved"
             << "C: A000004 GETACL \"INBOX/Foo\""
             << "S: * ACL INBOX/Foo foo@kde.org lrswipcda"
             << "S: A000004 OK acl retrieved"
             << "C: A000005 MYRIGHTS \"INBOX/Foo\""
             << "S: * MYRIGHTS \"INBOX/Foo\" lrswipkxtecda"
             << "S: A000005 OK rights retrieved"
             << "C: A000006 GETQUOTAROOT \"INBOX/Foo\""
             << "S: * QUOTAROOT INBOX/Foo user/foo"
             << "S: * QUOTA user/foo ( )"
             << "S: A000006 OK quota retrieved";

    callNames.clear();
    callNames << "applyCollectionChanges" << "taskDone";

    QTest::newRow( "first listing, connected IMAP" ) << collection << capabilities << scenario << callNames;
  }

  void shouldCollectionRetrieveMetadata()
  {
    QFETCH( Akonadi::Collection, collection );
    QFETCH( QStringList, capabilities );
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
    state->setCollection( collection );
    state->setServerCapabilities( capabilities );
    RetrieveCollectionMetadataTask *task = new RetrieveCollectionMetadataTask( state );
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

QTEST_KDEMAIN_CORE( TestRetrieveCollectionMetadataTask )

#include "testretrievecollectionmetadatatask.moc"
