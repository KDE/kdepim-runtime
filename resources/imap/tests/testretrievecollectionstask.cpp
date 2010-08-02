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

#include "retrievecollectionstask.h"
#include "noselectattribute.h"

#include <akonadi/cachepolicy.h>
#include <akonadi/entitydisplayattribute.h>
#include <akonadi/kmime/messageparts.h>

class TestRetrieveCollectionsTask : public ImapTestBase
{
  Q_OBJECT
public:
  TestRetrieveCollectionsTask( QObject *parent = 0 )
    : ImapTestBase( parent ), m_nextCollectionId( 1 )
  {
  }

private slots:
  void shouldListCollections_data()
  {
    QTest::addColumn<Akonadi::Collection::List>("expectedCollections");
    QTest::addColumn< QList<QByteArray> >("scenario");
    QTest::addColumn<QStringList>("callNames");
    QTest::addColumn<bool>("isSubscriptionEnabled");
    QTest::addColumn<bool>("isDisconnectedModeEnabled");
    QTest::addColumn<int>("intervalCheckTime");

    Akonadi::Collection collection;

    Akonadi::Collection::List expectedCollections;
    QList<QByteArray> scenario;
    QStringList callNames;
    bool isSubscriptionEnabled;
    bool isDisconnectedModeEnabled;
    int intervalCheckTime;

    expectedCollections.clear();
    expectedCollections << createRootCollection()
                        << createCollection( "/", "INBOX" )
                        << createCollection( "/", "INBOX/Calendar" )
                        << createCollection( "/", "INBOX/Calendar/Private" )
                        << createCollection( "/", "INBOX/Archives" );

    scenario.clear();
    scenario << defaultPoolConnectionScenario()
             << "C: A000003 LIST \"\" *"
             << "S: * LIST ( \\HasChildren ) / INBOX"
             << "S: * LIST ( \\HasChildren ) / INBOX/Calendar"
             << "S: * LIST ( ) / INBOX/Calendar/Private"
             << "S: * LIST ( ) / INBOX/Archives"
             << "S: A000003 OK list done";

    callNames.clear();
    callNames << "collectionsRetrieved" << "setIdleCollection" << "collectionsRetrieved" << "collectionsRetrievalDone";

    isSubscriptionEnabled = false;
    isDisconnectedModeEnabled = false;
    intervalCheckTime = -1;

    QTest::newRow( "first listing, connected IMAP" ) << expectedCollections << scenario << callNames
                                                     << isSubscriptionEnabled << isDisconnectedModeEnabled << intervalCheckTime;



    expectedCollections.clear();
    expectedCollections << createRootCollection( true, 5 )
                        << createCollection( "/", "INBOX" )
                        << createCollection( "/", "INBOX/Calendar" )
                        << createCollection( "/", "INBOX/Calendar/Private" )
                        << createCollection( "/", "INBOX/Archives" );

    scenario.clear();
    scenario << defaultPoolConnectionScenario()
             << "C: A000003 LIST \"\" *"
             << "S: * LIST ( \\HasChildren ) / INBOX"
             << "S: * LIST ( \\HasChildren ) / INBOX/Calendar"
             << "S: * LIST ( ) / INBOX/Calendar/Private"
             << "S: * LIST ( ) / INBOX/Archives"
             << "S: A000003 OK list done";

    callNames.clear();
    callNames << "collectionsRetrieved" << "setIdleCollection" << "collectionsRetrieved" << "collectionsRetrievalDone";

    isSubscriptionEnabled = false;
    isDisconnectedModeEnabled = true;
    intervalCheckTime = 5;

    QTest::newRow( "first listing, disconnected IMAP" ) << expectedCollections << scenario << callNames
                                                        << isSubscriptionEnabled << isDisconnectedModeEnabled << intervalCheckTime;


    expectedCollections.clear();
    expectedCollections << createRootCollection()
                        << createCollection( "/", "INBOX" )
                        << createCollection( "/", "INBOX/Calendar", true )
                        << createCollection( "/", "INBOX/Calendar/Private" )
                        << createCollection( "/", "INBOX/Archives" );

    scenario.clear();
    scenario << defaultPoolConnectionScenario()
             << "C: A000003 LIST \"\" *"
             << "S: * LIST ( \\HasChildren ) / INBOX"
             << "S: * LIST ( ) / INBOX/Calendar/Private"
             << "S: * LIST ( ) / INBOX/Archives"
             << "S: A000003 OK list done";

    callNames.clear();
    callNames << "collectionsRetrieved" << "setIdleCollection" << "collectionsRetrieved" << "collectionsRetrievalDone";

    isSubscriptionEnabled = false;
    isDisconnectedModeEnabled = false;
    intervalCheckTime = -1;

    QTest::newRow( "auto-insert missing nodes in the tree" ) << expectedCollections << scenario << callNames
                                                             << isSubscriptionEnabled << isDisconnectedModeEnabled << intervalCheckTime;



    expectedCollections.clear();
    expectedCollections << createRootCollection()
                        << createCollection( "/", "INBOX" )
                        << createCollection( "/", "INBOX/Calendar" )
                        << createCollection( "/", "INBOX/Calendar/Private" );

    scenario.clear();
    scenario << defaultPoolConnectionScenario()
             << "C: A000003 LSUB \"\" *"
             << "S: * LSUB ( \\HasChildren ) / INBOX"
             << "S: * LSUB ( ) / INBOX/Calendar"
             << "S: * LSUB ( ) / INBOX/Calendar/Private"
             << "S: A000003 OK list done";

    callNames.clear();
    callNames << "collectionsRetrieved" << "setIdleCollection" << "collectionsRetrieved" << "collectionsRetrievalDone";

    isSubscriptionEnabled = true;
    isDisconnectedModeEnabled = false;
    intervalCheckTime = -1;

    QTest::newRow( "subscription enabled" ) << expectedCollections << scenario << callNames
                                            << isSubscriptionEnabled << isDisconnectedModeEnabled << intervalCheckTime;
  }

