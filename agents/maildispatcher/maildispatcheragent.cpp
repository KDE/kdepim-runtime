/*
    Copyright 2008 Ingo Klöcker <kloecker@kde.org>
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

#include "maildispatcheragent.h"

#include "configdialog.h"
#include "outboxqueue.h"
#include "sendjob.h"
#include "settings.h"
#include "settingsadaptor.h"

#include <QtDBus/QDBusConnection>
#include <QTimer>

#include <KDebug>
#include <KWindowSystem>

#include <Akonadi/AttributeFactory>
#include <Akonadi/ItemFetchScope>

#include <outboxinterface/localfolders.h>
#include <outboxinterface/addressattribute.h>
#include <outboxinterface/dispatchmodeattribute.h>
#include <outboxinterface/errorattribute.h>
#include <outboxinterface/sentcollectionattribute.h>
#include <outboxinterface/transportattribute.h>

using namespace Akonadi;
using namespace OutboxInterface;


class MailDispatcherAgent::Private
{
  public:
    Private( MailDispatcherAgent *parent )
        : q( parent )
    {
      currentJob = 0;
    }

    ~Private()
    {
    }

    MailDispatcherAgent * const q;

    OutboxQueue *queue;
    KJob *currentJob;

    // slots:
    void dispatch();
    void localFoldersChanged();
    void itemFetched( Item &item );
    void sendResult( KJob *job );

};


void MailDispatcherAgent::Private::dispatch()
{
  Q_ASSERT( queue );

  if( !q->isOnline() ) {
    kDebug() << "Offline. See you later.";
    return;
  }

  if( currentJob ) {
    kDebug() << "Another job is active. See you later.";
    return;
  }

  kDebug() << "Attempting to dispatch the next message.";
  queue->fetchOne(); // will trigger itemFetched
}

void MailDispatcherAgent::Private::localFoldersChanged()
{
  // NOTE that this is called whenever the outbox/sent-mail collections change,
  // not only on startup.

  Collection outbox = LocalFolders::self()->outbox();
  Q_ASSERT( outbox.isValid() );
  kDebug() << "Outbox collection changed to (" << outbox.id() << "," << outbox.name() << ").";
  Q_ASSERT( queue );
  queue->setCollection( outbox );
}


MailDispatcherAgent::MailDispatcherAgent( const QString &id )
  : AgentBase( id ),
    d( new Private( this ) )
{
  // register attributes
  AttributeFactory::registerAttribute<AddressAttribute>();
  AttributeFactory::registerAttribute<DispatchModeAttribute>();
  AttributeFactory::registerAttribute<ErrorAttribute>();
  AttributeFactory::registerAttribute<SentCollectionAttribute>();
  AttributeFactory::registerAttribute<TransportAttribute>();

  kDebug() << "maildispatcheragent: At your service, sir!";

  new SettingsAdaptor( Settings::self() );
  QDBusConnection::sessionBus().registerObject( QLatin1String( "/Settings" ),
                              Settings::self(), QDBusConnection::ExportAdaptors );

  d->queue = new OutboxQueue( this );
  connect( d->queue, SIGNAL( newItems() ), this, SLOT( dispatch() ) );
  connect( d->queue, SIGNAL( itemReady( Akonadi::Item& ) ),
      this, SLOT( itemFetched( Akonadi::Item& ) ) );
  LocalFolders *folders = LocalFolders::self();
  connect( folders, SIGNAL( foldersReady() ), this, SLOT( localFoldersChanged() ) );
  folders->fetch(); // will emit foldersReady()
}

MailDispatcherAgent::~MailDispatcherAgent()
{
  delete d;
}

void MailDispatcherAgent::configure( WId windowId )
{
  Q_UNUSED( windowId );
  kDebug() << "I have no options; you can't break me.";
}

void MailDispatcherAgent::doSetOnline( bool online )
{
  Q_ASSERT( d->queue );
  if( online ) {
    kDebug() << "Online. Dispatching messages.";
    QTimer::singleShot( 0, this, SLOT( dispatch() ) );
  } else {
    kDebug() << "Offline.";

    // TODO: This way, the OutboxQueue will continue to react to changes in
    // the outbox, but the MDA will just not send anything.  Is this what we
    // want?
  }
  
  AgentBase::doSetOnline( online );
}

void MailDispatcherAgent::Private::itemFetched( Item &item )
{
  kDebug() << "Fetched item" << item.id() << "; creating SendJob.";
  Q_ASSERT( currentJob == 0 );
  currentJob = new SendJob( item );
  connect( currentJob, SIGNAL( result( KJob* ) ),
      q, SLOT( sendResult( KJob* ) ) );
  currentJob->start();
}

void MailDispatcherAgent::Private::sendResult( KJob *job )
{
  Q_ASSERT( job == currentJob );
  currentJob->disconnect( q );
  currentJob = 0;

  if( job->error() ) {
    // The SendJob gave the item an ErrorAttribute, so we don't have to
    // do anything.
    kDebug() << "Sending failed. error:" << job->errorString();
  } else {
    kDebug() << "Sending succeeded.";
  }

  // dispatch next message
  QTimer::singleShot( 0, q, SLOT( dispatch() ) );
}


AKONADI_AGENT_MAIN( MailDispatcherAgent )


#include "maildispatcheragent.moc"
