/*
    Copyright 2009 Constantin Berzan <exit3219@gmail.com>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "aborttest.h"

#include <QDBusInterface>
#include <QDBusReply>

#include <KDebug>

#include <Akonadi/AgentInstance>
#include <Akonadi/AgentManager>
#include <Akonadi/CollectionStatistics>
#include <Akonadi/Control>
#include <Akonadi/ItemDeleteJob>
#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemFetchScope>
#include <Akonadi/ItemModifyJob>
#include <Akonadi/Monitor>
#include <akonadi/qtest_akonadi.h>
#include <akonadi/private/collectionpathresolver_p.h>

#include <mailtransport/transport.h>
#include <mailtransport/transportmanager.h>

#include <kmime/kmime_message.h>
#include <boost/shared_ptr.hpp>

#include <outboxinterface/dispatcherinterface.h>
#include <outboxinterface/errorattribute.h>
#include <outboxinterface/localfolders.h>
#include <outboxinterface/messagequeuejob.h>
#include <outboxinterface/transportattribute.h>

#define SPAM_ADDRESS "idanoka@gmail.com"
// NOTE: This test relies on a large SMTP message taking long enough to deliver,
// for it to call abort.  So we need a valid receiver and a not-too-fast connection.
#define MESSAGE_MB 1

using namespace Akonadi;
using namespace KMime;
using namespace MailTransport;
using namespace OutboxInterface;


void AbortTest::initTestCase()
{
  QVERIFY( Control::start() );
  QTest::qWait( 1000 );
  LocalFolders::self()->fetch();
  QTest::kWaitForSignal( LocalFolders::self(), SIGNAL(foldersReady()) );

  // Clear outbox.
  QVERIFY( LocalFolders::self()->isReady() );
  outbox = LocalFolders::self()->outbox();
  ItemDeleteJob *djob = new ItemDeleteJob( outbox );
  djob->exec(); // may give error if outbox empty

  // Verify transports.
  akoTid = TransportManager::self()->defaultTransportId();
  Transport *t = TransportManager::self()->transportById( akoTid );
  QVERIFY( t );
  QCOMPARE( t->type(), int( Transport::EnumType::Akonadi ) );
  QList<int> tids = TransportManager::self()->transportIds();
  tids.removeAll( akoTid );
  QCOMPARE( tids.count(), 1 );
  smtpTid = tids.first();
  t = TransportManager::self()->transportById( smtpTid );
  QVERIFY( t );
  QCOMPARE( t->type(), int( Transport::EnumType::SMTP ) );

  // Set sink collection.
  t = TransportManager::self()->transportById( akoTid );
  const QString rid = t->host();
  const AgentInstance agent = AgentManager::self()->instance( rid );
  QVERIFY( agent.isValid() );
  CollectionPathResolver *resolver = new CollectionPathResolver( "sink", this );
  QVERIFY( resolver->exec() );
  sink = Collection( resolver->collection() );
  QVERIFY( sink.isValid() );
  QDBusInterface conf( "org.freedesktop.Akonadi.Resource." + rid,
          "/Settings", "org.kde.Akonadi.MailTransportDummy.Settings" );
  QVERIFY( conf.isValid() );
  QDBusReply<void> reply = conf.call( "setSink", sink.id() );
  QVERIFY( reply.isValid() );
  agent.reconfigure();

  // Watch sink collection.
  monitor = new Monitor( this );
  monitor->setCollectionMonitored( sink );
}

void AbortTest::testAbort()
{
  // Get the MDA interface.
  DispatcherInterface *iface = DispatcherInterface::self();
  QVERIFY( iface->dispatcherInstance().isValid() );
  QVERIFY( iface->dispatcherInstance().isOnline() );

  // Create a large message.
  kDebug() << "Building message.";
  Message::Ptr msg = Message::Ptr( new Message );
  QByteArray line( 70, 'a' );
  line.append( "\n" );
  QByteArray content( "\n" );
  for( int i = 0; i < MESSAGE_MB * 1024 * 1024 / line.length() + 10; i++ ) {
    content.append( line );
  }
  QVERIFY( content.length() > MESSAGE_MB * 1024 * 1024 ); // >10MiB
  msg->setContent( content );

  // Queue the message.
  kDebug() << "Queuing message.";
  MessageQueueJob *qjob = new MessageQueueJob( this );
  qjob->setMessage( msg );
  qjob->setTransportId( smtpTid );
  // default dispatch mode
  // default sent-mail collection
  qjob->setFrom( "naiba" );
  qjob->setTo( QStringList( SPAM_ADDRESS ) );
  QCOMPARE( iface->dispatcherInstance().status(), AgentInstance::Idle );
  AKVERIFYEXEC( qjob );

  // Wait for the MDA to begin dispatching.
  for( int ds = 0; iface->dispatcherInstance().status() == AgentInstance::Idle; ds++ ) {
    QTest::qWait( 100 );
    if( ds % 10 == 0 ) {
      kDebug() << "Waiting for the MDA to begin dispatching." << ds / 10 << "seconds elapsed.";
    }

    QVERIFY2( ds <= 100, "Timeout" );
  }
  QTest::qWait( 100 );

  // Tell the MDA to abort.
  QCOMPARE( iface->dispatcherInstance().status(), AgentInstance::Running );
  iface->dispatcherInstance().abort();
  for( int ds = 0; iface->dispatcherInstance().status() != AgentInstance::Idle; ds++ ) {
    QTest::qWait( 100 );
    if( ds % 10 == 0 ) {
      kDebug() << "Waiting for the MDA to become idle after aborting." << ds / 10 << "seconds elapsed.";
    }

    QVERIFY2( ds <= 100, "Timeout" );
  }
  QCOMPARE( iface->dispatcherInstance().status(), AgentInstance::Idle );

  // Verify that item has an ErrorAttribute.
  ItemFetchJob *fjob = new ItemFetchJob( outbox );
  fjob->fetchScope().fetchAllAttributes();
  AKVERIFYEXEC( fjob );
  QCOMPARE( fjob->items().count(), 1 );
  Item item = fjob->items().first();
  QVERIFY( item.hasAttribute<ErrorAttribute>() );
  ErrorAttribute *eA = item.attribute<ErrorAttribute>();
  kDebug() << "Stored error:" << eA->message();

  // "Fix" the item and send again, this time with the default (Akonadi) transport.
  item.removeAttribute<ErrorAttribute>();
  item.clearFlag( "error" );
  item.setFlag( "queued" );
  TransportAttribute *newTA = new TransportAttribute( akoTid );
  item.addAttribute( newTA );
  ItemModifyJob *cjob = new ItemModifyJob( item );
  QSignalSpy *addSpy = new QSignalSpy( monitor, SIGNAL(itemAdded(Akonadi::Item,Akonadi::Collection)) );
  AKVERIFYEXEC( cjob );

  // Verify that the item got sent.
  for( int ds = 0; addSpy->isEmpty(); ds++ ) {
    QTest::qWait( 100 );
    if( ds % 10 == 0 ) {
      kDebug() << "Waiting for an item to be sent." << ds / 10 << "seconds elapsed.";
    }

    QVERIFY2( ds <= 100, "Timeout" );
  }
  QCOMPARE( addSpy->count(), 1 );
  QCOMPARE( iface->dispatcherInstance().status(), AgentInstance::Idle );
}

void AbortTest::testAbortWhileIdle()
{
  // Get the MDA interface.
  DispatcherInterface *iface = DispatcherInterface::self();
  QVERIFY( iface->dispatcherInstance().isValid() );
  QVERIFY( iface->dispatcherInstance().isOnline() );

  // Abort thin air.
  QCOMPARE( iface->dispatcherInstance().status(), AgentInstance::Idle );
  iface->dispatcherInstance().abort();
  QCOMPARE( iface->dispatcherInstance().status(), AgentInstance::Idle );

  // Queue a message (to check that subsequent messages are being sent).
  QVERIFY( monitor );
  QSignalSpy *addSpy = new QSignalSpy( monitor, SIGNAL(itemAdded(Akonadi::Item,Akonadi::Collection)) );
  Message::Ptr msg = Message::Ptr( new Message );
  msg->setContent( "\ntestAbortWhileIdle" );
  MessageQueueJob *qjob = new MessageQueueJob( this );
  qjob->setMessage( msg );
  qjob->setTransportId( akoTid );
  // default dispatch mode
  // default sent-mail collection
  qjob->setFrom( "naiba" );
  qjob->setTo( QStringList( "dracu" ) );
  QCOMPARE( iface->dispatcherInstance().status(), AgentInstance::Idle );
  AKVERIFYEXEC( qjob );

  // Verify that the item got sent.
  for( int s = 0; addSpy->isEmpty(); s++ ) {
    QTest::qWait( 1000 );
    QVERIFY2( s <= 10, "Timeout" );
  }
  QCOMPARE( addSpy->count(), 1 );
  QCOMPARE( iface->dispatcherInstance().status(), AgentInstance::Idle );
}

QTEST_AKONADIMAIN( AbortTest, NoGUI )

#include "aborttest.moc"