  void shouldListCollections()
  {
    QFETCH( Akonadi::Collection::List, expectedCollections );
    QFETCH( QList<QByteArray>, scenario );
    QFETCH( QStringList, callNames );
    QFETCH( bool, isSubscriptionEnabled );
    QFETCH( bool, isDisconnectedModeEnabled );
    QFETCH( int, intervalCheckTime );

    FakeServer server;
    server.setScenario( scenario );
    server.startAndWait();

    SessionPool pool( 1 );

    pool.setPasswordRequester( createDefaultRequester() );
    QVERIFY( pool.connect( createDefaultAccount() ) );
    QVERIFY( waitForSignal( &pool, SIGNAL(connectDone(int, QString)) ) );

    DummyResourceState::Ptr state = DummyResourceState::Ptr(new DummyResourceState);
    state->setResourceName( "resource" );
    state->setSubscriptionEnabled( isSubscriptionEnabled );
    state->setDisconnectedModeEnabled( isDisconnectedModeEnabled );
    state->setIntervalCheckTime( intervalCheckTime );

    RetrieveCollectionsTask *task = new RetrieveCollectionsTask( state );
    task->start( &pool );
    QTest::qWait( 100 );

    Akonadi::Collection::List collections;

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
      } else if ( command == "collectionsRetrieved" ) {
        collections+= parameter.value<Akonadi::Collection::List>();
      }
    }

    QVERIFY( server.isAllScenarioDone() );
    compareCollectionLists( collections, expectedCollections );

    server.quit();
  }

