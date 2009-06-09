/* Copyright 2009 Thomas McGuire <mcguire@kde.org>

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU Library General Public License as published
   by the Free Software Foundation; either version 2 of the License or
   ( at your option ) version 3 or, at the discretion of KDE e.V.
   ( which shall act as a proxy as in section 14 of the GPLv3 ), any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/
#include "pop3test.h"

#include <Akonadi/AgentInstanceCreateJob>
#include <Akonadi/AgentManager>
#include <Akonadi/CollectionFetchJob>
#include <Akonadi/ItemDeleteJob>
#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemFetchScope>
#include <akonadi/qtest_akonadi.h>
#include <kmime/kmime_message.h>

#include <KStandardDirs>

#include <boost/shared_ptr.hpp>

typedef boost::shared_ptr<KMime::Message> MsgPtr;

QTEST_AKONADIMAIN( Pop3Test, NoGUI )

using namespace Akonadi;

void Pop3Test::initTestCase()
{
  //
  // Create the maildir and pop3 resources
  //
  AgentType maildirType = AgentManager::self()->type( "akonadi_maildir_resource" );
  AgentInstanceCreateJob *agentCreateJob = new AgentInstanceCreateJob( maildirType );
  QVERIFY( agentCreateJob->exec() );
  mMaildirIdentifier = agentCreateJob->instance().identifier();

  AgentType popType = AgentManager::self()->type( "akonadi_pop3_resource" );
  agentCreateJob = new AgentInstanceCreateJob( popType );
  QVERIFY( agentCreateJob->exec() );
  mPop3Identifier = agentCreateJob->instance().identifier();

  //
  // Configure the maildir resource
  //
  QString maildirRootPath = KGlobal::dirs()->saveLocation( "data", "tester", false ) + "maildirtest";
  mMaildirPath = maildirRootPath + "/new";
  mMaildirSettingsInterface = new OrgKdeAkonadiMaildirSettingsInterface(
      "org.freedesktop.Akonadi.Resource." + mMaildirIdentifier,
       "/Settings", QDBusConnection::sessionBus(), this );
  QDBusReply<void> setPathReply = mMaildirSettingsInterface->setPath( maildirRootPath );
  QVERIFY( setPathReply.isValid() );
  AgentManager::self()->instance( mMaildirIdentifier ).reconfigure();
  QDBusReply<QString> getPathReply = mMaildirSettingsInterface->path();
  QCOMPARE( getPathReply.value(), maildirRootPath );
  AgentManager::self()->instance( mMaildirIdentifier ).synchronize();
  
  //
  // Find the root maildir collection
  //
  bool found = false;
  QTime time;
  time.start();
  while ( !found ) {
    CollectionFetchJob *job = new CollectionFetchJob( Collection::root(), CollectionFetchJob::Recursive );
    QVERIFY( job->exec() );
    Collection::List collections = job->collections();
    foreach( const Collection &col, collections ) {
      if ( col.resource() == AgentManager::self()->instance( mMaildirIdentifier ).identifier() ) {
        mMaildirCollection = col;
        found = true;
        break;
      }
    }

    QVERIFY( time.elapsed() < 10 * 1000 ); // maildir should not need more than 10 secs to sync
  }

  //
  // Start the fake POP3 server
  //
  mFakeServer = new FakeServer( this );
  mFakeServer->start();

  //
  // Configure the pop3 resource
  //
  mPOP3SettingsInterface = new OrgKdeAkonadiPOP3SettingsInterface(
      "org.freedesktop.Akonadi.Resource." + mPop3Identifier,
       "/Settings", QDBusConnection::sessionBus(), this );

  QDBusReply<uint> reply0 = mPOP3SettingsInterface->port();
  QVERIFY( reply0.isValid() );
  QVERIFY( reply0.value() == 110 );
  
  mPOP3SettingsInterface->setPort( 5989 );
  AgentManager::self()->instance( mPop3Identifier ).reconfigure();
  QDBusReply<uint> reply = mPOP3SettingsInterface->port();
  QVERIFY( reply.isValid() );
  QVERIFY( reply.value() == 5989 );

  mPOP3SettingsInterface->setHost( "localhost" );
  AgentManager::self()->instance( mPop3Identifier ).reconfigure();
  QDBusReply<QString> reply2 = mPOP3SettingsInterface->host();
  QVERIFY( reply2.isValid() );
  QVERIFY( reply2.value() == "localhost" );
  mPOP3SettingsInterface->setLogin( "HansWurst" );
  AgentManager::self()->instance( mPop3Identifier ).reconfigure();
  QDBusReply<QString> reply3 = mPOP3SettingsInterface->login();
  QVERIFY( reply3.isValid() );
  QVERIFY( reply3.value() == "HansWurst" );

  mPOP3SettingsInterface->setPassword( "Geheim" );
  AgentManager::self()->instance( mPop3Identifier ).reconfigure();
  QDBusReply<QString> reply4 = mPOP3SettingsInterface->password();
  QVERIFY( reply4.isValid() );
  QVERIFY( reply4.value() == "Geheim" );

  mPOP3SettingsInterface->setTargetCollection( mMaildirCollection.id() );
  AgentManager::self()->instance( mPop3Identifier ).reconfigure();
  QDBusReply<qlonglong> reply5 = mPOP3SettingsInterface->targetCollection();
  QVERIFY( reply5.isValid() );
  QVERIFY( reply5.value() == mMaildirCollection.id() );
}

void Pop3Test::cleanupTestCase()
{
  delete mFakeServer;
  mFakeServer = 0;
}

static const QByteArray simpleMail1 =
  "From: \"Bill Lumbergh\" <BillLumbergh@initech.com>\r\n"
  "To: \"Peter Gibbons\" <PeterGibbons@initech.com>\r\n"
  "Subject: TPS Reports - New Cover Sheets\r\n"
  "MIME-Version: 1.0\r\n"
  "Content-Type: text/plain\r\n"
  "Date: Mon, 23 Mar 2009 18:04:05 +0300\r\n"
  "\r\n"
  "Hi, Peter. What's happening? We need to talk about your TPS reports.\r\n";

static const QByteArray simpleMail2 =
  "From: \"Amy McCorkell\" <yooper@mtao.net>\r\n"
  "To: gov.palin@yaho.com\r\n"
  "Subject: HI SARAH\r\n"
  "MIME-Version: 1.0\r\n"
  "Content-Type: text/plain\r\n"
  "Date: Mon, 23 Mar 2009 18:04:05 +0300\r\n"
  "\r\n"
  "Hey Sarah,\r\n"
  "bla bla bla bla bla\r\n";

static const QByteArray simpleMail3 =
  "From: chunkylover53@aol.com\r\n"
  "To: tylerdurden@paperstreetsoapcompany.com\r\n"
  "Subject: ILOVEYOU\r\n"
  "MIME-Version: 1.0\r\n"
  "Content-Type: text/plain\r\n"
  "Date: Mon, 23 Mar 2009 18:04:05 +0300\r\n"
  "\r\n"
  "kindly check the attached LOVELETTER coming from me.\r\n";

static const QByteArray simpleMail4 =
  "From: karl@aol.com\r\n"
  "To: lenny@aol.com\r\n"
  "Subject: Who took the donuts?\r\n"
  "\r\n"
  "Hi Lenny, do you know who took all the donuts?\r\n";

void Pop3Test::cleanupMaildir( Akonadi::Item::List items )
{
  // Delete all mails so the maildir is clean for the next test
  foreach( const Item &item, items ) {
    ItemDeleteJob *job = new ItemDeleteJob( item );
    QVERIFY( job->exec() );
  }
  QTime time;
  time.start();
  int lastCount = -1;
  forever {
    qApp->processEvents();
    QDir maildir( mMaildirPath );
    maildir.refresh();
    int curCount = maildir.entryList( QDir::Files | QDir::NoDotAndDotDot ).count();

    // Restart the timer when a mail arrives, as it shows that the maildir resource is
    // still alive and kicking.
    if ( curCount != lastCount ) {
      time.restart();
      lastCount = curCount;
    }

    if ( curCount == 0 )
      break;

    QVERIFY( time.elapsed() < 60000 || time.elapsed() > 80000000 );
  }
}

void Pop3Test::checkMailsInMaildir( const QList<QByteArray> &mails )
{
  // Now, test that all mails actually ended up in the maildir. Since the maildir resource
  // might be slower, give it a timeout so it can write the files to disk
  QTime time;
  time.start();
  int lastCount = -1;
  forever {
    qApp->processEvents();
    QDir maildir( mMaildirPath );
    maildir.refresh();
    int curCount = static_cast<int>( maildir.entryList( QDir::Files | QDir::NoDotAndDotDot ).count() );

    // Restart the timer when a mail arrives, as it shows that the maildir resource is
    // still alive and kicking.
    if ( curCount != lastCount ) {
      time.start();
      lastCount = curCount;
    }

    if (  curCount == mails.count() ) {
      break;
    }
    QVERIFY( static_cast<int>( maildir.entryList( QDir::NoDotAndDotDot ).count() ) <= mails.count() );
    QVERIFY( time.elapsed() < 60000 || time.elapsed() > 80000000 );
  }

  // TODO: check file contents as well or is this overkill?
}

Akonadi::Item::List Pop3Test::checkMailsOnAkonadiServer( const QList<QByteArray> &mails )
{
  // The fake server got disconnected, which means the pop3 resource has entered the QUIT
  // stage. That means all messages should be on the server now, so test that.
  ItemFetchJob *job = new ItemFetchJob( mMaildirCollection );
  job->fetchScope().fetchFullPayload();
  Q_ASSERT( job->exec() );
  Item::List items = job->items();
  Q_ASSERT( mails.size() == items.size() );

  QSet<QByteArray> ourMailBodies;
  QSet<QByteArray> itemMailBodies;

  foreach( const Item &item, items ) {
    MsgPtr itemMail = item.payload<MsgPtr>();
    QByteArray itemMailBody = itemMail->body();

    // For some reason, the body in the maildir has one additional newline.
    // Get rid of this so we can compare them.
    // FIXME: is this a bug? Find out where the newline comes from!
    itemMailBody.chop( 1 );
    itemMailBodies.insert( itemMailBody );
  }

  foreach( const QByteArray &mail, mails ) {
    MsgPtr ourMail( new KMime::Message() );
    ourMail->setContent( KMime::CRLFtoLF( mail ) );
    ourMail->parse();
    QByteArray ourMailBody = ourMail->body();
    ourMailBodies.insert( ourMailBody );
  }

  Q_ASSERT( ourMailBodies == itemMailBodies );
  return items;
}

void Pop3Test::syncAndWaitForFinish()
{
  QSignalSpy disconnectSpy( mFakeServer, SIGNAL( disconnected() ) );
  QSignalSpy progressSpy( mFakeServer, SIGNAL( progress() ) );
  AgentManager::self()->instance( mPop3Identifier ).synchronize();

  // The pop3 resource, ioslave and the fakeserver are all in different processes or threads.
  // We simply wait until the FakeServer got disconnected or until a timeout.
  // Since POP3 fetching can take longer, we reset the timeout timer when the FakeServer
  // does some processing.
  QTime time;
  time.start();
  forever {
    qApp->processEvents();
    if ( disconnectSpy.count() >= 1 ) {
      qDebug() << "Server quit normally.";
      break;
    }
    if ( progressSpy.count() > 0 ) {
      time.restart();
      progressSpy.clear();
    }
    if ( time.elapsed() >= 60000 ) {
      Q_ASSERT_X( false, "poptest", "FakeServer timed out." );
      break;
    }
  }
}

QString Pop3Test::loginSequence() const
{
  return
    "C: USER HansWurst\r\n"
    "S: +OK May I have your password, please?\r\n"
    "C: PASS Geheim\r\n"
    "S: +OK Mailbox locked and ready\r\n";
}

QString Pop3Test::retrieveSequence( const QList<QByteArray> &mails,
                                    const QList<int> &exceptions ) const
{
  QString result;
  for( int i = 1; i <= mails.size(); i++ ) {
    if ( !exceptions.contains( i ) ) {
      result += QString(
        "C: RETR %RETR%\r\n"
        "S: +OK Here is your spam\r\n"
            "%MAIL%\r\n"
            ".\r\n" );
    }
  }
  return result;
}

QString Pop3Test::deleteSequence( int numToDelete ) const
{
  QString result;
  for ( int i = 0; i < numToDelete; i++ ) {
    result +=
        "C: DELE %DELE%\r\n"
        "S: +OK message sent to /dev/null\r\n";
  }
  return result;
}

QString Pop3Test::quitSequence() const
{
  return
    "C: QUIT\r\n"
    "S: +OK Have a nice day.\r\n";
}

QString Pop3Test::listSequence( const QList<QByteArray> &mails ) const
{
  QString result = "C: LIST\r\n"
                   "S: +OK You got new spam\r\n";
  for ( int i = 1; i <= mails.size(); i++ ) {
    result += QString( "%1 %MAILSIZE%\r\n" ).arg( i );
  }
  result += ".\r\n";
  return result;
}

QString Pop3Test::uidSequence( const QStringList& uids ) const
{
  QString result = "C: UIDL\r\n"
                   "S: +OK\r\n";
  for ( int i = 1; i <= uids.size(); i++ ) {
    result += QString( "%1 %2\r\n" ).arg( i ).arg( uids[i - 1] );
  }
  result += ".\r\n";
  return result;
}


void Pop3Test::testSimpleDownload()
{
  QList<QByteArray> mails;
  mails << simpleMail1 << simpleMail2 << simpleMail3;
  QStringList uids;
  uids << "UID1" << "UID2" << "UID3";
  mFakeServer->setAllowedDeletions("1,2,3");
  mFakeServer->setAllowedRetrieves("1,2,3");
  mFakeServer->setMails( mails );
  mFakeServer->setNextConversation(
    loginSequence() +
    listSequence( mails ) +
    uidSequence( uids ) +
    retrieveSequence( mails ) +
    deleteSequence( mails.size() ) +
    quitSequence()
  );

  syncAndWaitForFinish();
  Akonadi::Item::List items = checkMailsOnAkonadiServer( mails );
  checkMailsInMaildir( mails );
  cleanupMaildir( items );
}

void Pop3Test::testBigFetch()
{
  QList<QByteArray> mails;
  QStringList uids;
  QString allowedRetrs;
  for( int i = 0; i < 1000; i++ ) {
    QByteArray newMail = simpleMail1;
    newMail.append( QString::number( i ).toAscii() );
    mails << newMail;
    uids << QString( "UID%1" ).arg( i );
    allowedRetrs += QString::number( i + 1 ) + ',';
  }
  allowedRetrs.chop( 1 );

  mFakeServer->setMails( mails );
  mFakeServer->setAllowedRetrieves( allowedRetrs );
  mFakeServer->setAllowedDeletions( allowedRetrs );
  mFakeServer->setNextConversation(
    loginSequence() +
    listSequence( mails ) +
    uidSequence( uids ) +
    retrieveSequence( mails ) +
    deleteSequence( mails.size() ) +
    quitSequence()
  );

  syncAndWaitForFinish();
  Akonadi::Item::List items = checkMailsOnAkonadiServer( mails );
  checkMailsInMaildir( mails );
  cleanupMaildir( items );
}


void Pop3Test::testSimpleLeaveOnServer()
{
  mPOP3SettingsInterface->setLeaveOnServer( true );

  QList<QByteArray> mails;
  mails << simpleMail1 << simpleMail2 << simpleMail3;
  QStringList uids;
  uids << "UID1" << "UID2" << "UID3";
  mFakeServer->setMails( mails );
  mFakeServer->setAllowedRetrieves("1,2,3");
  mFakeServer->setNextConversation(
    loginSequence() +
    listSequence( mails ) +
    uidSequence( uids ) +
    retrieveSequence( mails ) +
    quitSequence()
  );

  syncAndWaitForFinish();
  Akonadi::Item::List items = checkMailsOnAkonadiServer( mails );
  checkMailsInMaildir( mails );

  // The resource should have saved the UIDs of the seen messages
  QVERIFY( qEqual( uids.begin(), uids.end(), mPOP3SettingsInterface->seenUidList().value().begin() ) );

  //
  // OK, next mail check: We have to check that the old seen messages are not downloaded again,
  // only new mails.
  //
  QList<QByteArray> newMails( mails );
  newMails << simpleMail4;
  QStringList newUids( uids );
  newUids << "newUID";
  QList<int> idsToNotDownload;
  idsToNotDownload << 1 << 2 << 3;
  mFakeServer->setMails( newMails );
  mFakeServer->setAllowedRetrieves("4");
  mFakeServer->setNextConversation(
    loginSequence() +
    listSequence( newMails ) +
    uidSequence( newUids ) +
    retrieveSequence( newMails, idsToNotDownload ) +
    quitSequence(),
    idsToNotDownload
  );

  syncAndWaitForFinish();
  items = checkMailsOnAkonadiServer( newMails );
  checkMailsInMaildir( newMails );
  QVERIFY( qEqual( newUids.begin(), newUids.end(), mPOP3SettingsInterface->seenUidList().value().begin() ) );

  //
  // Ok, next test: When turning off leaving on the server, all mails should be deleted, but
  // none downloaded.
  //
  mPOP3SettingsInterface->setLeaveOnServer( false );

  mFakeServer->setAllowedDeletions( "1,2,3,4" );
  mFakeServer->setNextConversation(
    loginSequence() +
    listSequence( newMails ) +
    uidSequence( newUids ) +
    deleteSequence( newMails.size() ) +
    quitSequence()
  );

  syncAndWaitForFinish();
  items = checkMailsOnAkonadiServer( newMails );
  checkMailsInMaildir( newMails );
  cleanupMaildir( items );
  QVERIFY( mPOP3SettingsInterface->seenUidList().value().isEmpty() );
}
