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
    {
    }

    SendJob *const q;
    Item item;

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

  // get message from payload
  Q_ASSERT( item.hasPayload<Message::Ptr>() );
  const Message::Ptr message = item.payload<Message::Ptr>();
  const QByteArray cmsg = message->encodedContent( true ) + "\r\n";
  kDebug() << "msg:" << cmsg;
  Q_ASSERT( !cmsg.isEmpty() );

  // get transport and addresses from attributes
  TransportAttribute *tA = item.attribute<TransportAttribute>();
  Q_ASSERT( tA );
  AddressAttribute *addrA = item.attribute<AddressAttribute>();
  Q_ASSERT( addrA );
  TransportJob *job = TransportManager::self()->createTransportJob( tA->transportId() );
  if( !job ) {
    storeResult( false, i18n( "Could not initiate message transport. Possibly invalid transport." ) );
    return;
  }
  job->setData( cmsg );
  job->setSender( addrA->from() );
  job->setTo( addrA->to() );
  job->setCc( addrA->cc() );
  job->setBcc( addrA->bcc() );
  connect( job, SIGNAL( result( KJob * ) ), q, SLOT( transportResult( KJob * ) ) );
  job->start(); // non-Akonadi
}

void SendJob::Private::transportResult( KJob *job )
{
  if( job->error() ) {
    kDebug() << "Error transporting.";
    q->setError( UserDefinedError );
    q->setErrorText( i18n( "Failed to transport message." ) + ' ' + job->errorString() );
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
      ItemMoveJob *job = new ItemMoveJob( item, sentMail );
      QObject::connect( job, SIGNAL( result( KJob* ) ), this, SLOT( moveResult( KJob* ) ) );
    }
#endif
    storeResult( true );
  }
}

void SendJob::Private::moveResult( KJob *job )
{
  Q_ASSERT( false ); // moving to sent-mail disabled

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

  StoreResultJob *job = new StoreResultJob( item, success, message);
  connect( job, SIGNAL( result( KJob * ) ), q, SLOT( doEmitResult( KJob * ) ) );
}

void SendJob::Private::doEmitResult( KJob *job )
{
  if( job->error() ) {
    // This is bad.  Unless the item disappeared, the MDA will probably
    // enter an infinite loop of trying to send the same item over and over
    // again.  TODO what?

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


#include "sendjob.moc"
