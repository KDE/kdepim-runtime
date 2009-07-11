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

#include "dupetest.h"

#include <QDBusInterface>
#include <QDBusReply>

#include <KDebug>

#include <Akonadi/Control>
#include <Akonadi/AgentInstance>
#include <Akonadi/AgentManager>
#include <Akonadi/CollectionFetchJob>
#include <Akonadi/ItemDeleteJob>
#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemFetchScope>
#include <akonadi/qtest_akonadi.h>
#include <akonadi/private/collectionpathresolver_p.h>

#include <mailtransport/messagequeuejob.h>
#include <mailtransport/transport.h>
#include <mailtransport/transportmanager.h>

#include <kmime/kmime_message.h>
#include <boost/shared_ptr.hpp>


#define TIMEOUT_SECONDS 60
#define MAXCOUNT 99 // must be 2-digit!

using namespace Akonadi;
using namespace KMime;
using namespace MailTransport;

void DupeTest::initTestCase()
{
  QVERIFY( Control::start() );
  QTest::qWait( 1000 ); // give the MDA time to start

  // we need a default Akonadi transport
  int tid = TransportManager::self()->defaultTransportId();
  Transport *t = TransportManager::self()->transportById( tid );
  QVERIFY( t );
  QCOMPARE( t->type(), int( Transport::EnumType::Akonadi ) );

  // set the sink collection
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

  // set up monitor
  monitor = new Monitor( this );
  monitor->setCollectionMonitored( sink );
  monitor->itemFetchScope().fetchFullPayload();
}

void DupeTest::testDupes_data()
{
  QTest::addColumn<QString>( "message" ); // the prefix of the message to send (-msg## is appended)
  QTest::addColumn<int>( "count" ); // how many copies to send
  QTest::addColumn<int>( "delay" ); // number of ms to wait before sending next copy

  QTest::newRow( "1-nodelay" ) << "\n1-nodelay" << 1 << 0;
  QTest::newRow( "2-nodelay" ) << "\n2-nodelay" << 2 << 0;
  QTest::newRow( "5-nodelay" ) << "\n5-nodelay" << 5 << 0;
  QTest::newRow( "10-nodelay" ) << "\n10-nodelay" << 10 << 0;
  QTest::newRow( "20-nodelay" ) << "\n20-nodelay" << 20 << 0;
  QTest::newRow( "50-nodelay" ) << "\n50-nodelay" << 50 << 0;
  QTest::newRow( "99-nodelay" ) << "\n99-nodelay" << 99 << 0;
  QTest::newRow( "2-veryshortdelay" ) << "\n2-veryshortdelay" << 2 << 20;
  QTest::newRow( "5-veryshortdelay" ) << "\n5-veryshortdelay" << 5 << 20;
  QTest::newRow( "10-veryshortdelay" ) << "\n10-veryshortdelay" << 10 << 20;
  QTest::newRow( "20-veryshortdelay" ) << "\n20-veryshortdelay" << 20 << 20;
  QTest::newRow( "50-veryshortdelay" ) << "\n50-veryshortdelay" << 50 << 20;
  QTest::newRow( "99-veryshortdelay" ) << "\n99-veryshortdelay" << 99 << 20;
  QTest::newRow( "2-shortdelay" ) << "\n2-shortdelay" << 2 << 100;
  QTest::newRow( "5-shortdelay" ) << "\n5-shortdelay" << 5 << 100;
  QTest::newRow( "10-shortdelay" ) << "\n10-shortdelay" << 10 << 100;
  QTest::newRow( "20-shortdelay" ) << "\n20-shortdelay" << 20 << 100;
  QTest::newRow( "50-shortdelay" ) << "\n50-shortdelay" << 50 << 100;
  QTest::newRow( "99-shortdelay" ) << "\n99-shortdelay" << 99 << 99;
  QTest::newRow( "2-longdelay" ) << "\n2-longdelay" << 2 << 1000;
  QTest::newRow( "5-longdelay" ) << "\n5-longdelay" << 5 << 1000;
  QTest::newRow( "5-verylongdelay" ) << "\n5-verylongdelay" << 5 << 4000;
  Q_ASSERT( 99 <= MAXCOUNT );
  Q_ASSERT( MAXCOUNT < 100 ); // 2-digit

  // TODO I'm not sure more items means a better test
  // TODO test large items
  // TODO test modifying items while they are being sent...
}

