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

#include "localfolders.h"

#include "settings.h"

#include <QApplication>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusReply>
#include <QObject>
#include <QTimer>
#include <QVariant>

#include <KDebug>
#include <KGlobal>
#include <KLocalizedString>
#include <KStandardDirs>

#include <Akonadi/AgentInstance>
#include <Akonadi/AgentInstanceCreateJob>
#include <Akonadi/AgentManager>
#include <Akonadi/Collection>
#include <Akonadi/CollectionCreateJob>
#include <Akonadi/CollectionFetchJob>
#include <Akonadi/Monitor>

#define DBUS_SERVICE_NAME QLatin1String( "org.kde.pim.LocalFolders" )


using namespace Akonadi;
using namespace OutboxInterface;


/**
 * Private class that helps to provide binary compatibility between releases.
 * @internal
 */
class OutboxInterface::LocalFoldersPrivate
{
  public:
    LocalFoldersPrivate();
    ~LocalFoldersPrivate();

    LocalFolders *instance;
    bool ready;
    bool preparing;
    bool scheduled;
    bool isMainInstance;
    Collection outbox;
    Collection sentMail;
    Collection rootMaildir;
    KJob *outboxJob;
    KJob *sentMailJob;
    QDBusInterface *iface;
    Monitor *monitor;

    /**
      Called when DBus ownership of the service has changed.  This gives us
      the opportunity to register this class as the "main instance" when the
      previous "main instance" has quit.
    */
    void dbusServiceOwnerChanged( const QString &service, const QString &oldOwner,
                                  const QString &newOwner ); // slot

    /**
      If this is the main instance, attempts to create the resource and collections
      if necessary, then fetches them.
      If this is not the main instance, waits for them to be created by the main
      instance, and then fetches them.

      Will emit foldersReady() when done
    */
    void prepare();

    /**
      Schedules a prepare() in 1 second.
      Called when this is not the main instance and we need to wait, or when
      something disappeared and needs to be recreated.
    */
    void schedulePrepare(); // slot

    /**
      Creates the maildir resource, if it is not found.
    */
    void createResourceIfNeeded();

    /**
      Creates the outbox and sent-mail collections, if they are not present.
    */
    void createCollectionsIfNeeded();

    /**
      Creates a Monitor to watch the resource and connects to its signals.
      This is used to watch for evil users deleting the resource / outbox / etc.
    */
    void connectMonitor();

    /**
      Fetches the collections of the maildir resource.
      There is one root collection, which contains the outbox and sent-mail
      collections.
     */
    void fetchCollections();

    // slot called by job creating the resource
    void resourceCreateResult( KJob *job );

    // slot called by job creating a collection
    void collectionCreateResult( KJob *job );

    // slot called by job fetching the collections
    void collectionFetchResult( KJob *job );

};


K_GLOBAL_STATIC( LocalFoldersPrivate, sInstance )


LocalFolders::LocalFolders( LocalFoldersPrivate *dd )
  : QObject()
  , d( dd )
{
}

LocalFolders *LocalFolders::self()
{
  return sInstance->instance;
}

void LocalFolders::fetch()
{
  d->prepare();
}

bool LocalFolders::isReady() const
{
  return d->ready;
}

Collection LocalFolders::outbox() const
{
  Q_ASSERT( d->ready );
  return d->outbox;
}

Collection LocalFolders::sentMail() const
{
  Q_ASSERT( d->ready );
  return d->sentMail;
}



LocalFoldersPrivate::LocalFoldersPrivate()
  : instance( new LocalFolders(this) )
{
  QDBusConnection bus = QDBusConnection::sessionBus();
  isMainInstance = bus.registerService( DBUS_SERVICE_NAME );
  QObject::connect( bus.interface(), SIGNAL(serviceOwnerChanged(QString,QString,QString)),
      instance, SLOT(dbusServiceOwnerChanged(QString,QString,QString)) );

  ready = false;
  // prepare() expects these
  preparing = false;
  outboxJob = 0;
  sentMailJob = 0;
  iface = 0;
  monitor = 0;
  prepare();
}

LocalFoldersPrivate::~LocalFoldersPrivate()
{
  delete instance;
  delete iface;
}

void LocalFoldersPrivate::dbusServiceOwnerChanged( const QString &service,
    const QString &oldOwner, const QString &newOwner )
{
  Q_UNUSED( oldOwner );
  if( !isMainInstance && service == DBUS_SERVICE_NAME && newOwner.isEmpty() ) {
    kDebug() << "Previous main instance released D-Bus service name.";
    isMainInstance = QDBusConnection::sessionBus().registerService( DBUS_SERVICE_NAME );
    if( isMainInstance ) {
      kDebug() << "I have become the main instance.";
    }
  }
}

