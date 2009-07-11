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

#include <QDBusInterface>
#include <QDBusReply>
#include <QTimer>

#include <KDebug>
#include <KLocalizedString>

#include <Akonadi/Collection>
#include <Akonadi/Item>
#include <Akonadi/ItemDeleteJob>
#include <Akonadi/ItemModifyJob>
#include <Akonadi/ItemMoveJob>

#include <kmime/kmime_message.h>
#include <boost/shared_ptr.hpp>

#include <mailtransport/sentbehaviourattribute.h>
#include <mailtransport/transport.h>
#include <mailtransport/transportattribute.h>
#include <mailtransport/transportjob.h>
#include <mailtransport/transportmanager.h>

#include <akonadi/agentinstance.h>
#include <akonadi/agentmanager.h>
#include <akonadi/kmime/addressattribute.h>
#include <akonadi/kmime/messageparts.h>

using namespace Akonadi;
using namespace KMime;
using namespace MailTransport;

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
      , iface( 0 )
      , aborting( false )
    {
    }

    SendJob *const q;
    Item item;
    KJob *currentJob;
    QString resourceId;
    QDBusInterface *iface;
    bool aborting;

    void doTransport(); // slot
    void doAkonadiTransport();
    void doTraditionalTransport();
    void transportPercent( KJob *job, unsigned long percent ); // slot
    void transportResult( KJob *job ); // slot
    void resourceProgress( const AgentInstance &instance ); // slot
    void resourceResult( qlonglong itemId, bool success, const QString &message ); // slot
    void doPostJob( bool transportSuccess, const QString &transportMessage ); // result of transport
    void postJobResult( KJob *job ); // slot
    void storeResult( bool success, const QString &message = QString() );
    void doEmitResult( KJob *job ); // slot: result of storeResult transaction

};


void SendJob::Private::doTransport()
{
  kDebug() << "Transporting message.";

  if( aborting ) {
    kDebug() << "Marking message as aborted.";
    q->setError( UserDefinedError );
    q->setErrorText( i18n( "Message sending aborted." ) );
    storeResult( false, i18n( "Message sending aborted." ) );
    return;
  }

  // Is it an Akonadi transport or a traditional one?
  TransportAttribute *tA = item.attribute<TransportAttribute>();
  Q_ASSERT( tA );
  if( !tA->transport() ) {
    storeResult( false, i18n( "Could not initiate message transport. Possibly invalid transport." ) );
    return;
  }
  TransportType type = tA->transport()->transportType();
  Q_ASSERT( type.isValid() );
  if( type.type() == Transport::EnumType::Akonadi ) {
    // Send the item directly to the resource that will send it.
    resourceId = tA->transport()->host();
    doAkonadiTransport();
  } else {
    // Use a traditional transport job.
    doTraditionalTransport();
  }
}

void SendJob::Private::doAkonadiTransport()
{
  Q_ASSERT( !resourceId.isEmpty() );
  Q_ASSERT( iface == 0 );
  iface = new QDBusInterface(
      QLatin1String( "org.freedesktop.Akonadi.Resource." ) + resourceId,
      QLatin1String( "/Transport" ), QLatin1String( "org.freedesktop.Akonadi.Resource.Transport" ),
      QDBusConnection::sessionBus(), q );
  if( !iface->isValid() ) {
    storeResult( false, i18n( "Failed to get D-Bus interface of resource %1.", resourceId ) );
    return;
  }

  // Signals.
  QObject::connect( AgentManager::self(), SIGNAL(instanceProgressChanged(Akonadi::AgentInstance)),
      q, SLOT(resourceProgress(Akonadi::AgentInstance)) );
  QObject::connect( iface, SIGNAL(transportResult(qlonglong,bool,QString)),
      q, SLOT(resourceResult(qlonglong,bool,QString)) );

  // Start sending.
  QDBusReply<void> reply = iface->call( QLatin1String( "send" ), item.id() );
  if( !reply.isValid() ) {
    storeResult( false, i18n( "Invalid D-Bus reply from resource %1.", resourceId ) );
    return;
  }
}

void SendJob::Private::doTraditionalTransport()
{
  TransportAttribute *tA = item.attribute<TransportAttribute>();
  TransportJob *tjob = TransportManager::self()->createTransportJob( tA->transportId() );
  Q_ASSERT( tjob );
  Q_ASSERT( currentJob == 0 );
  currentJob = tjob;

  // Message.
  Q_ASSERT( item.hasPayload<Message::Ptr>() );
  const Message::Ptr message = item.payload<Message::Ptr>();
  const QByteArray cmsg = message->encodedContent( true ) + "\r\n";
  //kDebug() << "msg:" << cmsg;
  Q_ASSERT( !cmsg.isEmpty() );

  // Addresses.
  AddressAttribute *addrA = item.attribute<AddressAttribute>();
  Q_ASSERT( addrA );
  tjob->setData( cmsg );
  tjob->setSender( addrA->from() );
  tjob->setTo( addrA->to() );
  tjob->setCc( addrA->cc() );
  tjob->setBcc( addrA->bcc() );

  // Signals.
  connect( tjob, SIGNAL(result(KJob*)), q, SLOT(transportResult(KJob*)) );
  connect( tjob, SIGNAL(percent(KJob*,unsigned long)),
      q, SLOT(transportPercent(KJob*,unsigned long)) );
  tjob->start(); // non-Akonadi
}

