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

    Collection outbox;
    // TODO: only SendJob should worry about sent-mail
    Collection sentMail;
    ItemFilterProxyModel *model;
    Item currentItem;
    KJob *currentJob;

    void getLocalFolders();
    void connectModel( bool connect );

    // slots:
    void dispatch();
    void localFoldersReady();
    void itemFetched( Item &item );
    void sendResult( KJob *job );

};


void MailDispatcherAgent::Private::getLocalFolders()
{
  LocalFolders *folders = LocalFolders::self(); //triggers loading/creating collections
  connect( folders, SIGNAL( foldersReady() ), q, SLOT( localFoldersReady() ) );
}

void MailDispatcherAgent::Private::connectModel( bool connect )
{
  Q_ASSERT( model );
  if( connect ) {
    kDebug() << "Online. Connecting model.";
    q->connect( model, SIGNAL( rowsInserted( QModelIndex, int, int ) ),
        q, SLOT( dispatch() ) );
    q->connect( model, SIGNAL( itemFetched( Akonadi::Item& ) ),
        q, SLOT( itemFetched( Akonadi::Item& ) ) );
    dispatch();
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

void MailDispatcherAgent::Private::localFoldersReady()
{
  LocalFolders *folders = LocalFolders::self();
  // TODO is it ok to assign collections like this?
  outbox = folders->outbox();
  sentMail = folders->sentMail();

  // create model to monitor outbox
  if( !outbox.isValid() ) {
    kFatal() << "invalid outbox collection.";
  }
  kDebug() << "outbox collection (" << outbox.id() << "," << outbox.name() << ").";
  model = new ItemFilterProxyModel();
  ItemModel *itemModel = new ItemModel( model ); // child -> autodeletes
  // we need to set the fetchScope before calling setCollection
  itemModel->fetchScope().fetchAllAttributes(); // what ItemFilterProxyModel needs to check
  itemModel->setCollection( outbox );
  model->setSourceModel( itemModel );
  connectModel( q->isOnline() );

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
  AttributeFactory::registerAttribute<AddressAttribute>();
  AttributeFactory::registerAttribute<DispatchModeAttribute>();
  AttributeFactory::registerAttribute<TransportAttribute>();

  kDebug() << "maildispatcheragent: At your service, sir!";

  new SettingsAdaptor( Settings::self() );
  QDBusConnection::sessionBus().registerObject( QLatin1String( "/Settings" ),
                              Settings::self(), QDBusConnection::ExportAdaptors );

  d->getLocalFolders();
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
  currentItem = item;

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
