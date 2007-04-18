/*
    This file is part of libakonadi.

    Copyright (c) 2006 Till Adam <adam@kde.org>
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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

#include <kcmdlineargs.h>
#include <klocale.h>

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QHash>
#include <QtCore/QTimer>
#include <QtGui/QApplication>
#include <QtDBus/QtDBus>

#include <signal.h>
#include <stdlib.h>

#include "kcrash.h"
#include "resourcebase.h"
#include "resourceadaptor.h"
#include "collectionsync.h"

#include "tracerinterface.h"

#include <libakonadi/collectionlistjob.h>
#include <libakonadi/itemfetchjob.h>
#include <libakonadi/itemstorejob.h>
#include <libakonadi/job.h>
#include <libakonadi/session.h>
#include <libakonadi/monitor.h>

using namespace Akonadi;

static ResourceBase *sResourceBase = 0;

void crashHandler( int signal )
{
  if ( sResourceBase )
    sResourceBase->crashHandler( signal );

  exit( 255 );
}

class ResourceBase::Private
{
  public:
    Private()
      : mStatusCode( Ready ),
        mProgress( 0 ),
        mSettings( 0 ),
        online( true ),
        changeRecording( false ),
        syncState( Idle )
    {
      mStatusMessage = defaultReadyMessage();
    }

    QString defaultReadyMessage() const;
    QString defaultSyncingMessage() const;
    QString defaultErrorMessage() const;

    org::kde::Akonadi::Tracer *mTracer;
    QString mId;
    QString mName;

    int mStatusCode;
    QString mStatusMessage;

    uint mProgress;
    QString mProgressMessage;

    QSettings *mSettings;

    Session *session;
    Monitor *monitor;
    QHash<Akonadi::Job*, QDBusMessage> pendingReplys;

    bool online;

    // change recording
    enum ChangeType {
      ItemAdded, ItemChanged, ItemRemoved, CollectionAdded, CollectionChanged, CollectionRemoved
    };
    struct ChangeItem {
      ChangeType type;
      DataReference item;
      QString collection;
    };
    bool changeRecording;
    QList<ChangeItem> changes;

    void addChange( ChangeItem &c )
    {
      bool skipCurrent = false;
      // compress changes
      for ( QList<ChangeItem>::Iterator it = changes.begin(); it != changes.end(); ) {
        if ( !(*it).item.isNull() && (*it).item == c.item ) {
          if ( (*it).type == ItemAdded && c.type == ItemRemoved )
            skipCurrent = true; // add + remove -> noop
          else if ( (*it).type == ItemAdded && c.type == ItemChanged )
            c.type = ItemAdded; // add + change -> add
          it = changes.erase( it );
        } else if ( !(*it).collection.isEmpty() && (*it).collection == c.collection ) {
          if ( (*it).type == CollectionAdded && c.type == CollectionRemoved )
            skipCurrent = true; // add + remove -> noop
          else if ( (*it).type == CollectionAdded && c.type == CollectionChanged )
            c.type = ItemAdded; // add + change -> add
          it = changes.erase( it );
        } else
          ++it;
      }

      if ( !skipCurrent )
        changes << c;
    }

    void loadChanges()
    {
      changes.clear();
      mSettings->beginGroup( "Changes" );
      int size = mSettings->beginReadArray( "change" );
      for ( int i = 0; i < size; ++i ) {
        mSettings->setArrayIndex( i );
        ChangeItem c;
        c.type = (ChangeType)mSettings->value( "type" ).toInt();
        c.item = DataReference( mSettings->value( "item_uid" ).toInt(), mSettings->value( "item_rid" ).toString() );
        c.collection =  mSettings->value( "collection" ).toString();
        changes << c;
      }
      mSettings->endArray();
      mSettings->endGroup();
      changeRecording = mSettings->value( "Resource/ChangeRecording", false ).toBool();
    }

    void saveChanges()
    {
      mSettings->beginGroup( "Changes" );
      mSettings->beginWriteArray( "change", changes.count() );
      for ( int i = 0; i < changes.count(); ++i ) {
        mSettings->setArrayIndex( i );
        ChangeItem c = changes.at( i );
        mSettings->setValue( "type", c.type );
        mSettings->setValue( "item_uid", c.item.persistanceID() );
        mSettings->setValue( "item_rid", c.item.externalUrl() );
        mSettings->setValue( "collection", c.collection );
      }
      mSettings->endArray();
      mSettings->endGroup();
      mSettings->setValue( "Resource/ChangeRecording", changeRecording );
    }

    // synchronize states
    enum SyncState {
      Idle,
      RetrievingRemoteCollections,
      SyncingCollections,
      RetrievingLocalCollections,
      SyncingSingleCollection
    };
    SyncState syncState;

    Collection::List localCollections;
    Collection currentCollection;
};

QString ResourceBase::Private::defaultReadyMessage() const
{
  if ( online )
    return i18n( "Ready" );
  return i18n( "Offline" );
}

QString ResourceBase::Private::defaultSyncingMessage() const
{
  return tr( "Syncing..." );
}

QString ResourceBase::Private::defaultErrorMessage() const
{
  return tr( "Error!" );
}

ResourceBase::ResourceBase( const QString & id )
  : d( new Private )
{
  KCrash::init();
  KCrash::setEmergencyMethod( ::crashHandler );
  sResourceBase = this;

  d->mTracer = new org::kde::Akonadi::Tracer( "org.kde.Akonadi", "/tracing", QDBusConnection::sessionBus(), this );

  if ( !QDBusConnection::sessionBus().registerService( "org.kde.Akonadi.Resource." + id ) )
    error( QString( "Unable to register service at dbus: %1" ).arg( QDBusConnection::sessionBus().lastError().message() ) );

  new ResourceAdaptor( this );
  if ( !QDBusConnection::sessionBus().registerObject( "/", this, QDBusConnection::ExportAdaptors ) )
    error( QString( "Unable to register object at dbus: %1" ).arg( QDBusConnection::sessionBus().lastError().message() ) );

  d->mId = id;

  d->mSettings = new QSettings( QString( "%1/.akonadi/resource_config_%2" ).arg( QDir::homePath(), id ), QSettings::IniFormat );

  const QString name = d->mSettings->value( "Resource/Name" ).toString();
  if ( !name.isEmpty() )
    d->mName = name;

  d->online = settings()->value( "Resource/Online", true ).toBool();

  d->session = new Session( d->mId.toLatin1(), this );
  d->monitor = new Monitor( this );
  connect( d->monitor, SIGNAL(itemAdded(const Akonadi::Item&)), SLOT(slotItemAdded(const Akonadi::Item&)) );
  connect( d->monitor, SIGNAL(itemChanged(const Akonadi::Item&)), SLOT(slotItemChanged(const Akonadi::Item&)) );
  connect( d->monitor, SIGNAL(itemRemoved(const Akonadi::DataReference&)), SLOT(slotItemRemoved(const Akonadi::DataReference&)) );
  d->monitor->ignoreSession( session() );
  d->monitor->monitorResource( d->mId.toLatin1() );

  d->loadChanges();

  // initial configuration
  bool initialized = settings()->value( "Resource/Initialized", false ).toBool();
  if ( !initialized ) {
    QTimer::singleShot( 0, this, SLOT(configure()) ); // finish construction first
    settings()->setValue( "Resource/Initialized", true );
  }
}

ResourceBase::~ResourceBase()
{
  delete d->mSettings;
  delete d;
}

int ResourceBase::status() const
{
  return d->mStatusCode;
}

QString ResourceBase::statusMessage() const
{
  return d->mStatusMessage;
}

uint ResourceBase::progress() const
{
  return d->mProgress;
}

QString ResourceBase::progressMessage() const
{
  return d->mProgressMessage;
}

void ResourceBase::warning( const QString& message )
{
  d->mTracer->warning( QString( "ResourceBase(%1)" ).arg( d->mId ), message );
}

void ResourceBase::error( const QString& message )
{
  d->mTracer->error( QString( "ResourceBase(%1)" ).arg( d->mId ), message );
}

void ResourceBase::changeStatus( Status status, const QString &message )
{
  d->mStatusMessage = message;
  d->mStatusCode = 0;

  switch ( status ) {
    case Ready:
      if ( d->mStatusMessage.isEmpty() )
        d->mStatusMessage = d->defaultReadyMessage();

      d->mStatusCode = 0;
      break;
    case Syncing:
      if ( d->mStatusMessage.isEmpty() )
        d->mStatusMessage = d->defaultSyncingMessage();

      d->mStatusCode = 1;
      break;
    case Error:
      if ( d->mStatusMessage.isEmpty() )
        d->mStatusMessage = d->defaultErrorMessage();

      d->mStatusCode = 2;
      break;
    default:
      Q_ASSERT( !"Unknown status passed" );
      break;
  }

  emit statusChanged( d->mStatusCode, d->mStatusMessage );
}

void ResourceBase::changeProgress( uint progress, const QString &message )
{
  d->mProgress = progress;
  d->mProgressMessage = message;

  emit progressChanged( d->mProgress, d->mProgressMessage );
}

void ResourceBase::configure()
{
}

void ResourceBase::synchronize()
{
  if ( d->syncState != Private::Idle )
    return;

  d->syncState = Private::RetrievingRemoteCollections;
  retrieveCollections();
}

void ResourceBase::setName( const QString &name )
{
  if ( name == d->mName )
    return;

  // TODO: rename collection
  d->mName = name;

  if ( d->mName.isEmpty() || d->mName == d->mId )
    d->mSettings->remove( "Resource/Name" );
  else
    d->mSettings->setValue( "Resource/Name", d->mName );

  d->mSettings->sync();

  emit nameChanged( d->mName );
}

QString ResourceBase::name() const
{
  if ( d->mName.isEmpty() )
    return d->mId;
  else
    return d->mName;
}

QString ResourceBase::parseArguments( int argc, char **argv )
{
  QString identifier;
  if ( argc && argv ) {
    if ( argc < 3 ) {
      qDebug( "ResourceBase::parseArguments: Not enough arguments passed..." );
      exit( 1 );
    }

    for ( int i = 1; i < argc - 1; ++i ) {
      if ( QString( argv[ i ] ) == "--identifier" )
        identifier = QString( argv[ i + 1 ] );
    }
  } else {
    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
    if ( args && args->isSet( "identifier" ) )
      identifier = QString::fromLatin1( args->getOption( "identifier" ) );
  }

  if ( identifier.isEmpty() ) {
    qDebug( "ResourceBase::parseArguments: Identifier argument missing" );
    exit( 1 );
  }

  QApplication::setQuitOnLastWindowClosed( false );

  return identifier;
}

void ResourceBase::quit()
{
  d->saveChanges();
  aboutToQuit();

  d->mSettings->sync();

  QCoreApplication::exit( 0 );
}

void ResourceBase::aboutToQuit()
{
}

QString ResourceBase::identifier() const
{
  return d->mId;
}

void ResourceBase::cleanup() const
{
  const QString fileName = d->mSettings->fileName();

  /**
   * First destroy the settings object...
   */
  delete d->mSettings;
  d->mSettings = 0;

  /**
   * ... then remove the file from hd.
   */
  QFile::remove( fileName );

  QCoreApplication::exit( 0 );
}