void SendJob::Private::transportPercent( KJob *job, unsigned long percent )
{
  Q_UNUSED( percent );
  Q_ASSERT( currentJob == job );
  kDebug() << "Processed amount" << job->processedAmount( KJob::Bytes )
    << "total amount" << job->totalAmount( KJob::Bytes );
  q->setTotalAmount( KJob::Bytes, job->totalAmount( KJob::Bytes ) ); // Is not set at the time of start().
  q->setProcessedAmount( KJob::Bytes, job->processedAmount( KJob::Bytes ) );
}

void SendJob::Private::transportResult( KJob *job )
{
  Q_ASSERT( currentJob == job );
  currentJob = 0;
  doPostJob( !job->error(), job->errorString() );
}

void SendJob::Private::resourceProgress( const AgentInstance &instance )
{
  if( !iface ) {
    // We might have gotten a very late signal.
    kWarning() << "called but no resource job running!";
    return;
  }

  if( instance.identifier() == resourceId ) {
    // This relies on the resource's progress representing the progress of
    // sending this item.
    q->setPercent( instance.progress() );
  }
}

void SendJob::Private::resourceResult( qlonglong itemId, bool success, const QString &message )
{
  Q_ASSERT( iface );
  delete iface; // So that abort() knows the transport job is over.
  iface = 0;
  Q_ASSERT( itemId == item.id() );
  doPostJob( success, message );
}

void SendJob::Private::doPostJob( bool transportSuccess, const QString &transportMessage )
{
  kDebug() << "success" << transportSuccess << "message" << transportMessage;

  if( !transportSuccess ) {
    kDebug() << "Error transporting.";
    q->setError( UserDefinedError );
    QString err;
    if( aborting ) {
      err = i18n( "Message transport aborted." );
    } else {
      err = i18n( "Failed to transport message." );
    }
    q->setErrorText( err + ' ' + transportMessage );
    storeResult( false, q->errorString() );
  } else {
    kDebug() << "Success transporting.";

    // Delete or move to sent-mail.
    SentBehaviourAttribute *sA = item.attribute<SentBehaviourAttribute>();
    Q_ASSERT( sA );
    if( sA->sentBehaviour() == SentBehaviourAttribute::Delete ) {
      kDebug() << "Deleting item from outbox.";
      currentJob = new ItemDeleteJob( item );
      QObject::connect( currentJob, SIGNAL(result(KJob*)), q, SLOT(postJobResult(KJob*)) );
    } else {
#if 0 // intra-resource moves do not work
      Collection moveTo( sA->moveToCollection() );
      if( sA->sentBehaviour() == SentBehaviourAttribute::MoveToDefaultSentCollection ) {
        if( !LocalFolders::self()->isReady() ) {
          // We were unlucky and LocalFolders is recreating its stuff right now.
          // We will not wait for it.
          moveTo = Collection();
        } else {
          moveTo = LocalFolders::self()->sentMail();
        }
      }
      kDebug() << "Moving to sent-mail collection with id" << moveTo.id();
      if( !moveTo.isValid() ) {
        q->setError( UserDefinedError );
        q->setErrorText( i18n( "Invalid sent-mail folder. Keeping message in outbox." ) );
        storeResult( false, q->errorString() );
      } else {
        Q_ASSERT( currentJob == 0 );
        currentJob = new ItemMoveJob( item, sentMail );
        QObject::connect( currentJob, SIGNAL(result(KJob*)), q, SLOT(postJobResult(KJob*)) );
      }
#endif
      storeResult( true );
    }
  }
}

void SendJob::Private::postJobResult( KJob *job )
{
  Q_ASSERT( currentJob == job );
  currentJob = 0;

  if( job->error() ) {
    kDebug() << "Error deleting or moving to sent-mail.";
    q->setError( UserDefinedError );
    q->setErrorText( i18n( "Sending succeeded, but failed to finalize message." ) + ' ' + job->errorString() );
    storeResult( false, q->errorString() );
  } else {
    kDebug() << "Success deleting or moving to sent-mail.";
    storeResult( true );
  }
}

void SendJob::Private::storeResult( bool success, const QString &message )
{
  kDebug() << "success" << success << "message" << message;

  Q_ASSERT( currentJob == 0 );
  currentJob = new StoreResultJob( item, success, message);
  connect( currentJob, SIGNAL(result(KJob*)), q, SLOT(doEmitResult(KJob*)) );
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

  if( dynamic_cast<TransportJob*>( d->currentJob ) ) {
    kDebug() << "Abort called, active transport job.";
    // Abort transport.
    d->currentJob->kill( KJob::EmitResult );
  } else if( d->iface != 0 ) {
    kDebug() << "Abort called, propagating to resource.";
    // Abort resource doing transport.
    AgentInstance instance = AgentManager::self()->instance( d->resourceId );
    instance.abort();
  } else {
    kDebug() << "Abort called, but no transport job is active.";
    // Either transport has not started, in which case doTransport will
    // mark the item as aborted, or the item has already been sent, in which
    // case there is nothing we can do.
  }
}

#include "sendjob.moc"
