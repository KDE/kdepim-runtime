/*
    Copyright 2008 Ingo Klöcker <kloecker@kde.org>

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

#include "maildispatcheragent.h"

#include "configdialog.h"
#include "dispatchmodeattribute.h"
#include "transportattribute.h"
#include "settings.h"
#include "settingsadaptor.h"

#include <QtDBus/QDBusConnection>

#include <KDebug>
#include <KUrl>
#include <KWindowSystem>

#include <Akonadi/AttributeFactory>
#include <Akonadi/ChangeRecorder>
#include <Akonadi/Collection>
#include <Akonadi/CollectionFetchJob>
#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemFetchScope>
#include <Akonadi/ItemMoveJob>

#include <mailtransport/transport.h>
#include <mailtransport/transportmanager.h>
#include <mailtransport/transportjob.h>

#include <outboxinterface/localfolders.h>

#include <akonadi/kmime/messageparts.h>
#include <kmime/kmime_message.h>
#include <boost/shared_ptr.hpp>
typedef boost::shared_ptr<KMime::Message> MessagePtr;

using namespace Akonadi;
using namespace OutboxInterface;


class MailDispatcherAgent::Private
{
  public:
    Private( MailDispatcherAgent *parent )
        : q( parent )
    {
    }

    ~Private()
    {
    }

    MailDispatcherAgent * const q;

    Collection outbox;
    Collection sentMail;
    QMap < KJob *, Akonadi::Item > sentItems;


    void getCollections();

    // slots:
    void localFoldersReady();
    void itemFetchDone( KJob *job );
    void transportResult( KJob *job );

};


void MailDispatcherAgent::Private::getCollections()
{
  LocalFolders *folders = LocalFolders::self(); //triggers loading/creating collections
  connect( folders, SIGNAL( foldersReady() ), q, SLOT( localFoldersReady() ) );

  // TODO: how to make sure that nothing (like idemAdded) will happen until localFoldersReady?
}

void MailDispatcherAgent::Private::localFoldersReady()
{
  LocalFolders *folders = LocalFolders::self();
  // TODO is it ok to assign collections like this?
  outbox = folders->outbox();
  sentMail = folders->sentMail();

  // start monitoring outbox
  if( !outbox.isValid() ) {
    kFatal() << "invalid outbox collection.";
  }
  kDebug() << "outbox collection (" << outbox.id() << "," << outbox.name() << ").";
  q->changeRecorder()->setCollectionMonitored( outbox, true );

  if( !sentMail.isValid() ) {
    // non-fatal: some users don't have a sent-mail folder at all
    kWarning() << "invalid sent-mail collection.";
  }
  kDebug() << "sent-mail collection (" << sentMail.id() << "," << sentMail.name() << ").";
}


MailDispatcherAgent::MailDispatcherAgent( const QString &id )
  : AgentBase( id ),
    d( new Private( this ) )
{
  // register attributes
  AttributeFactory::registerAttribute<DispatchModeAttribute>();
  AttributeFactory::registerAttribute<TransportAttribute>();

  kDebug() << "maildispatcheragent: At your service, sir!";

  new SettingsAdaptor( Settings::self() );
  QDBusConnection::sessionBus().registerObject( QLatin1String( "/Settings" ),
                              Settings::self(), QDBusConnection::ExportAdaptors );

  // We need to enable this here, otherwise we neither get the remote ID of the
  // parent collection when a collection changes, nor the full item when an item
  // is added.
  changeRecorder()->setChangeRecordingEnabled( false );
  changeRecorder()->fetchCollection( true );
  //changeRecorder()->itemFetchScope().fetchFullPayload( true );
  changeRecorder()->itemFetchScope().fetchPayloadPart( MessagePart::Envelope );

  d->getCollections();
}

MailDispatcherAgent::~MailDispatcherAgent()
{
  if ( !d->sentItems.empty() )
  {
    // can we do kWarning from destructors?
    kWarning() << "agent destroyed while jobs still in progress.";
    // should we attempt to abort the jobs?
  }
  delete d;
}

void MailDispatcherAgent::configure( WId windowId )
{
  kDebug() << "I have no options; you can't break me.";
}

void MailDispatcherAgent::doSetOnline( bool online )
{
  if ( online )
  {
    kDebug() << "maildispatcheragent: We are online. Dispatching pending messages...";
  }
  else
  {
    kDebug() << "maildispatcheragent: We are offline. Dispatching suspended...";
  }
}

void MailDispatcherAgent::collectionChanged( const Collection &col )
{
  if ( col.id() == d->outbox.id() )
  {
    kDebug() << "Outbox collection has changed.";
  }
  else if ( col.id() == d->sentMail.id() )
  {
    kDebug() << "SentMail collection has changed. Why am I getting this?";
  }
  else
  {
    kDebug() << "Collection" << col.id() << "with name" << col.name() << "has changed.";
  }

  // let base implementation tell the change recorder that we
  // have processed the change
  AgentBase::Observer::collectionChanged( col );
/*
  // FIXME This doesn't seem to work: attribute empty
  if ( !col.hasAttribute( "MailDispatcherSort" ) )
    return;

  if ( col.attribute( "MailDispatcherSort" ) != QByteArray( "sort" ) )
    return;

  Collection c( col );
  MailDispatcherAttribute *a = c.attribute<MailDispatcherAttribute>( Collection::AddIfMissing );
  a->deserialize( QByteArray() );
  CollectionModifyJob *job = new CollectionModifyJob( c );
  if ( !job->exec() )
    kDebug() << "Unable to modify collection";
*/
}