void LocalFoldersPrivate::prepare()
{
  if( ready ) {
    kDebug() << "Already ready.  Emitting foldersReady().";
    emit instance->foldersReady();
    return;
  }
  if( preparing ) {
    kDebug() << "Already preparing.";
    return;
  }
  kDebug() << "Preparing. isMainInstance" << isMainInstance;
  preparing = true;
  scheduled = false;

  Q_ASSERT( outboxJob == 0 );
  Q_ASSERT( sentMailJob == 0 );
  Q_ASSERT( iface == 0 );
  Q_ASSERT( monitor == 0);

  rootMaildir = Collection( -1 );
  outbox = Collection( -1 );
  sentMail = Collection( -1 );

  createResourceIfNeeded();
}

void LocalFoldersPrivate::schedulePrepare()
{
  if( scheduled ) {
    kDebug() << "Prepare already scheduled.";
    return;
  }

  kDebug() << "Scheduling prepare.";

  if( monitor ) {
    monitor->disconnect( instance );
    monitor->deleteLater();
    monitor = 0;
  }

  ready = false;
  preparing = false;
  scheduled = true;
  QTimer::singleShot( 1000, instance, SLOT( prepare() ) );
}

void LocalFoldersPrivate::createResourceIfNeeded()
{
  Q_ASSERT( preparing );

  // Another instance might have created the resource and updated the config.
  Settings::self()->readConfig();
  kDebug() << "Resource from config:" << Settings::resourceId();

  // check that the maildir resource exists
  AgentInstance resource = AgentManager::self()->instance( Settings::resourceId() );
  if( !resource.isValid() ) {
    if( !isMainInstance ) {
      kDebug() << "Waiting for the main instance to create the resource.";
      schedulePrepare();
    } else {
      kDebug() << "Creating maildir resource.";
      AgentType type = AgentManager::self()->type( "akonadi_maildir_resource" );
      AgentInstanceCreateJob *job = new AgentInstanceCreateJob( type );
      QObject::connect( job, SIGNAL( result( KJob * ) ),
        instance, SLOT( resourceCreateResult( KJob * ) ) );
      // this is not an Akonadi::Job, so we must start it ourselves
      job->start();
    }
  } else {
    connectMonitor();
  }
}

void LocalFoldersPrivate::createCollectionsIfNeeded()
{
  Q_ASSERT( preparing ); // but I may not be the main instance
  Q_ASSERT( rootMaildir.isValid() );

  if( !outbox.isValid() ) {
    kDebug() << "Creating outbox collection.";
    Collection col;
    col.setParent( rootMaildir );
    col.setName( "outbox" );
    col.setContentMimeTypes( QStringList( "message/rfc822" ) );
    Q_ASSERT( outboxJob == 0 );
    outboxJob = new CollectionCreateJob( col );
    QObject::connect( outboxJob, SIGNAL( result( KJob * ) ),
      instance, SLOT( collectionCreateResult( KJob * ) ) );
  }

  if( !sentMail.isValid() ) {
    kDebug() << "Creating sent-mail collection.";
    Collection col;
    col.setParent( rootMaildir );
    col.setName( "sent-mail" );
    col.setContentMimeTypes( QStringList( "message/rfc822" ) );
    Q_ASSERT( sentMailJob == 0 );
    sentMailJob = new CollectionCreateJob( col );
    QObject::connect( sentMailJob, SIGNAL( result( KJob * ) ),
      instance, SLOT( collectionCreateResult( KJob * ) ) );
  }

  if( outboxJob == 0 && sentMailJob == 0 ) {
    // Everything is ready (created and fetched).
    kDebug() << "Local folders ready. resourceId" << Settings::resourceId()
             << "outbox id" << outbox.id() << "sentMail id" << sentMail.id();

    Q_ASSERT( !ready );
    ready = true;
    preparing = false;
    Settings::self()->writeConfig();
    emit instance->foldersReady();
  }
}

void LocalFoldersPrivate::connectMonitor()
{
  Q_ASSERT( preparing ); // but I may not be the main instance
  Q_ASSERT( monitor == 0 );
  monitor = new Monitor( instance );
  monitor->setResourceMonitored( Settings::resourceId().toAscii() );
  QObject::connect( monitor, SIGNAL( collectionRemoved( Akonadi::Collection ) ),
      instance, SLOT( schedulePrepare() ) );
  kDebug() << "Connected monitor.";
  fetchCollections();
}