void ResourceBase::crashHandler( int signal )
{
  /**
   * If we retrieved a SIGINT or SIGTERM we close normally
   */
  if ( signal == SIGINT || signal == SIGTERM )
    quit();
}

QSettings* ResourceBase::settings()
{
  return d->mSettings;
}

Session* ResourceBase::session()
{
  return d->session;
}

bool ResourceBase::deliverItem(Akonadi::Job * job, const QDBusMessage & msg)
{
  msg.setDelayedReply( true );
  d->pendingReplys.insert( job, msg.createReply() );
  connect( job, SIGNAL(result(KJob*)), SLOT(slotDeliveryDone(KJob*)) );
  return false;
}

void ResourceBase::slotDeliveryDone(KJob * job)
{
  Q_ASSERT( d->pendingReplys.contains( static_cast<Akonadi::Job*>( job ) ) );
  QDBusMessage reply = d->pendingReplys.take( static_cast<Akonadi::Job*>( job ) );
  if ( job->error() ) {
    error( "Error while creating item: " + job->errorString() );
    reply << false;
  } else {
    reply << true;
  }
  QDBusConnection::sessionBus().send( reply );
}

void ResourceBase::enableChangeRecording(bool enable)
{
  if ( d->changeRecording == enable )
    return;
  d->changeRecording = enable;
  if ( !d->changeRecording ) {
    slotReplayNextItem();
  }
}

