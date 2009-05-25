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
#include <QDBusInterface>
#include <QDBusReply>
#include <QObject>
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
#include <Akonadi/Control>


using namespace Akonadi;
using namespace OutboxInterface;


/**
 * Private class that helps to provide binary compatibility between releases.
 * @internal
 */
class OutboxInterface::LocalFoldersPrivate
{
  public:
    LocalFoldersPrivate()
      : instance( new LocalFolders(this) )
      , rootMaildir( -1 )
    {
      if( !Akonadi::Control::start() ) {
        kFatal() << "Cannot start Akonadi server.  Quitting.";
      }

      ready = false;
      recreate = false;
      outboxJob = 0;
      sentMailJob = 0;
      iface = 0;

      createResourceIfNeeded(); // will emit foldersReady()
    }
    ~LocalFoldersPrivate()
    {
      delete instance;
      delete iface;
    }

    LocalFolders *instance;
    bool ready;
    bool recreate;
    Entity::Id outboxId;
    Entity::Id sentMailId;
    Collection outbox;
    Collection sentMail;
    Collection rootMaildir;
    KJob *outboxJob;
    KJob *sentMailJob;
    QDBusInterface *iface;

    /**
      Creates the maildir resource, if it is not found.
    */
    void createResourceIfNeeded();

    /**
      Creates the outbox and sent-mail collections, if they are not present.
    */
    void createCollectionsIfNeeded();

    /**
      Fetches the collections of the maildir resource.
      There is one root collection, which contains the outbox and sent-mail
      collections.
     */
    void fetchCollections(); // slot

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


void LocalFoldersPrivate::createResourceIfNeeded()
{
  kDebug() << "Resource" << Settings::resourceId();

  // check that the maildir resource exists
  AgentInstance resource = AgentManager::self()->instance( Settings::resourceId() );
  if( !resource.isValid() ) {
    kDebug() << "Creating maildir resource.";
    AgentType type = AgentManager::self()->type( "akonadi_maildir_resource" );
    AgentInstanceCreateJob *job = new AgentInstanceCreateJob( type );
    QObject::connect( job, SIGNAL( result( KJob * ) ),
      instance, SLOT( resourceCreateResult( KJob * ) ) );
    // this is not an Akonadi::Job, so we must start it ourselves
    job->start();
  } else {
    fetchCollections();
  }
}

void LocalFoldersPrivate::createCollectionsIfNeeded()
{
  Q_ASSERT( rootMaildir.isValid() );

  if( outboxId < 0 ) {
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

  if( sentMailId < 0 ) {
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
             << "outbox id" << outboxId << "sentMail id" << sentMailId;

    Q_ASSERT( !ready );
    ready = true;
    Settings::self()->writeConfig();
    emit instance->foldersReady();
  }
}

void LocalFoldersPrivate::fetchCollections()
{
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
      instance, SLOT( fetchCollections() ) );
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
  if( job->error() ) {
    kFatal() << "CollectionCreateJob failed to make a collection for us.";
  }

  CollectionCreateJob *createJob = static_cast<CollectionCreateJob *>( job );
  Collection col = createJob->collection();
  if( job == outboxJob ) {
    outboxJob = 0;
    outboxId = col.id();
    kDebug() << "created outbox collection with id" << outboxId;
  } else if( job == sentMailJob ) {
    sentMailJob = 0;
    sentMailId = col.id();
    kDebug() << "created sent-mail collection with id" << sentMailId;
  } else {
    kFatal() << "got a result for a job I don't know about.";
  }

  if( outboxJob == 0 && sentMailJob == 0 ) {
    // Done creating.  Refetch everything.
    fetchCollections();
  }
}

void LocalFoldersPrivate::collectionFetchResult( KJob *job )
{
  CollectionFetchJob *fetchJob = static_cast<CollectionFetchJob *>( job );
  Collection::List cols = fetchJob->collections();

  kDebug() << "CollectionFetchJob fetched" << cols.count() << "collections.";

  outboxId = -1;
  sentMailId = -1;
  Q_FOREACH( const Collection col, cols ) {
    if( col.parent() == Collection::root().id() ) {
      rootMaildir = col;
      kDebug() << "Fetched root maildir collection.";
    } else if( col.name() == "outbox" ) {
      Q_ASSERT( outboxId == -1 );
      outbox = col;
      outboxId = col.id();
      kDebug() << "Fetched outbox collection.";
    } else if( col.name() == "sent-mail" ) {
      Q_ASSERT( sentMailId == -1 );
      sentMail = col;
      sentMailId = col.id();
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