void MailDispatcherAgent::collectionRemoved( const Collection &col )
{
  if ( col.id() == d->outbox.id() )
  {
    kDebug() << "Outbox collection has been removed.";
  }
  else if ( col.id() == d->sentMail.id() )
  {
    kDebug() << "SentMail collection has been removed.";
  }
  else
  {
    kDebug() << "Collection" << col.id() << "with name" << col.name() << "has been removed.";
  }

  // let base implementation tell the change recorder that we
  // have processed the change
  AgentBase::Observer::collectionRemoved( col );
}

void MailDispatcherAgent::itemAdded( const Item &item, const Collection &col )
{
  if ( col.id() == d->outbox.id() )
  {
    kDebug() << "An item was added to the outbox.";
    if ( isOnline() )
    {
      kDebug() << "fetching item.";
      ItemFetchJob *fetchJob = new ItemFetchJob( item, this );
      fetchJob->fetchScope().fetchFullPayload();
      connect( fetchJob, SIGNAL( result( KJob* ) ), SLOT( itemFetchDone( KJob* ) ) );
    }
    else
    {
      kDebug() << "maildispatcheragent: We are offline. Putting message in spool..." ;
    }
  }
  else
  {
    kDebug() << "An item was added to collection" << col.id() << "with name" << col.name();
  }

  Item modifiedItem = Item( item );

  // let base implementation tell the change recorder that we
  // have processed the change
  AgentBase::Observer::itemAdded( item, col );
}

void MailDispatcherAgent::itemChanged( const Item &item,
  const QSet< QByteArray > &partIdentifiers )
{
  kDebug() << "An item has changed. What am I supposed to do??";

  // let base implementation tell the change recorder that we
  // have processed the change
  AgentBase::Observer::itemChanged( item, partIdentifiers );
}

void MailDispatcherAgent::itemRemoved( const Item &item )
{
  kDebug() << "An item was removed. Big deal.";
  // nothing to do for us

  // let base implementation tell the change recorder that we
  // have processed the change
  AgentBase::Observer::itemRemoved( item );
}

void MailDispatcherAgent::Private::itemFetchDone( KJob *job )
{
  ItemFetchJob *fetchJob = static_cast<ItemFetchJob*>( job );
  if ( job->error() )
  {
    kError() << job->errorString();
    return;
  }

  if ( fetchJob->items().isEmpty() )
  {
    kWarning() << "Job did not retrieve any items";
    return;
  }

  Item item = fetchJob->items().first();
  if ( !item.isValid() )
  {
    kWarning() << "Item not valid";
    return;
  }

  if ( !item.hasPayload<MessagePtr>() )
  {
    kWarning() << "Item does not have message payload";
    return;
  }

  const MessagePtr message = item.payload<MessagePtr>();

  // following code mostly stolen from Mailody:
  const QByteArray cmsg = message->encodedContent( true ) + "\r\n";
  kDebug() << cmsg;
  if ( cmsg.isEmpty() )
    return;

  // hardwired to use default transport
  const int transportId = MailTransport::TransportManager::self()->defaultTransportId();
  if ( transportId == -1 )
  {
    // HACK: checking for -1 breaks encapsulation. TransportManager should provide a hasDefaultTransport()
    kWarning() << "There is no default mail transport. I can't send anything.";
    return;
  }
  MailTransport::Transport *base =
    MailTransport::TransportManager::self()->transportById( transportId );
  if ( base == 0 )
  {
    kWarning() << "I got a null transport!";
    return;
  }
  kDebug() << "Sending to:" << base->host() << base->port();

  MailTransport::TransportJob *sendJob =
      MailTransport::TransportManager::self()->createTransportJob( transportId );
  if ( !sendJob )
  {
    kWarning() << "Not possible to create TransportJob!";
    return;
  }

  sendJob->setData( cmsg );
  // hardwired sender
  sendJob->setSender( "fake-sender-in-code@naiba.md" );
  // hardwired receiver
  sendJob->setTo( QStringList( "idanoka@gmail.com" ) );

  q->connect( sendJob, SIGNAL( result( KJob* ) ), q, SLOT( transportResult( KJob* ) ) );
  // keep track of what item this jobs refers to (is there a better way?!)
  sentItems[sendJob] = item;
  MailTransport::TransportManager::self()->schedule( sendJob );
}

void MailDispatcherAgent::Private::transportResult( KJob *job )
{
  if ( job->error() )
  {
    kWarning() << "TransportJob reported error...";
    return;
  }

  if ( !sentMail.isValid() )
  {
    kWarning() << "Invalid sent-mail collection.";
    return;
  }

  if ( !sentItems.contains( job ) )
  {
    kWarning() << "I know of no such job!";
    return;
  }

  kDebug() << "TransportJob succeeded. Moving message to sent-mail.";
  Item item = sentItems.value(job);
  sentItems.remove(job);
  ItemMoveJob *mjob = new ItemMoveJob(item, sentMail);
  // TODO: care about the result
  kDebug() << "MoveJob created.";
}


AKONADI_AGENT_MAIN( MailDispatcherAgent )

#include "maildispatcheragent.moc"
