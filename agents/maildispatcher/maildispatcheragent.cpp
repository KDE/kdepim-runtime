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
#include "itemfilterproxymodel.h"
#include "sendjob.h"
#include "settings.h"
#include "settingsadaptor.h"

#include <QtDBus/QDBusConnection>
#include <QTimer>

#include <KDebug>
#include <KWindowSystem>

#include <Akonadi/AttributeFactory>
#include <Akonadi/ItemFetchScope>
#include <Akonadi/ItemModel>

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
      model = 0;
      currentJob = 0;
    }

    ~Private()
    {
      delete model;
    }

    MailDispatcherAgent * const q;

    ItemFilterProxyModel *model;
    KJob *currentJob;

    void connectModel( bool connect );

    // slots:
    void dispatch();
    void localFoldersChanged();
    void itemFetched( Item &item );
    void sendResult( KJob *job );

};


void MailDispatcherAgent::Private::connectModel( bool connect )
{
  Q_ASSERT( model );
  if( connect ) {
    kDebug() << "Online. Connecting model.";
    q->connect( model, SIGNAL( rowsInserted( QModelIndex, int, int ) ),
        q, SLOT( dispatch() ) );
    q->connect( model, SIGNAL( itemFetched( Akonadi::Item& ) ),
        q, SLOT( itemFetched( Akonadi::Item& ) ) );
    QTimer::singleShot( 0, q, SLOT( dispatch() ) );
  } else {
    kDebug() << "Offline. Disconnecting model.";
    model->disconnect( q );
  }
}

void MailDispatcherAgent::Private::dispatch()
{
  Q_ASSERT( model );

  if( currentJob ) {
    kDebug() << "Another job is active. See you later.";
    return;
  }

  kDebug() << "Attempting to dispatch the next message.";
  model->fetchAnItem(); // will trigger itemFetched
}

void MailDispatcherAgent::Private::localFoldersChanged()
{
  // NOTE that this is called whenever the outbox/sent-mail collections change,
  // not only on startup.

  Collection outbox = LocalFolders::self()->outbox();
  Q_ASSERT( outbox.isValid() );
  kDebug() << "Outbox collection changed to (" << outbox.id() << "," << outbox.name() << ").";
  ItemModel *itemModel = dynamic_cast<ItemModel*>( model->sourceModel() );
  Q_ASSERT( itemModel );
  itemModel->setCollection( outbox );
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

  d->model = new ItemFilterProxyModel();
  ItemModel *itemModel = new ItemModel( d->model ); // child -> autodeletes
  // we need to set the fetchScope before calling setCollection
  itemModel->fetchScope().fetchAllAttributes(); // what ItemFilterProxyModel needs to check
  d->model->setSourceModel( itemModel );
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
  if( d->model ) {
    d->connectModel( online );
  }
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