private:
  qint64 m_nextCollectionId;

  Akonadi::Collection createRootCollection( bool isDisconnectedImap = false, int intervalCheck = -1 )
  {
    // Root
    Akonadi::Collection collection = Akonadi::Collection( m_nextCollectionId++ );
    collection.setName( "resource" );
    collection.setRemoteId( "root-id" );
    collection.setContentMimeTypes( QStringList( Akonadi::Collection::mimeType() ) );
    collection.setRights( Akonadi::Collection::ReadOnly );
    collection.setParentCollection( Akonadi::Collection::root() );
    collection.addAttribute( new NoSelectAttribute( true ) );

    Akonadi::CachePolicy policy;
    policy.setInheritFromParent( false );
    policy.setSyncOnDemand( true );

    if ( isDisconnectedImap ) {
      policy.setLocalParts( QStringList() << Akonadi::MessagePart::Envelope
                                          << Akonadi::MessagePart::Header
                                          << Akonadi::MessagePart::Body );
      policy.setCacheTimeout( -1 );
    } else {
      policy.setLocalParts( QStringList() << Akonadi::MessagePart::Envelope
                                          << Akonadi::MessagePart::Header );
      policy.setCacheTimeout( 60 );
    }

    policy.setIntervalCheckTime( intervalCheck );

    collection.setCachePolicy( policy );

    return collection;
  }

  Akonadi::Collection createCollection( const QString &separator, const QString &path, bool isNoSelect = false )
  {
    // No path? That's the root of this resource then
    if ( path.isEmpty() ) {
      return createRootCollection();
    }

    QStringList pathParts = path.split(separator);

    const QString pathPart = pathParts.takeLast();
    const QString parentPath = pathParts.join(separator);

    // Here we should likely reuse already produced collections if possible to be 100% accurate
    // but in the tests we check only a limited amount of properties (namely remote id and name).
    const Akonadi::Collection parentCollection = createCollection( separator, parentPath );


    Akonadi::Collection collection( m_nextCollectionId++ );
    collection.setName( pathPart );
    collection.setRemoteId( separator + pathPart );

    collection.setParentCollection( parentCollection );
    collection.setContentMimeTypes( QStringList() << "message/rfc822" << Akonadi::Collection::mimeType() );

    // If the folder is the Inbox, make some special settings.
    if ( pathPart.compare( QLatin1String("INBOX") , Qt::CaseInsensitive ) == 0 ) {
      Akonadi::EntityDisplayAttribute *attr = new Akonadi::EntityDisplayAttribute;
      attr->setDisplayName( i18n( "Inbox" ) );
      attr->setIconName( "mail-folder-inbox" );
      collection.addAttribute( attr );
    }

    // If the folder is the user top-level folder, mark it as well, even although it is not officially noted in the RFC
    if ( (pathPart.compare( QLatin1String("user") , Qt::CaseInsensitive ) == 0) && isNoSelect ) {
      Akonadi::EntityDisplayAttribute *attr = new Akonadi::EntityDisplayAttribute;
      attr->setDisplayName( i18n( "Shared Folders" ) );
      attr->setIconName( "x-mail-distribution-list" );
      collection.addAttribute( attr );
    }

    // If this folder is a noselect folder, make some special settings.
    if ( isNoSelect ) {
      collection.addAttribute( new NoSelectAttribute( true ) );
      collection.setContentMimeTypes( QStringList() << Akonadi::Collection::mimeType() );
      collection.setRights( Akonadi::Collection::ReadOnly );
    }

    return collection;
  }

  void compareCollectionLists( const Akonadi::Collection::List &resultList,
                               const Akonadi::Collection::List &expectedList )
  {
    for ( int i=0; i<expectedList.size(); i++ ) {
      Akonadi::Collection expected = expectedList[i];
      bool found = false;

      for ( int j=0; j<resultList.size(); j++ ) {
        Akonadi::Collection result = resultList[j];

        if ( result.remoteId()==expected.remoteId() ) {
          found = true;

          QCOMPARE( result.name(), expected.name() );
          QCOMPARE( result.contentMimeTypes(), expected.contentMimeTypes() );
          QCOMPARE( result.rights(), expected.rights() );
          if ( expected.parentCollection()==Akonadi::Collection::root() ) {
            QCOMPARE( result.parentCollection(), expected.parentCollection() );
          } else {
            QCOMPARE( result.parentCollection().remoteId(), expected.parentCollection().remoteId() );
          }

          QCOMPARE( result.cachePolicy().inheritFromParent(), expected.cachePolicy().inheritFromParent() );
          QCOMPARE( result.cachePolicy().syncOnDemand(), expected.cachePolicy().syncOnDemand() );
          QCOMPARE( result.cachePolicy().localParts(), expected.cachePolicy().localParts() );
          QCOMPARE( result.cachePolicy().cacheTimeout(), expected.cachePolicy().cacheTimeout() );
          QCOMPARE( result.cachePolicy().intervalCheckTime(), expected.cachePolicy().intervalCheckTime() );

          QCOMPARE( result.hasAttribute<NoSelectAttribute>(), expected.hasAttribute<NoSelectAttribute>() );
          QCOMPARE( result.hasAttribute<Akonadi::EntityDisplayAttribute>(), expected.hasAttribute<Akonadi::EntityDisplayAttribute>() );

          break;
        }
      }

      QVERIFY2( found, QString("%1 not found!").arg(expected.remoteId()).toUtf8() );
    }

    QCOMPARE( resultList.size(), expectedList.size() );
  }
};

QTEST_KDEMAIN_CORE( TestRetrieveCollectionsTask )

#include "testretrievecollectionstask.moc"
