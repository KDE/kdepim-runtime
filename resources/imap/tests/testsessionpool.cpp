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

#include <qtest_kde.h>

#include <kimap/capabilitiesjob.h>
#include <kimaptest/fakeserver.h>

#include "dummypasswordrequester.h"
#include "imapaccount.h"
#include "sessionpool.h"

Q_DECLARE_METATYPE(ImapAccount*)
Q_DECLARE_METATYPE(DummyPasswordRequester*)
Q_DECLARE_METATYPE(KIMAP::Session*)


class TestSessionPool : public QObject
{
  Q_OBJECT

private:
  ImapAccount *createDefaultAccount()
  {
    ImapAccount *account = new ImapAccount;

    account->setServer( "127.0.0.1" );
    account->setPort( 5989 );
    account->setUserName( "test@kdab.com" );
    account->setSubscriptionEnabled( true );
    account->setEncryptionMode( KIMAP::LoginJob::Unencrypted );
    account->setAuthenticationMode( KIMAP::LoginJob::ClearText );

    return account;
  }

  DummyPasswordRequester *createDefaultRequester()
  {
    DummyPasswordRequester *requester = new DummyPasswordRequester( this );
    requester->setPassword( "foobar" );
    return requester;
  }


private slots:
  void setupTestCase()
  {
    qRegisterMetaType<KIMAP::Session*>();
  }

