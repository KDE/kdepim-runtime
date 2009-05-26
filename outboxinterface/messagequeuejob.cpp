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

#include "messagequeuejob.h"

#include "localfolders.h"
#include "addressattribute.h"
#include "dispatchmodeattribute.h"
#include "errorattribute.h"
#include "sentcollectionattribute.h"
#include "transportattribute.h"

#include <QTimer>

#include <KDebug>
#include <KLocalizedString>

#include <Akonadi/AttributeFactory>
#include <Akonadi/Collection>
#include <Akonadi/Item>
#include <Akonadi/ItemCreateJob>

#include <mailtransport/transport.h>
#include <mailtransport/transportmanager.h>


using namespace Akonadi;
using namespace KMime;
using namespace MailTransport;
using namespace OutboxInterface;


/**
 * Private class that helps to provide binary compatibility between releases.
 * @internal
 */
class OutboxInterface::MessageQueueJob::Private
{
  public:
    Private( MessageQueueJob *qq )
      : q( qq )
    {
      transport = -1;
      mode = DispatchModeAttribute::Immediately;
      useDefaultSentMail = true;
      sentMail = -1;
      started = false;
    }

    MessageQueueJob *const q;

    Message::Ptr message;
    int transport;
    DispatchModeAttribute::DispatchMode mode;
    QDateTime dueDate;
    bool useDefaultSentMail;
    Collection::Id sentMail;
    QString from;
    QStringList to;
    QStringList cc;
    QStringList bcc;
    bool started;


    void readAddressesFromMime();

    /**
      Checks that this message has everything it needs and is ready to be sent.
    */
    bool validate();

    // slot
    void doStart();

};


void MessageQueueJob::Private::readAddressesFromMime()
{
  kDebug() << "implement me";
  // big TODO
}

bool MessageQueueJob::Private::validate()
{
  if( !message ) {
    q->setError( UserDefinedError );
    q->setErrorText( i18n( "Empty message." ) );
    q->emitResult();
    return false;

    // NOTE: the MDA also asserts that msg->encodedContent(true) is non-empty.
  }

  if( to.count() + cc.count() + bcc.count() == 0 ) {
    q->setError( UserDefinedError );
    q->setErrorText( i18n( "Message has no recipients." ) );
    q->emitResult();
    return false;
  }

  if( mode == DispatchModeAttribute::AfterDueDate && !dueDate.isValid() ) {
    q->setError( UserDefinedError );
    q->setErrorText( i18n( "Message has invalid due date." ) );
    q->emitResult();
    return false;
  }

  if( TransportManager::self()->transportById( transport, false ) == 0 ) {
    q->setError( UserDefinedError );
    q->setErrorText( i18n( "Message has invalid transport." ) );
    q->emitResult();
    return false;
  }

  if( useDefaultSentMail ) {
    Q_ASSERT( LocalFolders::self()->isReady() );
    sentMail = LocalFolders::self()->sentMail().id();
    // Can't do this in the constructor because LocalFolders is not ready.

    // TODO: add support for DefaultSentMailCollection and DeleteAfterSending
    // in SentCollectionAttribute
  }

  if( sentMail < 0 ) {
    q->setError( UserDefinedError );
    q->setErrorText( i18n( "Message has invalid sent-mail folder." ) );
    q->emitResult();
    return false;
  }

  return true; // all ok
}

void MessageQueueJob::Private::doStart()
{
  if( !validate() ) {
    // The error has been set; the result has been emitted.
    return;
  }

  Q_ASSERT( !started );
  started = true;

  // create item
  Item item;
  item.setMimeType( "message/rfc822" );
  item.setPayload<Message::Ptr>( message );
  kDebug() << "message:" << message->encodedContent( true );

  // set attributes
  AddressAttribute *addrA = new AddressAttribute( from, to, cc, bcc );
  DispatchModeAttribute *dmA = new DispatchModeAttribute( mode );
  SentCollectionAttribute *sA = new SentCollectionAttribute( sentMail );
  TransportAttribute *tA = new TransportAttribute( transport );
  item.addAttribute( addrA );
  item.addAttribute( dmA );
  item.addAttribute( sA );
  item.addAttribute( tA );

  // set flags
  item.setFlag( "queued" );

  // put item in Akonadi storage
  Q_ASSERT( LocalFolders::self()->isReady() );
  Collection col = LocalFolders::self()->outbox();
  ItemCreateJob *job = new ItemCreateJob( item, col ); // job autostarts
  q->addSubjob( job );
}



MessageQueueJob::MessageQueueJob( QObject *parent )
  : KCompositeJob( parent )
  , d( new Private( this ) )
{
  // register attributes
  AttributeFactory::registerAttribute<AddressAttribute>();
  AttributeFactory::registerAttribute<DispatchModeAttribute>();
  AttributeFactory::registerAttribute<ErrorAttribute>();
  AttributeFactory::registerAttribute<SentCollectionAttribute>();
  AttributeFactory::registerAttribute<TransportAttribute>();
}

MessageQueueJob::~MessageQueueJob()
{
  delete d;
}


Message::Ptr MessageQueueJob::message() const
{
  return d->message;
}

int MessageQueueJob::transportId() const
{
  return d->transport;
}

DispatchModeAttribute::DispatchMode MessageQueueJob::dispatchMode() const
{
  return d->mode;
}

QDateTime MessageQueueJob::sendDueDate() const
{
  if( d->mode != DispatchModeAttribute::AfterDueDate ) {
    kWarning() << "called when mode is not AfterDueDate";
  }
  return d->dueDate;
}

Collection::Id MessageQueueJob::sentMailCollection() const
{
  return d->sentMail;
}

QString MessageQueueJob::from() const
{
  return d->from;
}

QStringList MessageQueueJob::to() const
{
  return d->to;
}

QStringList MessageQueueJob::cc() const
{
  return d->cc;
}

QStringList MessageQueueJob::bcc() const
{
  return d->bcc;
}

void MessageQueueJob::setMessage( Message::Ptr message )
{
  d->message = message;
}

void MessageQueueJob::setTransportId( int id )
{
  d->transport = id;
}

void MessageQueueJob::setDispatchMode( DispatchModeAttribute::DispatchMode mode )
{
  d->mode = mode;
}

void MessageQueueJob::setDueDate( const QDateTime &date )
{
  d->dueDate = date;
}

void MessageQueueJob::setSentMailCollection( Collection::Id id )
{
  d->useDefaultSentMail = false;
  d->sentMail = id;
}

void MessageQueueJob::setFrom( const QString &from )
{
  d->from = from;
}

void MessageQueueJob::setTo( const QStringList &to )
{
  d->to = to;
}

void MessageQueueJob::setCc( const QStringList &cc )
{
  d->cc = cc;
}

void MessageQueueJob::setBcc( const QStringList &bcc )
{
  d->bcc = bcc;
}

void MessageQueueJob::readAddressesFromMime()
{
  d->readAddressesFromMime();
}

void MessageQueueJob::start()
{
  LocalFolders *folders = LocalFolders::self();
  connect( folders, SIGNAL( foldersReady() ),
      this, SLOT( doStart() ) );
  folders->fetch(); // will emit foldersReady()
}

void MessageQueueJob::slotResult( KJob *job )
{
  // error handling
  KCompositeJob::slotResult( job );

  if( !error() ) {
    kDebug() << "item created ok. emitting result.";
    emitResult();
  }
}

#include "messagequeuejob.moc"