void DupeTest::testDupes()
{
  QFETCH( QString, message );
  QFETCH( int, count );
  QFETCH( int, delay );

  // clean sink
  ItemFetchJob *fjob = new ItemFetchJob( sink, this );
  AKVERIFYEXEC( fjob );
  if( fjob->items().count() > 0 ) {
    // this test is needed because ItemDeleteJob gives error if no items are found
    ItemDeleteJob *djob = new ItemDeleteJob( sink, this );
    AKVERIFYEXEC( djob );
  }
  fjob = new ItemFetchJob( sink, this );
  AKVERIFYEXEC( fjob );
  QCOMPARE( fjob->items().count(), 0 );

  // queue messages
  Q_ASSERT( monitor );
  QSignalSpy *addSpy = new QSignalSpy( monitor, SIGNAL( itemAdded( Akonadi::Item, Akonadi::Collection ) ) );
  kDebug() << "Queuing" << count << "messages...";
  for( int i = 0; i < count; i++ ) {
    //kDebug() << "Queuing message" << i + 1 << "of" << count;

    Message::Ptr msg = Message::Ptr( new Message );
    msg->setContent( QString( "%1-msg%2\n" ).arg( message ).arg( i + 1, 2, 10, QLatin1Char( '0' ) ).toLatin1() );

    MessageQueueJob *job = new MessageQueueJob( this );
    job->setMessage( msg );
    job->setTransportId( TransportManager::self()->defaultTransportId() );
    // default dispatch mode
    // default sent-mail collection
    job->setFrom( "naiba" );
    job->setTo( QStringList( "dracu" ) );
    //AKVERIFYEXEC( job );
    job->start();
    QTest::qWait( delay );
  }
  kDebug() << "Queued" << count << "messages.";

  // wait for the MDA to send them
  int seconds = 0;
  while( true ) {
    seconds++;
    QTest::qWait( 1000 );
    kDebug() << seconds << "seconds elapsed." << addSpy->count() << "messages got to sink.";
    if( addSpy->count() >= count )
      break;

#if 0
    if( seconds >= TIMEOUT_SECONDS ) {
      kDebug() << "Timeout, gdb master!";
      QTest::qWait( 1000*1000 );
    }
#endif
    QVERIFY2( seconds < TIMEOUT_SECONDS, "Timeout" );
  }

  // TODO I should verify that the MDA has actually finished its work and has an empty queue
  QTest::qWait( 2000 );

  // verify what has been sent
  fjob = new ItemFetchJob( sink, this );
  fjob->fetchScope().fetchFullPayload();
  AKVERIFYEXEC( fjob );
  const Item::List items = fjob->items();
  int found[ MAXCOUNT ];
  for( int i = 0; i < count; i++ ) {
    found[i] = 0;
  }
  for( int i = 0; i < items.count(); i++ ) {
    QVERIFY( items[i].hasPayload<Message::Ptr>() );
    Message::Ptr msg = items[i].payload<Message::Ptr>();
    const QByteArray content = msg->encodedContent();
    //kDebug() << "i" << i << "content" << content;
    int loc = content.indexOf( "-msg" );
    QVERIFY( loc >= 0 );
    bool ok;
    int who = content.mid( loc + 4, 2 ).toInt( &ok );
    QVERIFY( ok );
    //kDebug() << "identified msg" << who;
    QVERIFY( who > 0 && who <= count );
    found[ who - 1 ]++;
  }
  for( int i = 0; i < count; i++ ) {
    if( found[i] > 1 ) {
      kDebug() << "found duplicate message" << i + 1 << "(" << found[i] << "times )";
    } else if( found[i] < 1 ) {
      kDebug() << "didn't find message" << i + 1;
    }
    QCOMPARE( found[i], 1 );
  }
  QCOMPARE( addSpy->count(), count );
  QCOMPARE( items.count(), count );
}

QTEST_AKONADIMAIN( DupeTest, NoGUI )

#include "dupetest.moc"
