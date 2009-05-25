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

#include "messagequeuejobtest.h"

#include <QStringList>

#include <Akonadi/AgentInstance>
#include <Akonadi/AgentManager>
#include <Akonadi/Collection>
#include <Akonadi/Control>
#include <akonadi/qtest_akonadi.h>

#include <mailtransport/transportmanager.h>
#include <mailtransport/transport.h>

#include <kmime/kmime_message.h>
#include <boost/shared_ptr.hpp>

#include <outboxinterface/messagequeuejob.h>

#define SPAM_ADDRESS ( QStringList() << "idanoka@gmail.com" )

using namespace Akonadi;
using namespace KMime;
using namespace MailTransport;
using namespace OutboxInterface;


void MessageQueueJobTest::initTestCase()
{
  Control::start();
  QTest::qWait( 1000 );
  // HACK:
  // Akonadi will start the MDA, and that will attempt to create the Local Folders
  // resource and collections.  The MessageQueueJob will try to do the same,
  // failing because the DB cannot have collections with the same name
  // TODO: come up with a real locking mechanism

  // switch all resources offline to avoid interference and spam
  foreach( AgentInstance agent, AgentManager::self()->instances() ) {
    agent.setIsOnline( false );
  }
}

void MessageQueueJobTest::testAddressesFromMime()
{
  // TODO
}

void MessageQueueJobTest::testValidMessages()
{
  // check transport
  int tid = TransportManager::self()->defaultTransportId();
  QVERIFY2( tid >= 0, "I need a default transport, but there is none." );

  // send a valid message using the default transport
  MessageQueueJob *job = new MessageQueueJob;
  job->setTransportId( tid );
  Message::Ptr msg = Message::Ptr( new Message );
  msg->setContent( "This is message #1 sent from the MessageQueueJobTest unittest.\n" );
  job->setMessage( msg );
  job->setTo( SPAM_ADDRESS );
  AKVERIFYEXEC( job );

  // TODO test with no To: but only BCC:

  // TODO test due-date sending

  // TODO test sending with custom sent-mail collections
}

void MessageQueueJobTest::testInvalidMessages()
{
  MessageQueueJob *job = 0;
  Message::Ptr msg;

  // without message
  job = new MessageQueueJob;
  job->setTransportId( TransportManager::self()->defaultTransportId() );
  job->setTo( SPAM_ADDRESS );
  QVERIFY( !job->exec() );

  // without recipients
  job = new MessageQueueJob;
  msg = Message::Ptr( new Message );
  msg->setContent( "This is a message sent from the MessageQueueJobTest unittest. This shouldn't have been sent.\n" );
  job->setMessage( msg );
  job->setTransportId( TransportManager::self()->defaultTransportId() );
  QVERIFY( !job->exec() );

  // without transport
  job = new MessageQueueJob;
  msg = Message::Ptr( new Message );
  msg->setContent( "This is a message sent from the MessageQueueJobTest unittest. This shouldn't have been sent.\n" );
  job->setMessage( msg );
  job->setTo( SPAM_ADDRESS );
  QVERIFY( !job->exec() );

  // with AfterDueDate and no due date
  job = new MessageQueueJob;
  msg = Message::Ptr( new Message );
  msg->setContent( "This is a message sent from the MessageQueueJobTest unittest. This shouldn't have been sent.\n" );
  job->setMessage( msg );
  job->setTo( SPAM_ADDRESS );
  job->setDispatchMode( DispatchModeAttribute::AfterDueDate );
  QVERIFY( !job->exec() );

  // with invalid sent-mail folder
  job = new MessageQueueJob;
  msg = Message::Ptr( new Message );
  msg->setContent( "This is a message sent from the MessageQueueJobTest unittest. This shouldn't have been sent.\n" );
  job->setMessage( msg );
  job->setTo( SPAM_ADDRESS );
  job->setSentMailCollection( -1 );
  QVERIFY( !job->exec() );
}


QTEST_AKONADIMAIN( MessageQueueJobTest, NoGUI )

#include "messagequeuejobtest.moc"
