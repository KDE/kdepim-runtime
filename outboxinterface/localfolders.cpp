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

#include <KConfig>
#include <KConfigGroup>
#include <KDebug>

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
class LocalFolders::Private
{
  public:
    Private( LocalFolders *parent )
      : q( parent )
    {
    }
    ~Private()
    {
      delete config;
    }

    LocalFolders * const q;
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
};

void LocalFolders::Private::createResourceIfNeeded()
{
  // check that the maildir resource exists
  AgentInstance resource = AgentManager::self()->instance( resourceId );
  if( !resource.isValid() ) {
    kDebug() << "creating maildir resource...";
    AgentType type = AgentManager::self()->type( "akonadi_maildir_resource" );
    AgentInstanceCreateJob *job = new AgentInstanceCreateJob( type );
    connect( job, SIGNAL( result( KJob * ) ),
             q, SLOT( resourceCreateResult( KJob * ) ) );
    // this is not an Akonadi::Job, so we must start it ourselves
    job->start();
  } else {
    createCollectionsIfNeeded();
  }
}

void LocalFolders::Private::createCollectionsIfNeeded()
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
    connect( job, SIGNAL( result( KJob * ) ),
             q, SLOT( collectionCreateResult( KJob * ) ) );
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
    connect( job, SIGNAL( result( KJob * ) ),
             q, SLOT( collectionCreateResult( KJob * ) ) );
  } else {
    waitForSentMail = false;
  }

  if( !waitForOutbox && !waitForSentMail ) {
    fetchCollections();
  }
}

void LocalFolders::Private::fetchCollections()
{
  kDebug() << "fetching collections...";
  Q_ASSERT( !waitForOutbox && !waitForSentMail );
  waitForOutbox = true;
  waitForSentMail = true;

  CollectionFetchJob *job = new CollectionFetchJob( Collection( outboxId ), CollectionFetchJob::Base );
  connect( job, SIGNAL( result( KJob * ) ),
           q, SLOT( collectionFetchResult( KJob * ) ) );

  job = new CollectionFetchJob( Collection( sentMailId ), CollectionFetchJob::Base );
  connect( job, SIGNAL( result( KJob * ) ),
           q, SLOT( collectionFetchResult( KJob * ) ) );
}

void LocalFolders::Private::resourceCreateResult( KJob *job )
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

void LocalFolders::Private::collectionCreateResult( KJob *job )
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

void LocalFolders::Private::collectionFetchResult( KJob *job )
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
    q->writeConfig();
    emit q->foldersReady();
  }
}


class StaticLocalFolders : public LocalFolders
{
  public:
    StaticLocalFolders() : LocalFolders() {}
};

StaticLocalFolders *sSelf = 0;

static void destroyStaticLocalFolders() {
  delete sSelf;
}

LocalFolders::LocalFolders()
  : QObject()
  , d( new Private( this ) )
{
  kDebug() << "LocalFolders constructor";

  // KGlobal::locale()->insertCatalog( QLatin1String( "libmailtransport" ) );
  qAddPostRoutine( destroyStaticLocalFolders );
  d->ready = false;
  d->config = new KConfig( QLatin1String( "localfolders" ) );
}

LocalFolders::~LocalFolders()
{
  kDebug() << "LocalFolders destructor";

  qRemovePostRoutine( destroyStaticLocalFolders );
  delete d;
}

LocalFolders *LocalFolders::self()
{
  if ( !sSelf ) {
    // TODO why not just sSelf = new LocalFolders???
    sSelf = new StaticLocalFolders;
    sSelf->readConfig();
  }
  return sSelf;
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

void LocalFolders::readConfig()
{
  KConfigGroup group( d->config, "General" );
  // TODO test these. Entity::Id is a typedef for qint64.
  d->resourceId = group.readEntry( "resource-id", QString("") );
  d->outboxId = group.readEntry( "outbox-id", Entity::Id(-1) );
  d->sentMailId = group.readEntry( "sent-mail-id", Entity::Id(-1) );
  kDebug() << "resource" << d->resourceId << "outbox" << d->outboxId
           << "sent-mail" << d->sentMailId;
  d->createResourceIfNeeded(); // will emit foldersReady()
}

void LocalFolders::writeConfig()
{
  kDebug() << "resource" << d->resourceId << "outbox" << d->outboxId
           << "sent-mail" << d->sentMailId;

  KConfigGroup group( d->config, "General" );
  group.writeEntry( "resource-id", d->resourceId );
  group.writeEntry( "outbox-id", d->outboxId );
  group.writeEntry( "sent-mail-id", d->sentMailId );
  d->config->sync();
}


#include "localfolders.moc"