void ResourceBase::slotReplayNextItem()
{
  if ( d->changes.count() > 0 ) {
    const Private::ChangeItem c = d->changes.takeFirst();
    switch ( c.type ) {
      case Private::ItemAdded:
        {
          ItemFetchJob *job = new ItemFetchJob( c.item, this );
          connect( job, SIGNAL( result( KJob* ) ), this, SLOT( slotReplayItemAdded( KJob* ) ) );
          job->start();
        }
        break;
      case Private::ItemChanged:
        {
          ItemFetchJob *job = new ItemFetchJob( c.item, this );
          connect( job, SIGNAL( result( KJob* ) ), this, SLOT( slotReplayItemChanged( KJob* ) ) );
          job->start();
        }
        break;
      case Private::ItemRemoved:
        itemRemoved( c.item );
        break;
      case Private::CollectionAdded:
      case Private::CollectionChanged:
      case Private::CollectionRemoved:
        // TODO
        break;
    }
  }
}

void ResourceBase::slotReplayItemAdded( KJob *job )
{
  if ( job->error() ) {
    error( i18n( "Unable to fetch item in replay mode." ) );
  } else {
    ItemFetchJob *fetchJob = qobject_cast<ItemFetchJob*>( job );

    const Item item = fetchJob->items().first();
    if ( item.isValid() )
      itemAdded( item );
  }

  QTimer::singleShot( 0, this, SLOT( slotReplayNextItem() ) );
}