  void shouldPrepareFirstSessionOnConnect_data()
  {
    QTest::addColumn<ImapAccount*>("account");
    QTest::addColumn<DummyPasswordRequester*>("requester");
    QTest::addColumn< QList<QByteArray> >("scenario");
    QTest::addColumn<QString>("password");
    QTest::addColumn<int>("errorCode");
    QTest::addColumn<QStringList>("capabilities");

    ImapAccount *account = 0;
    DummyPasswordRequester *requester = 0;
    QList<QByteArray> scenario;
    QString password;
    int errorCode = SessionPool::NoError;
    QStringList capabilities;

    account = createDefaultAccount();
    requester = createDefaultRequester();
    scenario.clear();
    scenario << FakeServer::greeting()
             << "C: A000001 LOGIN test@kdab.com foobar"
             << "S: A000001 OK User Logged in"
             << "C: A000002 CAPABILITY"
             << "S: * CAPABILITY IMAP4 IMAP4rev1 NAMESPACE UIDPLUS IDLE"
             << "S: A000002 OK Completed"
             << "C: A000003 NAMESPACE"
             << "S: * NAMESPACE ( (\"INBOX/\" \"/\") ) ( (\"user/\" \"/\") ) ( (\"\" \"/\") )"
             << "S: A000003 OK Completed";
    password = "foobar";
    errorCode = SessionPool::NoError;
    capabilities.clear();
    capabilities << "IMAP4" << "IMAP4REV1" << "NAMESPACE" << "UIDPLUS" << "IDLE";
    QTest::newRow("normal case") << account << requester << scenario
                                 << password << errorCode << capabilities;


    account = createDefaultAccount();
    requester = createDefaultRequester();
    scenario.clear();
    scenario << FakeServer::greeting()
             << "C: A000001 LOGIN test@kdab.com foobar"
             << "S: A000001 OK User Logged in"
             << "C: A000002 CAPABILITY"
             << "S: * CAPABILITY IMAP4 IMAP4rev1 UIDPLUS IDLE"
             << "S: A000002 OK Completed";
    password = "foobar";
    errorCode = SessionPool::NoError;
    capabilities.clear();
    capabilities << "IMAP4" << "IMAP4REV1" << "UIDPLUS" << "IDLE";
    QTest::newRow("no NAMESPACE support") << account << requester << scenario
                                          << password << errorCode << capabilities;


    account = createDefaultAccount();
    requester = createDefaultRequester();
    scenario.clear();
    scenario << FakeServer::greeting()
             << "C: A000001 LOGIN test@kdab.com foobar"
             << "S: A000001 OK User Logged in"
             << "C: A000002 CAPABILITY"
             << "S: * CAPABILITY IMAP4 IMAP4rev1 IDLE"
             << "S: A000002 OK Completed"
             << "C: A000003 LOGOUT";
    password = "foobar";
    errorCode = SessionPool::IncompatibleServerError;
    capabilities.clear();
    QTest::newRow("incompatible server") << account << requester << scenario
                                         << password << errorCode << capabilities;


    QList<DummyPasswordRequester::RequestType> requests;
    QList<DummyPasswordRequester::ResultType> results;

    account = createDefaultAccount();
    requester = createDefaultRequester();
    requests.clear();
    results.clear();
    requests << DummyPasswordRequester::StandardRequest << DummyPasswordRequester::WrongPasswordRequest;
    results << DummyPasswordRequester::PasswordRetrieved << DummyPasswordRequester::UserRejected;
    requester->setScenario( requests, results );
    scenario.clear();
    scenario << FakeServer::greeting()
             << "C: A000001 LOGIN test@kdab.com foobar"
             << "S: A000001 NO Login failed"
             << "C: A000002 LOGOUT";
    password = "foobar";
    errorCode = SessionPool::LoginFailError;
    capabilities.clear();
    QTest::newRow("login fail, user reject password entry") << account << requester << scenario
                                                            << password << errorCode << capabilities;

    account = createDefaultAccount();
    requester = createDefaultRequester();
    requests.clear();
    results.clear();
    requests << DummyPasswordRequester::StandardRequest << DummyPasswordRequester::WrongPasswordRequest;
    results << DummyPasswordRequester::PasswordRetrieved << DummyPasswordRequester::PasswordRetrieved;
    requester->setScenario( requests, results );
    scenario.clear();
    scenario << FakeServer::greeting()
             << "C: A000001 LOGIN test@kdab.com foobar"
             << "S: A000001 NO Login failed"
             << "C: A000002 LOGIN test@kdab.com foobar"
             << "S: A000002 OK Login succeeded"
             << "C: A000003 CAPABILITY"
             << "S: * CAPABILITY IMAP4 IMAP4rev1 UIDPLUS IDLE"
             << "S: A000003 OK Completed";
    password = "foobar";
    errorCode = SessionPool::NoError;
    capabilities.clear();
    capabilities << "IMAP4" << "IMAP4REV1" << "UIDPLUS" << "IDLE";
    QTest::newRow("login fail, user provide new password") << account << requester << scenario
                                                           << password << errorCode << capabilities;

    account = createDefaultAccount();
    requester = createDefaultRequester();
    requests.clear();
    results.clear();
    requests << DummyPasswordRequester::StandardRequest << DummyPasswordRequester::WrongPasswordRequest;
    results << DummyPasswordRequester::PasswordRetrieved << DummyPasswordRequester::EmptyPasswordEntered;
    requester->setScenario( requests, results );
    scenario.clear();
    scenario << FakeServer::greeting()
             << "C: A000001 LOGIN test@kdab.com foobar"
             << "S: A000001 NO Login failed"
             << "C: A000002 LOGOUT";
    password = "foobar";
    errorCode = SessionPool::LoginFailError;
    capabilities.clear();
    QTest::newRow("login fail, user provided empty password") << account << requester << scenario
                                                              << password << errorCode << capabilities;

    account = createDefaultAccount();
    requester = createDefaultRequester();
    requests.clear();
    results.clear();
    requests << DummyPasswordRequester::StandardRequest << DummyPasswordRequester::WrongPasswordRequest;
    results << DummyPasswordRequester::PasswordRetrieved << DummyPasswordRequester::ReconnectNeeded;
    requester->setScenario( requests, results );
    scenario.clear();
    scenario << FakeServer::greeting()
             << "C: A000001 LOGIN test@kdab.com foobar"
             << "S: A000001 NO Login failed"
             << "C: A000002 LOGOUT";
    password = "foobar";
    errorCode = SessionPool::ReconnectNeededError;
    capabilities.clear();
    QTest::newRow("login fail, user change the settings") << account << requester << scenario
                                                          << password << errorCode << capabilities;
  }


