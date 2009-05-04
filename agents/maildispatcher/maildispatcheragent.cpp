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
#include "maildispatcherattribute.h"
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
#include <Akonadi/ItemDeleteJob>
#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemFetchScope>

#include <mailtransport/transport.h>
#include <mailtransport/transportmanager.h>
#include <mailtransport/transportjob.h>

#include <akonadi/kmime/messageparts.h>
#include <kmime/kmime_message.h>
#include <boost/shared_ptr.hpp>
typedef boost::shared_ptr<KMime::Message> MessagePtr;

using namespace Akonadi;


void MailDispatcherAgent::updateMonitoredCollections()
{
  const Entity::Id outboxId = Settings::self()->outbox();
  if ( mOutbox.id() != outboxId ) // it changed
  {
    // stop monitoring the old collection
    if ( mOutbox.isValid() )
    {
      changeRecorder()->setCollectionMonitored( mOutbox, false );
    }

    mOutbox = Collection();
    if ( outboxId != -1 )
    {
      CollectionFetchJob *job = new CollectionFetchJob( Collection( outboxId ), CollectionFetchJob::Base );
      if ( job->exec() )
      {
        Collection::List collections = job->collections();
        if ( collections.size() == 1 )
        {
          mOutbox = collections.first();
        }
      }
    }

    // start monitoring the new collection
    if ( mOutbox.isValid() )
    {
      kDebug() << "maildispatcheragent: Monitoring collection ("
               << mOutbox.id() << "," << mOutbox.name() << ") as outbox.";
      changeRecorder()->setCollectionMonitored( mOutbox, true );
    }
  }

  const Entity::Id sentId = Settings::self()->sentMail();
  if ( mSentMail.id() != sentId )
  {
    // we do not monitor the sent-mail collection, only keep track of it.
    mSentMail = Collection();
    if ( sentId != -1 )
    {
      CollectionFetchJob *job = new CollectionFetchJob( Collection( sentId ), CollectionFetchJob::Base );
      if ( job->exec() )
      {
        Collection::List collections = job->collections();
        if ( collections.size() == 1 )
        {
          mSentMail = collections.first();
        }
      }
    }

    if ( mSentMail.isValid() )
    {
      kDebug() << "maildispatcheragent: SentMail collection was changed to ("
               << mSentMail.id() << "," << mSentMail.name() << ").";
    }
  }
}


MailDispatcherAgent::MailDispatcherAgent( const QString &id )
  : AgentBase( id )
{
  // register MailDispatcherAttribute
  AttributeFactory::registerAttribute<MailDispatcherAttribute>();

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

  updateMonitoredCollections();
}

MailDispatcherAgent::~MailDispatcherAgent()
{
  if ( !mSentItems.empty() )
  {
    // can we do kWarning from destructors?
    kWarning() << "agent destroyed while jobs still in progress.";
    // should we attempt to abort the jobs?
  }
}

void MailDispatcherAgent::configure( WId windowId )
{
  ConfigDialog dlg;
  if ( windowId )
  {
    KWindowSystem::setMainWindow( &dlg, windowId );
  }
  dlg.exec();

  updateMonitoredCollections();
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
  if ( col.id() == mOutbox.id() )
  {
    kDebug() << "Outbox collection has changed.";
  }
  else if ( col.id() == mSentMail.id() )
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
  if ( col.id() == mOutbox.id() )
  {
    kDebug() << "Outbox collection has been removed.";
  }
  else if ( col.id() == mSentMail.id() )
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
  if ( col.id() == mOutbox.id() )
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

void MailDispatcherAgent::itemFetchDone( KJob *job )
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

  connect( sendJob, SIGNAL( result( KJob* ) ), SLOT( transportResult( KJob* ) ) );
  // keep track of what item this jobs refers to (is there a better way?!)
  mSentItems[sendJob] = item;
  MailTransport::TransportManager::self()->schedule( sendJob );
}

void MailDispatcherAgent::transportResult( KJob *job )
{
  if ( job->error() )
  {
    kWarning() << "TransportJob reported error...";
  }
  else
  {
    // TODO: move message to sent-mail
    kDebug() << "TransportJob succeeded. Removing message from outbox.";
    if ( !mSentItems.contains( job ) )
    {
      kWarning() << "I know of no such job!";
      return;
    }

    Item item = mSentItems.value(job);
    mSentItems.remove(job);
    ItemDeleteJob *job = new ItemDeleteJob(item);
    // do we care about the result?
    kDebug() << "ItemDeleteJob created.";
  }
}


AKONADI_AGENT_MAIN( MailDispatcherAgent )

#include "maildispatcheragent.moc"