void ResourceBase::slotReplayItemChanged( KJob *job )
{
  if ( job->error() ) {
    error( i18n( "Unable to fetch item in replay mode." ) );
  } else {
    ItemFetchJob *fetchJob = qobject_cast<ItemFetchJob*>( job );

    const Item item = fetchJob->items().first();
    if ( item.isValid() )
      itemChanged( item );
  }

  QTimer::singleShot( 0, this, SLOT( slotReplayNextItem() ) );
}

void ResourceBase::slotItemAdded( const Akonadi::Item &item )
{
  if ( d->changeRecording ) {
    Private::ChangeItem c;
    c.type = Private::ItemAdded;
    c.item = item.reference();
    d->addChange( c );
  } else {
    itemAdded( item );
  }
}

void ResourceBase::slotItemChanged( const Akonadi::Item &item )
{
  if ( d->changeRecording ) {
    Private::ChangeItem c;
    c.type = Private::ItemChanged;
    c.item = item.reference();
    d->addChange( c );
  } else {
    itemChanged( item );
  }
}

void ResourceBase::slotItemRemoved(const Akonadi::DataReference & ref)
{
  if ( d->changeRecording ) {
    Private::ChangeItem c;
    c.type = Private::ItemRemoved;
    c.item = ref;
    d->addChange( c );
  } else {
    itemRemoved( ref );
  }
}

void ResourceBase::changesCommitted(const DataReference & ref)
{
  ItemStoreJob *job = new ItemStoreJob( ref, session() );
  job->setRemoteId();
  job->setClean();
}

bool ResourceBase::requestItemDelivery(int uid, const QString & remoteId, int type )
{
  if ( !isOnline() ) {
    error( i18n( "Cannot fetch item in offline mode." ) );
    return false;
  }
  return requestItemDelivery( DataReference( uid, remoteId ), type, message() );
}

bool ResourceBase::isOnline() const
{
  return d->online;
}

void ResourceBase::setOnline(bool state)
{
  d->online = state;
  settings()->setValue( "Resource/Online", state );
  enableChangeRecording( !state );
}

void ResourceBase::collectionsRetrieved(const Collection::List & collections)
{
  CollectionSync *syncer = new CollectionSync( d->mId, session() );
  syncer->setRemoteCollections( collections );
  d->syncState = Private::SyncingCollections;
  connect( syncer, SIGNAL(result(KJob*)), SLOT(slotCollectionSyncDone(KJob*)) );
}

void ResourceBase::collectionsRetrievedIncremental(const Collection::List & changedCollections, const Collection::List & removedCollections)
{
  CollectionSync *syncer = new CollectionSync( d->mId, session() );
  syncer->setRemoteCollections( changedCollections, removedCollections );
  d->syncState = Private::SyncingCollections;
  connect( syncer, SIGNAL(result(KJob*)), SLOT(slotCollectionSyncDone(KJob*)) );
}

void ResourceBase::slotCollectionSyncDone(KJob * job)
{
  if ( job->error() ) {
    error( job->errorString() );
    d->syncState = Private::Idle;
    return;
  }

  d->syncState = Private::RetrievingLocalCollections;
  CollectionListJob *list = new CollectionListJob( Collection::root(), CollectionListJob::Recursive, session() );
  list->setResource( d->mId );
  connect( list, SIGNAL(result(KJob*)), SLOT(slotLocalListDone(KJob*)) );
}

void ResourceBase::slotLocalListDone(KJob * job)
{
  if ( job->error() ) {
    error( job->errorString() );
    d->syncState = Private::Idle;
    return;
  }

  d->localCollections = static_cast<CollectionListJob*>( job )->collections();

  // make sure all signals are emitted before we enter syncCollection()
  // which might use exec() and block signals to Session which causes a deadlock
  QTimer::singleShot( 0, this, SLOT(slotSyncNextCollection()) );
  d->syncState = Private::SyncingSingleCollection;
}

void ResourceBase::slotSyncNextCollection()
{
  while ( !d->localCollections.isEmpty() ) {
    d->currentCollection = d->localCollections.takeFirst();
    // check if this collection actually can contain anything
    QStringList contentTypes = d->currentCollection.contentTypes();
    contentTypes.removeAll( Collection::collectionMimeType() );
    if ( !contentTypes.isEmpty() ) {
      changeStatus( Syncing, i18n( "Syncing collection '%1'", d->currentCollection.name() ) );
      synchronizeCollection( d->currentCollection );
      return;
    }
  }

  d->syncState = Private::Idle;
  changeStatus( Ready );
}

void ResourceBase::collectionSynchronized()
{
  QTimer::singleShot( 0, this, SLOT(slotSyncNextCollection()) );
}

Collection ResourceBase::currentCollection() const
{
  return d->currentCollection;
}