  void shouldPrepareFirstSessionOnConnect()
  {
    QFETCH( ImapAccount*, account );
    QFETCH( DummyPasswordRequester*, requester );
    QFETCH( QList<QByteArray>, scenario );
    QFETCH( QString, password );
    QFETCH( int, errorCode );
    QFETCH( QStringList, capabilities );

    QSignalSpy requesterSpy( requester, SIGNAL(done(int, QString)) );

    FakeServer server;
    server.setScenario( scenario );
    server.start();

    SessionPool pool( 2 );

    QSignalSpy poolSpy( &pool, SIGNAL(connectDone(int, QString)) );

    pool.setPasswordRequester( requester );
    QVERIFY( pool.connect( account ) );

    QTest::qWait( 100 );
    QVERIFY( requesterSpy.count()>0 );
    if ( requesterSpy.count()==1 ) {
      QCOMPARE( requesterSpy.at(0).at(0).toInt(), 0 );
      QCOMPARE( requesterSpy.at(0).at(1).toString(), password );
    }

    QCOMPARE( poolSpy.count(), 1 );
    QCOMPARE( poolSpy.at(0).at(0).toInt(), errorCode );

    QCOMPARE( pool.serverCapabilities(), capabilities );
    QVERIFY( pool.serverNamespaces().isEmpty() );

    QVERIFY( server.isAllScenarioDone() );

    server.quit();
  }

  void shouldManageSeveralSessions()
  {
    FakeServer server;
    server.addScenario( QList<QByteArray>()
                        << FakeServer::greeting()
                        << "C: A000001 LOGIN test@kdab.com foobar"
                        << "S: A000001 OK User Logged in"
                        << "C: A000002 CAPABILITY"
                        << "S: * CAPABILITY IMAP4 IMAP4rev1 NAMESPACE UIDPLUS IDLE"
                        << "S: A000002 OK Completed"
                        << "C: A000003 NAMESPACE"
                        << "S: * NAMESPACE ( (\"INBOX/\" \"/\") ) ( (\"user/\" \"/\") ) ( (\"\" \"/\") )"
                        << "S: A000003 OK Completed"
    );

    server.addScenario( QList<QByteArray>()
                        << FakeServer::greeting()
                        << "C: A000001 LOGIN test@kdab.com foobar"
                        << "S: A000001 OK User Logged in"
    );

    server.start();



    ImapAccount *account = createDefaultAccount();
    DummyPasswordRequester *requester = createDefaultRequester();

    QSignalSpy requesterSpy( requester, SIGNAL(done(int, QString)) );

    SessionPool pool( 2 );
    pool.setPasswordRequester( requester );

    QSignalSpy connectSpy( &pool, SIGNAL(connectDone(int, QString)) );
    QSignalSpy sessionSpy( &pool, SIGNAL(sessionRequestDone(qint64, KIMAP::Session*, int, QString)) );


    // Before connect we can't get any session
    qint64 requestId = pool.requestSession();
    QCOMPARE( requestId, qint64(-1) );


    // Initial connect should trigger only a password request and a connect
    QVERIFY( pool.connect( account ) );
    QTest::qWait( 100 );
    QCOMPARE( requesterSpy.count(),  1 );
    QCOMPARE( connectSpy.count(), 1 );
    QCOMPARE( sessionSpy.count(), 0 );


    // Requesting a first session shouldn't create a new one,
    // only sessionRequestDone is emitted right away
    requestId = pool.requestSession();
    QCOMPARE( requestId, qint64(1) );
    QTest::qWait( 100 );
    QCOMPARE( requesterSpy.count(),  1 );
    QCOMPARE( connectSpy.count(), 1 );
    QCOMPARE( sessionSpy.count(), 1 );

    QCOMPARE( sessionSpy.at(0).at(0).toLongLong(), requestId );
    QVERIFY( sessionSpy.at(0).at(1).value<KIMAP::Session*>() != 0 );
    QCOMPARE( sessionSpy.at(0).at(2).toInt(), 0 );
    QCOMPARE( sessionSpy.at(0).at(3).toString(), QString() );


    // Requesting an extra session should create a new one
    // So for instance password will be requested
    requestId = pool.requestSession();
    QCOMPARE( requestId, qint64(2) );
    QTest::qWait( 100 );
    QCOMPARE( requesterSpy.count(),  2 );
    QCOMPARE( connectSpy.count(), 1 );
    QCOMPARE( sessionSpy.count(), 2 );

    QCOMPARE( sessionSpy.at(1).at(0).toLongLong(), requestId );
    QVERIFY( sessionSpy.at(1).at(1).value<KIMAP::Session*>() != 0 );
    // Should be different sessions...
    QVERIFY( sessionSpy.at(0).at(1).value<KIMAP::Session*>() != sessionSpy.at(1).at(1).value<KIMAP::Session*>() );
    QCOMPARE( sessionSpy.at(1).at(2).toInt(), 0 );
    QCOMPARE( sessionSpy.at(1).at(3).toString(), QString() );


    // Requesting yet another session should fail as we reached the
    // maximum pool size, and they're all reserved
    requestId = pool.requestSession();
    QCOMPARE( requestId, qint64(3) );
    QTest::qWait( 100 );
    QCOMPARE( requesterSpy.count(),  2 );
    QCOMPARE( connectSpy.count(), 1 );
    QCOMPARE( sessionSpy.count(), 3 );

    QCOMPARE( sessionSpy.at(2).at(0).toLongLong(), requestId );
    QVERIFY( sessionSpy.at(2).at(1).value<KIMAP::Session*>()==0 );
    QCOMPARE( sessionSpy.at(2).at(2).toInt(), (int)SessionPool::NoAvailableSessionError );
    QVERIFY( !sessionSpy.at(2).at(3).toString().isEmpty() );


    // OTOH, if we release one now, and then request another one
    // it should succeed without even creating a new session
    KIMAP::Session *session = sessionSpy.at(0).at(1).value<KIMAP::Session*>();
    pool.releaseSession( session );
    requestId = pool.requestSession();
    QCOMPARE( requestId, qint64(4) );
    QTest::qWait( 100 );
    QCOMPARE( requesterSpy.count(),  2 );
    QCOMPARE( connectSpy.count(), 1 );
    QCOMPARE( sessionSpy.count(), 4 );

    QCOMPARE( sessionSpy.at(3).at(0).toLongLong(), requestId );
    // Only one session was available, so that should be the one we get gack
    QVERIFY( sessionSpy.at(3).at(1).value<KIMAP::Session*>() == session );
    QCOMPARE( sessionSpy.at(3).at(2).toInt(), 0 );
    QCOMPARE( sessionSpy.at(3).at(3).toString(), QString() );



    QVERIFY( server.isAllScenarioDone() );

    server.quit();
  }

