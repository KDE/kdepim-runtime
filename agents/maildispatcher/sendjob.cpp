/*
    Copyright (c) 2009 Constantin Berzan <exit3219@gmail.com>

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

#include "sendjob.h"

#include <QTimer>

#include <KDebug>
#include <KLocalizedString>

#include <Akonadi/Collection>
#include <Akonadi/Item>
#include <Akonadi/ItemModifyJob>
#include <Akonadi/ItemMoveJob>

#include <mailtransport/transport.h>
#include <mailtransport/transportjob.h>
#include <mailtransport/transportmanager.h>

#include <akonadi/kmime/messageparts.h>
#include <kmime/kmime_message.h>
#include <boost/shared_ptr.hpp>

#include <outboxinterface/localfolders.h>
#include <outboxinterface/addressattribute.h>
#include <outboxinterface/dispatchmodeattribute.h>
#include <outboxinterface/statusattribute.h>
#include <outboxinterface/transportattribute.h>

using namespace Akonadi;
using namespace KMime;
using namespace MailTransport;
using namespace OutboxInterface;


/**
 * Private class that helps to provide binary compatibility between releases.
 * @internal
 */
class SendJob::Private
{
  public:
    Private( const Item &itm, SendJob *qq )
      : q( qq )
      , item( itm )
    {
      currentStep = NotStarted;
      errorStored = false;
    }

    enum Step
    {
      NotStarted,
      AddActiveFlag,
      DoTransport,
      AddSentFlag,
      MoveToSentMail,
      Finished
    } currentStep;

    SendJob *const q;
    Item item;
    bool errorStored;

    // slot
    void doNextStep();

    void doStoreError();

};


void SendJob::Private::doNextStep()
{
  currentStep = Step( int( currentStep ) + 1 ); // ugly HACK
  kDebug() << "current step" << currentStep;

  if( currentStep == AddActiveFlag ) {
    // TODO: figure out if adding flags is desirable at all, and how much of a
    // performance problem they are.

    // 1. add "active" flag
    kDebug() << "Step 1: adding 'active' flag.";
    item.clearFlag( "queued" );
    item.setFlag( "active" );
    ItemModifyJob *job = new ItemModifyJob( item );
    job->setIgnorePayload( true );
    q->addSubjob( job );
  } else if( currentStep == DoTransport ) {
    // 2. send message
    kDebug() << "Step 2: transporting message.";
    // The model was supposed to make sure this is a valid item,
    // and that it is time for it to be sent.
    Q_ASSERT( item.hasPayload<Message::Ptr>() );

    // get message from payload
    const Message::Ptr message = item.payload<Message::Ptr>();
    const QByteArray cmsg = message->encodedContent( true ) + "\r\n";
    kDebug() << "msg:" << cmsg;
    Q_ASSERT( !cmsg.isEmpty() );

    // get transport from attribute
    TransportAttribute *tA = item.attribute<TransportAttribute>();
    Q_ASSERT( tA );
    Transport *transport = tA->transport();
    // TODO: the transport may have been deleted between the time this message
    // was put into the outbox (i.e. checked by outboxinterface/messagequeuejob)
    // and the time it is actually sent.  Replace this assert with a proper
    // check.
    Q_ASSERT( transport != 0 );
    kDebug() << "Sending to server:" << transport->host() << transport->port();

    // get addresses from attribute
    AddressAttribute *addrA = item.attribute<AddressAttribute>();
    Q_ASSERT( addrA );
    kDebug() << "from" << addrA->from() << "to" << addrA->to()
             << "cc" << addrA->cc() << "bcc" << addrA->bcc();
    TransportJob *job = TransportManager::self()->createTransportJob( tA->transportId() );
    if( !job ) {
      // TODO: check that this doesn't interfere with the fact that q already has subjobs
      q->setError( UserDefinedError );
      q->setErrorText( i18n( "Could not initiate message transport. Possibly invalid transport." ) );
      doStoreError();
      return;
    }
    job->setData( cmsg );
    job->setSender( addrA->from() );
    job->setTo( addrA->to() );
    job->setCc( addrA->cc() );
    job->setBcc( addrA->bcc() );
    q->addSubjob( job );
    job->start(); // non-Akonadi; needs to be started manually
  } else if( currentStep == AddSentFlag ) {
    // 3. set "sent" flag
    kDebug() << "Step 3: adding 'sent' flag.";
    // TODO: test that this actually works the way I intend it to.
    item.clearFlag( "active" );
    item.setFlag( "sent" );
    ItemModifyJob *job = new ItemModifyJob( item );
    job->setIgnorePayload( true );
    q->addSubjob( job );
  } else if( currentStep == MoveToSentMail ) {
    // 4. move to sent-mail
    kDebug() << "Step 4: moving to sent-mail.";
    // TODO: use attribute for sent-mail collection
    Collection sentMail = LocalFolders::self()->sentMail();
    if( !sentMail.isValid() ) {
      // TODO: check that this doesn't interfere with the fact that q already has subjobs
      q->setError( UserDefinedError );
      q->setErrorText( i18n( "Invalid sent-mail collection. Keeping message in outbox." ) );
      doStoreError();
      return;
    }
    ItemMoveJob *job = new ItemMoveJob( item, sentMail );
    q->addSubjob( job );
  } else if( currentStep == Finished ) {
    Q_ASSERT( !q->error() );
    kDebug() << "All subjobs done. Item sent ok. Emitting result.";
    q->emitResult();
  } else {
    Q_ASSERT( false );
  }
}

void SendJob::Private::doStoreError()
{
  // I can only be called once.
  Q_ASSERT( !errorStored );
  errorStored = true;

  Q_ASSERT( q->error() );
  StatusAttribute *sA = new StatusAttribute( StatusAttribute::Failed,
      q->errorString() );
  item.addAttribute( sA ); // replaces old one
  ItemModifyJob *job = new ItemModifyJob( item );
  q->addSubjob( job );
}

SendJob::SendJob( const Item &item, QObject *parent )
  : KCompositeJob( parent )
  , d( new Private( item, this ) )
{
}

SendJob::~SendJob()
{
  delete d;
}


void SendJob::start()
{
  QTimer::singleShot( 0, this, SLOT( doNextStep() ) );
}

void SendJob::slotResult( KJob *job )
{
  // TODO: there is probably a better way to do this.  Maybe the switch( currentStep )
  // should happen here???
  ItemModifyJob *modifyJob = dynamic_cast<ItemModifyJob*>( job );
  if( modifyJob ) {
    // If it was an ItemModifyJob, we need to save the modified item.
    // Otherwise we get the 'Item was modified elsewhere' error.
    d->item = modifyJob->item();
  }

  // HACK: This is based on KCompositeJob::slotResult.
  // We can't call that directly because we need to doStoreError() before
  // doing emitResult().

  if( job->error() ) {
    if( error() ) {
      // Error from the job storing the error.  Bloody hell.
      kDebug() << "Error trying to store error. Emitting result.";
      emitResult();
    } else {
      // First error from a regular subjob.  Store it.
      kDebug() << "Error. Trying to store result in item.";
      setError( job->error() );
      setErrorText( job->errorText() );
      d->doStoreError();
    }
  } else if( error() ) {
    // Previous error; success from the job storing the error.
    kDebug() << "Error stored. Emitting result.";
    emitResult();
  } else {
    // No error.  Do next step.
    kDebug() << "No error so far; doing next step.";
    d->doNextStep();
  }

  removeSubjob( job );
}


#include "sendjob.moc"