void LocalFoldersPrivate::fetchCollections()
{
  Q_ASSERT( preparing ); // but I may not be the main instance
  if( iface ) {
    iface->deleteLater();
    iface = 0;
  }
  kDebug() << "Fetching collections in maildir resource.";

  CollectionFetchJob *job = new CollectionFetchJob( Collection::root(), CollectionFetchJob::Recursive );
  job->setResource( Settings::resourceId() ); // limit search
  QObject::connect( job, SIGNAL( result( KJob * ) ),
      instance, SLOT( collectionFetchResult( KJob * ) ) );
}

void LocalFoldersPrivate::resourceCreateResult( KJob *job )
{
  Q_ASSERT( isMainInstance );
  Q_ASSERT( preparing );
  if( job->error() ) {
    kFatal() << "AgentInstanceCreateJob failed to make a maildir resource for us.";
  }

  AgentInstanceCreateJob *createJob = static_cast<AgentInstanceCreateJob *>( job );
  AgentInstance agent = createJob->instance();
  Settings::setResourceId( agent.identifier() );
  kDebug() << "Created maildir resource with id" << Settings::resourceId();

  // configure the resource
  agent.setName( i18n( "Local Mail Folders" ) );
  QDBusInterface conf( "org.freedesktop.Akonadi.Resource." + Settings::resourceId(),
          "/Settings", "org.kde.Akonadi.Maildir.Settings" );
  QDBusReply<void> reply = conf.call( "setPath", KGlobal::dirs()->localxdgdatadir() + "mail" );
  if( !reply.isValid() ) {
    kFatal() << "Failed to set the root maildir.";
  }
  agent.reconfigure();

  // wait for the sync to take place
  // TODO: use ResourceSynchronizeJob from playground/pim/akonaditest/resourcetester
  Q_ASSERT( iface == 0 );
  iface = new QDBusInterface( "org.freedesktop.Akonadi.Resource." + Settings::resourceId(),
      "/", "org.freedesktop.Akonadi.Resource" );
  QObject::connect( iface, SIGNAL( synchronized() ),
      instance, SLOT( connectMonitor() ) );
  reply = iface->call( "synchronize" );
  if( !reply.isValid() ) {
    kFatal() << "Cannot sync the resource via DBus.";
  }

  // NOTE: initially I called synchronizeCollectionTree instead of synchronize,
  // but the problem was, the function is async, so half the time it wouldn't sync
  // in time for fetchCollections().  Calling synchronize allows us to connect
  // to the synchronized signal.
}

void LocalFoldersPrivate::collectionCreateResult( KJob *job )
{
  Q_ASSERT( isMainInstance );
  if( job->error() ) {
    kFatal() << "CollectionCreateJob failed to make a collection for us.";
  }

  CollectionCreateJob *createJob = static_cast<CollectionCreateJob *>( job );
  if( job == outboxJob ) {
    outboxJob = 0;
    outbox = createJob->collection();
    kDebug() << "Created outbox collection with id" << outbox.id();
  } else if( job == sentMailJob ) {
    sentMailJob = 0;
    sentMail = createJob->collection();
    kDebug() << "Created sent-mail collection with id" << sentMail.id();
  } else {
    kFatal() << "Got a result for a job I don't know about.";
  }

  if( outboxJob == 0 && sentMailJob == 0 ) {
    // Done creating.  Refetch everything.
    fetchCollections();
  }
}

void LocalFoldersPrivate::collectionFetchResult( KJob *job )
{
  Q_ASSERT( preparing ); // but I may not be the main instance
  CollectionFetchJob *fetchJob = static_cast<CollectionFetchJob *>( job );
  Collection::List cols = fetchJob->collections();

  kDebug() << "CollectionFetchJob fetched" << cols.count() << "collections.";

  outbox = Collection( -1 );
  sentMail = Collection( -1 );
  Q_FOREACH( const Collection col, cols ) {
    if( col.parent() == Collection::root().id() ) {
      rootMaildir = col;
      kDebug() << "Fetched root maildir collection.";
    } else if( col.name() == "outbox" ) {
      Q_ASSERT( outbox.id() == -1 );
      outbox = col;
      kDebug() << "Fetched outbox collection.";
    } else if( col.name() == "sent-mail" ) {
      Q_ASSERT( sentMail.id() == -1 );
      sentMail = col;
      kDebug() << "Fetched sent-mail collection.";
    } else {
      kWarning() << "Extraneous collection" << col.name() << "with id"
        << col.id() << "found.";
    }
  }

  if( !rootMaildir.isValid() ) {
    kFatal() << "Failed to fetch root maildir collection.";
  }

  createCollectionsIfNeeded();
}


#include "localfolders.moc"