  void shouldNotifyConnectionLost()
  {
    FakeServer server;
    server.addScenario( QList<QByteArray>()
                        << FakeServer::greeting()
                        << "C: A000001 LOGIN test@kdab.com foobar"
                        << "S: A000001 OK User Logged in"
                        << "C: A000002 CAPABILITY"
                        << "S: * CAPABILITY IMAP4 IMAP4rev1 UIDPLUS IDLE"
                        << "S: A000002 OK Completed"
                        << "C: A000003 CAPABILITY"
                        << "S: * CAPABILITY IMAP4 IMAP4rev1 UIDPLUS IDLE"
                        << "X"
    );

    server.start();

    ImapAccount *account = createDefaultAccount();
    DummyPasswordRequester *requester = createDefaultRequester();

    SessionPool pool( 1 );
    pool.setPasswordRequester( requester );

    QSignalSpy connectSpy( &pool, SIGNAL(connectDone(int, QString)) );
    QSignalSpy sessionSpy( &pool, SIGNAL(sessionRequestDone(qint64, KIMAP::Session*, int, QString)) );
    QSignalSpy lostSpy( &pool, SIGNAL(connectionLost(KIMAP::Session*)) );


    // Initial connect should trigger only a password request and a connect
    QVERIFY( pool.connect( account ) );
    QTest::qWait( 100 );
    QCOMPARE( connectSpy.count(), 1 );
    QCOMPARE( sessionSpy.count(), 0 );

    qint64 requestId = pool.requestSession();
    QTest::qWait( 100 );
    QCOMPARE( sessionSpy.count(), 1 );

    QCOMPARE( sessionSpy.at(0).at(0).toLongLong(), requestId );
    KIMAP::Session *s = sessionSpy.at(0).at(1).value<KIMAP::Session*>();

    KIMAP::CapabilitiesJob *job = new KIMAP::CapabilitiesJob( s );
    job->start();
    QTest::qWait( 100 );
    QCOMPARE( lostSpy.count(), 1 );
    QCOMPARE( lostSpy.at(0).at(0).value<KIMAP::Session*>(), s );


    QVERIFY( server.isAllScenarioDone() );

    server.quit();
  }
};

QTEST_KDEMAIN_CORE( TestSessionPool )

#include "testsessionpool.moc"
