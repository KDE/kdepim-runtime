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

#include <QApplication>
#include <QObject>

#include <KConfig>
#include <KConfigGroup>
#include <KDebug>
#include <KGlobal>

#include <Akonadi/AgentInstance>
#include <Akonadi/AgentInstanceCreateJob>
#include <Akonadi/AgentManager>
#include <Akonadi/Collection>
#include <Akonadi/CollectionCreateJob>
#include <Akonadi/CollectionFetchJob>


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
    {
      ready = false;
      config = new KConfig( QLatin1String( "localfolders" ) );
      readConfig();
    }
    ~LocalFoldersPrivate()
    {
      delete instance;
      delete config;
    }

    LocalFolders *instance;
    bool waitForOutbox;
    bool waitForSentMail;
    bool ready;
    KConfig *config;
    QString resourceId;
    Entity::Id outboxId;
    Entity::Id sentMailId;
    Collection outbox;
    Collection sentMail;

    /**
      Creates the maildir resource, if it is not found.
    */
    void createResourceIfNeeded();

    /**
      Creates the outbox and sent-mail collections, if they are not present.
    */
    void createCollectionsIfNeeded();

    /**
      Fetches the collections once they have been created or loaded from config.
    */
    void fetchCollections();

    // slot called by job creating the resource
    void resourceCreateResult( KJob *job );

    // slot called by job creating a collection
    void collectionCreateResult( KJob *job );

    // slot called by job fetching a collection
    void collectionFetchResult( KJob *job );

    void readConfig();
    void writeConfig();

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
  // check that the maildir resource exists
  AgentInstance resource = AgentManager::self()->instance( resourceId );
  if( !resource.isValid() ) {
    kDebug() << "creating maildir resource...";
    AgentType type = AgentManager::self()->type( "akonadi_maildir_resource" );
    AgentInstanceCreateJob *job = new AgentInstanceCreateJob( type );
    QObject::connect( job, SIGNAL( result( KJob * ) ),
      instance, SLOT( resourceCreateResult( KJob * ) ) );
    // this is not an Akonadi::Job, so we must start it ourselves
    job->start();
  } else {
    createCollectionsIfNeeded();
  }
}

void LocalFoldersPrivate::createCollectionsIfNeeded()
{
  // FIXME corner case not treated: what if the collections exist but are not
  // owned by the right resource?

  if( outboxId < 0 ) {
    kDebug() << "creating outbox collection...";
    waitForOutbox = true;
    Collection col;
    col.setParent( Collection::root() );
    col.setName( "outbox" );
    col.setContentMimeTypes( QStringList( "message/rfc822" ) );
    col.setResource( resourceId );
    CollectionCreateJob *job = new CollectionCreateJob( col );
    QObject::connect( job, SIGNAL( result( KJob * ) ),
      instance, SLOT( collectionCreateResult( KJob * ) ) );
  } else {
    waitForOutbox = false;
  }

  if( sentMailId < 0 ) {
    kDebug() << "creating sent-mail collection...";
    waitForSentMail = true;
    Collection col;
    col.setParent( Collection::root() );
    col.setName( "sent-mail" );
    col.setContentMimeTypes( QStringList( "message/rfc822" ) );
    col.setResource( resourceId );
    CollectionCreateJob *job = new CollectionCreateJob( col );
    QObject::connect( job, SIGNAL( result( KJob * ) ),
      instance, SLOT( collectionCreateResult( KJob * ) ) );
  } else {
    waitForSentMail = false;
  }

  if( !waitForOutbox && !waitForSentMail ) {
    fetchCollections();
  }
}

void LocalFoldersPrivate::fetchCollections()
{
  kDebug() << "fetching collections...";
  Q_ASSERT( !waitForOutbox && !waitForSentMail );
  waitForOutbox = true;
  waitForSentMail = true;

  CollectionFetchJob *job = new CollectionFetchJob( Collection( outboxId ), CollectionFetchJob::Base );
  QObject::connect( job, SIGNAL( result( KJob * ) ),
    instance, SLOT( collectionFetchResult( KJob * ) ) );

  job = new CollectionFetchJob( Collection( sentMailId ), CollectionFetchJob::Base );
  QObject::connect( job, SIGNAL( result( KJob * ) ),
    instance, SLOT( collectionFetchResult( KJob * ) ) );
}

