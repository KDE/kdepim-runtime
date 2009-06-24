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

#include "storeresultjob.h"

#include <QTimer>

#include <KDebug>
#include <KLocalizedString>

#include <Akonadi/Collection>
#include <Akonadi/Item>
#include <Akonadi/ItemModifyJob>
#include <Akonadi/ItemMoveJob>

#include <mailtransport/transport.h>
#include <mailtransport/transportjob.h>
#include <mailtransport/akonadijob.h> // TODO really stinks that it needs special treatment
#include <mailtransport/transportmanager.h>

#include <akonadi/kmime/messageparts.h>
#include <kmime/kmime_message.h>
#include <boost/shared_ptr.hpp>

#include <outboxinterface/addressattribute.h>
#include <outboxinterface/sentcollectionattribute.h>
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
      , currentJob( 0 )
      , aborting( false )
    {
    }

    SendJob *const q;
    Item item;
    KJob *currentJob;
    bool aborting;

    void storeResult( bool success, const QString &message = QString() );

    // slots:
    void doTransport();
    void transportResult( KJob *job );
    void moveResult( KJob *job );
    void doEmitResult( KJob *job ); // result of storeResult transaction

};


void SendJob::Private::doTransport()
{
  kDebug() << "Transporting message.";

  if( aborting ) {
    kDebug() << "Marking message as aborted.";
    storeResult( false, i18n( "Message sending aborted." ) );
    return;
  }

  // get transport and addresses from attributes
  TransportAttribute *tA = item.attribute<TransportAttribute>();
  Q_ASSERT( tA );
  TransportJob *tjob = TransportManager::self()->createTransportJob( tA->transportId() );
  if( !tjob ) {
    storeResult( false, i18n( "Could not initiate message transport. Possibly invalid transport." ) );
    return;
  }
  Q_ASSERT( currentJob == 0 );
  currentJob = tjob;

  AkonadiJob *ajob = dynamic_cast<AkonadiJob*>( currentJob );
  if( ajob ) {
    // It's an Akonadi job.
    ajob->setItemId( item.id() );
    // FIXME this keeps all the attributes and flags of the MDA...
  } else {
    // It's a SMTP or Sendmail job.
    // get message from payload
    Q_ASSERT( item.hasPayload<Message::Ptr>() );
    const Message::Ptr message = item.payload<Message::Ptr>();
    const QByteArray cmsg = message->encodedContent( true ) + "\r\n";
    //kDebug() << "msg:" << cmsg;
    Q_ASSERT( !cmsg.isEmpty() );

    AddressAttribute *addrA = item.attribute<AddressAttribute>();
    Q_ASSERT( addrA );
    tjob->setData( cmsg );
    tjob->setSender( addrA->from() );
    tjob->setTo( addrA->to() );
    tjob->setCc( addrA->cc() );
    tjob->setBcc( addrA->bcc() );
  }
  connect( currentJob, SIGNAL( result( KJob * ) ), q, SLOT( transportResult( KJob * ) ) );
  currentJob->start(); // non-Akonadi

  // TODO something about timeouts.
}

void SendJob::Private::transportResult( KJob *job )
{
  Q_ASSERT( currentJob == job );
  currentJob = 0;

  if( job->error() ) {
    kDebug() << "Error transporting.";
    q->setError( UserDefinedError );
    QString err;
    if( aborting ) {
      err = i18n( "Message transport aborted." );
    } else {
      err = i18n( "Failed to transport message." );
    }
    q->setErrorText( err + ' ' + job->errorString() );
    storeResult( false, q->errorString() );
  } else {
    kDebug() << "Success transporting.";

#if 0
    // Move to sent-mail
    SentCollectionAttribute *sA = item.attribute<SentCollectionAttribute>();
    Q_ASSERT( sA );
    kDebug() << "Moving to sent-mail collection with id" << sA->sentCollection();
    Collection sentMail( sA->sentCollection() );
    if( !sentMail.isValid() ) {
      q->setError( UserDefinedError );
      q->setErrorText( i18n( "Invalid sent-mail folder. Keeping message in outbox." ) );
      storeResult( false, q->errorString() );
    } else {
      Q_ASSERT( currentJob == 0 );
      currentJob = new ItemMoveJob( item, sentMail );
      QObject::connect( currentJob, SIGNAL( result( KJob* ) ), this, SLOT( moveResult( KJob* ) ) );
    }
#endif
    storeResult( true );
  }
}

void SendJob::Private::moveResult( KJob *job )
{
  Q_ASSERT( false ); // moving to sent-mail disabled

  Q_ASSERT( currentJob == job );
  currentJob = 0;

  if( job->error() ) {
    kDebug() << "Error moving to sent-mail.";
    q->setError( UserDefinedError );
    q->setErrorText( i18n( "Failed to move message to sent-mail." ) + ' ' + job->errorString() );
    storeResult( false, q->errorString() );
  } else {
    kDebug() << "Success moving to sent-mail.";
    storeResult( true );
  }
}

void SendJob::Private::storeResult( bool success, const QString &message )
{
  kDebug() << "success" << success << "message" << message;

  Q_ASSERT( currentJob == 0 );
  currentJob = new StoreResultJob( item, success, message);
  connect( currentJob, SIGNAL( result( KJob * ) ), q, SLOT( doEmitResult( KJob * ) ) );
}

void SendJob::Private::doEmitResult( KJob *job )
{
  Q_ASSERT( currentJob == job );
  currentJob = 0;

  if( job->error() ) {
    kWarning() << "Error storing result.";
    q->setError( UserDefinedError );
    q->setErrorText( q->errorString() + ' ' + i18n( "Failed to store result in item." ) + ' ' + job->errorString() );
  } else {
    kDebug() << "Success storing result.";
    // It is still possible that the transport failed.
  }

  q->emitResult();
}



SendJob::SendJob( const Item &item, QObject *parent )
  : KJob( parent )
  , d( new Private( item, this ) )
{
}

SendJob::~SendJob()
{
  delete d;
}


void SendJob::start()
{
  QTimer::singleShot( 0, this, SLOT( doTransport() ) );
}

void SendJob::setMarkAborted()
{
  Q_ASSERT( !d->aborting );
  d->aborting = true;
}

void SendJob::abort()
{
  setMarkAborted();

  if( !d->currentJob ) {
    kDebug() << "Abort called but no active job.";
    // Nothing to do; doTransport will abort when the time comes.
  } else if( dynamic_cast<TransportJob*>( d->currentJob ) ) {
    kDebug() << "Abort called, active transport job.";
    // Abort transport.
    d->currentJob->kill( KJob::EmitResult );
    // TODO verify that a proper error (aborted) is set when a TransportJob is killed
  } else if( dynamic_cast<ItemMoveJob*>( d->currentJob ) ) {
    kDebug() << "Abort called, active itemMove job.";
    // Item was already sent, so let it finish in peace.
  } else if( dynamic_cast<StoreResultJob*>( d->currentJob ) ) {
    kDebug() << "Abort called, active storeResult job.";
    // Item was already sent, so let it finish in peace.
  } else {
    Q_ASSERT( false );
  }
}


#include "sendjob.moc"
