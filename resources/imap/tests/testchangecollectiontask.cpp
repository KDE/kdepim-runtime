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

#include "changecollectiontask.h"

#include "collectionannotationsattribute.h"
#include "imapaclattribute.h"

Q_DECLARE_METATYPE( QSet<QByteArray> )

class TestChangeCollectionTask : public ImapTestBase
{
  Q_OBJECT

private slots:
  void shouldUpdateMetadataAclAndName_data()
  {
    QTest::addColumn<Akonadi::Collection>("collection");
    QTest::addColumn< QSet<QByteArray> >("parts");
    QTest::addColumn< QList<QByteArray> >("scenario");
    QTest::addColumn<QStringList>("callNames");
    QTest::addColumn<QString>("collectionName");

    Akonadi::Collection collection;
    QSet<QByteArray> parts;
    QList<QByteArray> scenario;
    QStringList callNames;

    collection = Akonadi::Collection( 1 );
    collection.setName( "Bar" );
    collection.setRemoteId( "/Foo" );
    collection.setRights( Akonadi::Collection::AllRights );

    Akonadi::ImapAclAttribute *acls = new Akonadi::ImapAclAttribute;
    QMap<QByteArray, KIMAP::Acl::Rights> rights;
    // Old rights
    rights["test@kdab.com"] = KIMAP::Acl::rightsFromString( "lrswipckxtda" );
    rights["foo@kde.org"] = KIMAP::Acl::rightsFromString( "lrswipcda" );
    acls->setRights( rights );

    // New rights
    rights["test@kdab.com"] = KIMAP::Acl::rightsFromString( "lrswipckxtda" );
    rights["foo@kde.org"] = KIMAP::Acl::rightsFromString( "lrswipcda" );
    acls->setRights( rights );
    collection.addAttribute( acls );

    Akonadi::CollectionAnnotationsAttribute *annotationsAttr = new Akonadi::CollectionAnnotationsAttribute;
    QMap<QByteArray, QByteArray> annotations;
    annotations["/vendor/kolab/folder-test"] = "false";
    annotations["/vendor/kolab/folder-test2"] = "true";
    annotationsAttr->setAnnotations( annotations );
    collection.addAttribute( annotationsAttr );

    parts << "NAME" << "AccessRights" << "imapacl" << "collectionannotations";

    scenario.clear();
    scenario << defaultPoolConnectionScenario()
             << "C: A000003 SETACL \"Foo\" \"test@kdab.com\" \"lrswipckxtda\""
             << "S: A000003 OK acl changed"
             << "C: A000004 SETANNOTATION \"Foo\" \"/vendor/kolab/folder-test\" (\"value.shared\" \"false\")"
             << "S: A000004 OK annotations changed"
             << "C: A000005 SETANNOTATION \"Foo\" \"/vendor/kolab/folder-test2\" (\"value.shared\" \"true\")"
             << "S: A000005 OK annotations changed"
             << "C: A000006 SETACL \"Foo\" \"foo@kde.org\" \"lrswipcda\""
             << "S: A000006 OK acl changed"
             << "C: A000007 SETACL \"Foo\" \"test@kdab.com\" \"lrswipckxtda\""
             << "S: A000007 OK acl changed"
             << "C: A000008 RENAME \"Foo\" \"Bar\""
             << "S: A000008 OK rename done"
             << "C: A000009 SUBSCRIBE \"Bar\""
             << "S: A000009 OK mailbox subscribed";

    callNames.clear();
    callNames << "collectionChangeCommitted";

    QTest::newRow( "complete case" ) << collection << parts << scenario << callNames << collection.name();

    collection = Akonadi::Collection( 1 );
    collection.setName( "Bar/Baz" );
    collection.setRemoteId( "/Foo" );
    scenario.clear();
    scenario << defaultPoolConnectionScenario()
             << "C: A000003 RENAME \"Foo\" \"BarBaz\""
             << "S: A000003 OK rename done"
             << "C: A000004 SUBSCRIBE \"BarBaz\""
             << "S: A000004 OK mailbox subscribed";
    parts.clear();
    parts << "NAME";
    callNames.clear();
    callNames << "collectionChangeCommitted";
    QTest::newRow( "rename with invalid separator" ) << collection << parts << scenario << callNames
                                                     << "BarBaz";
  }

  void shouldUpdateMetadataAclAndName()
  {
    QFETCH( Akonadi::Collection, collection );
    QFETCH( QSet<QByteArray>, parts );
    QFETCH( QList<QByteArray>, scenario );
    QFETCH( QStringList, callNames );
    QFETCH( QString, collectionName );

    FakeServer server;
    server.setScenario( scenario );
    server.startAndWait();

    SessionPool pool( 1 );

    pool.setPasswordRequester( createDefaultRequester() );
    QVERIFY( pool.connect( createDefaultAccount() ) );
    QVERIFY( waitForSignal( &pool, SIGNAL(connectDone(int, QString)) ) );

    DummyResourceState::Ptr state = DummyResourceState::Ptr(new DummyResourceState);
    state->setUserName( defaultUserName() );
    state->setServerCapabilities( QStringList() << "ANNOTATEMORE" << "ACL" );
    state->setCollection( collection );
    state->setParts( parts );
    ChangeCollectionTask *task = new ChangeCollectionTask( state );
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
      if ( command == "collectionChangeCommitted" ) {
        QCOMPARE( parameter.value<Akonadi::Collection>().name(), collectionName );
        QCOMPARE( parameter.value<Akonadi::Collection>().remoteId().right( collectionName.length() ),
                  collectionName );
      }
    }

    QVERIFY( server.isAllScenarioDone() );

    server.quit();
  }
};

QTEST_KDEMAIN_CORE( TestChangeCollectionTask )

#include "testchangecollectiontask.moc"