void LocalFoldersPrivate::resourceCreateResult( KJob *job )
{
  if( job->error() ) {
    kFatal() << "AgentInstanceCreateJob failed to make a maildir resource for us.";
  }

  AgentInstanceCreateJob *createJob = static_cast<AgentInstanceCreateJob *>( job );
  resourceId = createJob->instance().identifier();
  kDebug() << "created maildir resource with id" << resourceId;

  // make sure createCollectionsIfNeeded will know it needs to create these
  outboxId = -1;
  sentMailId = -1;
  createCollectionsIfNeeded();
}

void LocalFoldersPrivate::collectionCreateResult( KJob *job )
{
  if( job->error() ) {
    kFatal() << "CollectionCreateJob failed to make a collection for us.";
  }

  CollectionCreateJob *createJob = static_cast<CollectionCreateJob *>( job );
  Collection col = createJob->collection();
  if( col.name() == "outbox" ) {
    Q_ASSERT( waitForOutbox );
    waitForOutbox = false;
    outboxId = col.id();
    kDebug() << "created outbox collection with id" << outboxId;
  } else if( col.name() == "sent-mail" ) {
    Q_ASSERT( waitForSentMail );
    waitForSentMail = false;
    sentMailId = col.id();
    kDebug() << "created sent-mail collection with id" << sentMailId;
  } else {
    Q_ASSERT(false);
  }

  if( !waitForOutbox && !waitForSentMail ) {
    fetchCollections();
  }
}

void LocalFoldersPrivate::collectionFetchResult( KJob *job )
{
  if( job->error() ) {
    kFatal() << "CollectionFetchJob failed to fetch a collection for us.";
  }

  CollectionFetchJob *fetchJob = static_cast<CollectionFetchJob *>( job );
  Collection::List cols = fetchJob->collections();
  if( cols.count() != 1 ) {
    kFatal() << "FetchJob fetched" << cols.count() << "collections; expected one.";
  }
  Collection col = cols.first();
  if( col.id() == outboxId ) {
    Q_ASSERT( waitForOutbox );
    waitForOutbox = false;
    outbox = col;
    kDebug() << "fetched outbox collection.";
  } else if( col.id() == sentMailId ) {
    Q_ASSERT( waitForSentMail );
    waitForSentMail = false;
    sentMail = col;
    kDebug() << "fetched sent-mail collection.";
  } else {
    Q_ASSERT(false);
  }

  if( !waitForOutbox && !waitForSentMail ) {
    kDebug() << "Local folders ready. resourceId" << resourceId << "outbox id"
             << outboxId << "sentMail id" << sentMailId;

    Q_ASSERT( !ready );
    ready = true;
    writeConfig();
    emit instance->foldersReady();
  }
}


void LocalFoldersPrivate::readConfig()
{
  KConfigGroup group( config, "General" );
  // TODO test these. Entity::Id is a typedef for qint64.
  resourceId = group.readEntry( "resource-id", QString("") );
  outboxId = group.readEntry( "outbox-id", Entity::Id(-1) );
  sentMailId = group.readEntry( "sent-mail-id", Entity::Id(-1) );
  kDebug() << "resource" << resourceId << "outbox" << outboxId
           << "sent-mail" << sentMailId;
  createResourceIfNeeded(); // will emit foldersReady()
}

void LocalFoldersPrivate::writeConfig()
{
  kDebug() << "resource" << resourceId << "outbox" << outboxId
           << "sent-mail" << sentMailId;

  KConfigGroup group( config, "General" );
  group.writeEntry( "resource-id", resourceId );
  group.writeEntry( "outbox-id", outboxId );
  group.writeEntry( "sent-mail-id", sentMailId );
  config->sync();
}


#include "localfolders.moc"
