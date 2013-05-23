/*
    Copyright (c) 2007 Tobias Koenig <tokoe@kde.org>
                  2008 Sebastian Trueg <trueg@kde.org>
                  2009 Volker Krause <vkrause@kde.org>
                  2011 Christian Mollekopf <chrigi_1@fastmail.fm>

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

#include "nepomukfeederagent.h"
#include "nepomukfeederagentdialog.h"
#include <aneo.h>

#include <akonadi/agentmanager.h>
#include <akonadi/collectionfetchjob.h>
#include <akonadi/collectionfetchscope.h>
#include <akonadi/entityhiddenattribute.h>
#include <akonadi/indexpolicyattribute.h>
#include <akonadi/item.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/entitydisplayattribute.h>

#include <nepomuk2/simpleresource.h>
#include <nepomuk2/simpleresourcegraph.h>
#include <nepomuk2/datamanagement.h>
#include <nepomuk2/resourcemanager.h>

#include <KLocale>
#include <KUrl>
#include <KProcess>
#include <KStandardDirs>
#include <KIdleTime>
#include <KConfigGroup>
#include <KNotification>
#include <KIconLoader>
#include <KIcon>
#include <KWindowSystem>

#include <QtCore/QTimer>
#include <nepomukfeederutils.h>
#include "pluginloader.h"
#include "nepomukhelpers.h"
#include "findunindexeditemsjob.h"
#include "nepomukcleanerjob.h"
#include "nepomukfeeder-config.h"
#include "nepomukfeederadaptor.h"

typedef QSharedPointer< QMultiHash< Akonadi::Collection::Id,  Akonadi::Item::Id> > MultiHashPointer;
Q_DECLARE_METATYPE(MultiHashPointer)

namespace Akonadi {

static inline bool indexingDisabled( const Collection &collection )
{
  if ( collection.hasAttribute<EntityHiddenAttribute>() )
    return true;

  IndexPolicyAttribute *indexPolicy = collection.attribute<IndexPolicyAttribute>();
  if ( indexPolicy && !indexPolicy->indexingEnabled() )
    return true;

  if ( collection.isVirtual() )
    return true;

  // check if we have a plugin for the stuff in this collection
  foreach ( const QString &mimeType, collection.contentMimeTypes() ) {
    if ( mimeType == Collection::mimeType() )
     continue;
    if ( !FeederPluginloader::instance().feederPluginsForMimeType( mimeType ).isEmpty() )
      return false;
  }

  return true;
}

static inline QString emailMimetype()
{
    return QLatin1String("message/rfc822");
}

NepomukFeederAgent::NepomukFeederAgent(const QString& id) :
  AgentBase(id),
  mInitialUpdateDone( false ),
  mIdleDetectionDisabled( true ),
  mLostChanges( false ),
  mInitialIndexingDisabled( false ),
  mItemBatchCounter( 0 ),
  mBatchDetected( false ),
  mTotalItems(0),
  mIndexedItems(0),
  m_collectionFetchJob(0),
  m_findUnindexedItemsJob(0)
{
  KGlobal::locale()->insertCatalog( "akonadi_nepomukfeeder" ); //TODO do we really need this?

  new NepomukFeederAdaptor( this );

  Nepomuk2::ResourceManager::instance()->init();
  connect( Nepomuk2::ResourceManager::instance(), SIGNAL(nepomukSystemStarted()), SLOT(selfTest()) );
  connect( Nepomuk2::ResourceManager::instance(), SIGNAL(nepomukSystemStopped()), SLOT(selfTest()) );
  connect( this, SIGNAL(reloadConfiguration()), SLOT(selfTest()) );
  connect( this, SIGNAL(fullyIndexed()), this, SLOT(slotFullyIndexed()) );

  connect( KIdleTime::instance(), SIGNAL(timeoutReached(int)), SLOT(systemIdle()) );
  connect( KIdleTime::instance(), SIGNAL(resumingFromIdle()), SLOT(systemResumed()) );

  KConfigGroup cfgGrp( componentData().config(), identifier() );
  KIdleTime::instance()->addIdleTimeout( 1000 * cfgGrp.readEntry( "IdleTimeout", 120 ) );
  disableIdleDetection( cfgGrp.readEntry( "DisableIdleDetection", true ) );
  mInitialIndexingDisabled = cfgGrp.readEntry( "DisableInitialIndexing", false );

  m_indexerConfig = new IndexerConfig( this );
  connect( m_indexerConfig, SIGNAL(configChanged()), this, SLOT(selfTest()) );
  setOnline( false );
  QTimer::singleShot( 0, this, SLOT(selfTest()) );
  QTimer::singleShot( 1000, this, SLOT(checkMigration()) );

  mQueue.setIndexingSpeed( mIdleDetectionDisabled ? FeederQueue::FullSpeed : FeederQueue::ReducedSpeed );

  connect(&mQueue, SIGNAL(progress(int)), SIGNAL(percent(int)));
  connect(&mQueue, SIGNAL(idle(QString)), this, SLOT(emitIdle(QString)));
  connect(&mQueue, SIGNAL(running(QString)), this, SLOT(emitRunning(QString)));
  connect(&mQueue, SIGNAL(fullyIndexed()), this, SIGNAL(fullyIndexed()));
  
  mItemBatchTimer.setSingleShot( true );
  connect( &mItemBatchTimer, SIGNAL(timeout()), SLOT(batchTimerElapsed()) );

  mInitialIndexingTimer.setSingleShot( true );
  connect( &mInitialIndexingTimer, SIGNAL(timeout()), SLOT(checkForLostChanges()) );
  if ( !mInitialIndexingDisabled ) {
    mInitialIndexingTimer.start( 3600 * 1000 );
  } else {
    kDebug() << "Initial indexing was disabled in the configuration.";
  }
}

NepomukFeederAgent::~NepomukFeederAgent()
{

}

void NepomukFeederAgent::configure( WId windowId )
{
  Q_UNUSED( windowId );
  NepomukFeederAgentDialog dlg;
  if(windowId) {
#ifndef Q_WS_WIN
    KWindowSystem::setMainWindow( &dlg, windowId );
#else
    KWindowSystem::setMainWindow( &dlg, (HWND)windowId );
#endif
  }
  dlg.exec();
}

void NepomukFeederAgent::forceReindexCollection(const qlonglong id)
{
  CollectionFetchJob *job = new CollectionFetchJob( Collection(id), Akonadi::CollectionFetchJob::Base, this );
  connect( job, SIGNAL(collectionsReceived(Akonadi::Collection::List)), SLOT(collectionsReceived(Akonadi::Collection::List)));
}

void NepomukFeederAgent::forceReindexItem(const qlonglong id)
{
  mQueue.addItem( Akonadi::Item(id) );
}

void NepomukFeederAgent::processNextNotification()
{
  //check if still running otherwise stop
  if ( isOnline() ) {
    changeProcessed();
  }
}

static int maxItemsWithinTimeframe = 10;
static int timeframeSeconds = 10;
void NepomukFeederAgent::batchTimerElapsed()
{
  if (mItemBatchCounter <= maxItemsWithinTimeframe) {
    kDebug() << "end of batch";
    mBatchDetected = false;
  } else {
    mItemBatchTimer.start( timeframeSeconds * 1000 );
  }
  mItemBatchCounter = 0;
}

bool NepomukFeederAgent::skipBatch(const Item& item)
{
  if ( item.mimeType() != emailMimetype() ) {
    return false;
  }
  mItemBatchCounter++;
  if (!mBatchDetected && mItemBatchCounter > maxItemsWithinTimeframe) {
    kDebug() << "batch detected, skipping";
    mBatchDetected = true;
  }
  
  if (!mItemBatchTimer.isActive()) {
    mItemBatchTimer.start( timeframeSeconds * 1000 );
  }
  return mBatchDetected;
}

void NepomukFeederAgent::itemAdded(const Akonadi::Item& item, const Akonadi::Collection& collection)
{
  if( !isOnline() )
      return;

  kDebug() << item.id();
  if ( indexingDisabled( collection ) )
    return processNextNotification();
  if ( skipBatch( item ) ) {
    kDebug() << "skipping item";
    //Mark that we skipped some changes which we should retrieve again using FindUnindexedItemsJob
    mLostChanges = true;
    return processNextNotification();
  }

  Q_ASSERT( item.parentCollection() == collection );
  mQueue.addItem( item );
  processNextNotification();
}

void NepomukFeederAgent::itemChanged(const Akonadi::Item& item, const QSet< QByteArray >& partIdentifiers)
{
  if( !isOnline() )
      return;

  QSet<QByteArray> parts = partIdentifiers;
  // Remove parts that we don't use in nepomuk, to avoid unnecessary re-indexing
  // We care for FLAGS, and PLD (headers, body), possibly for modification time
  QMutableSetIterator<QByteArray> it( parts );
  while ( it.hasNext() ) {
      const QByteArray part = it.next();
      if ( part == "REMOTEID" || part.startsWith( "ATR:" ) ) {
        it.remove();
      }
  }
  // Nothing to re-index if only the flags have changed, or only the remote ID
  if ( parts.isEmpty() )
    return processNextNotification();
  if ( indexingDisabled( item.parentCollection() ) )
    return processNextNotification();
  if ( skipBatch( item ) ) {
    return processNextNotification();
  }
  //kDebug() << item.id() << partIdentifiers;
  mQueue.addItem( item );
  processNextNotification();
}

void NepomukFeederAgent::itemRemoved(const Akonadi::Item& item)
{
  if( !isOnline() )
      return;

  //kDebug() << item.url();
  Nepomuk2::removeResources( QList <QUrl>() << item.url(), Nepomuk2::RemoveSubResoures );
  processNextNotification();
}

void NepomukFeederAgent::collectionAdded(const Akonadi::Collection& collection, const Akonadi::Collection& parent)
{
  if( !isOnline() )
      return;
  Q_UNUSED( parent );
  if ( indexingDisabled( collection ) )
    return processNextNotification();
  NepomukHelpers::addCollectionToNepomuk( collection );
  processNextNotification();
}

void NepomukFeederAgent::collectionChanged(const Akonadi::Collection& collection, const QSet< QByteArray >& partIdentifiers)
{
  if( !isOnline() )
      return;

  QSet<QByteArray> parts = partIdentifiers;
  QMutableSetIterator<QByteArray> it( parts );
  while ( it.hasNext() ) {
      const QByteArray part = it.next();
      if ( part == "REMOTEID" ) {
        it.remove();
      }
  }
  if ( parts.isEmpty() )
    return processNextNotification();
  if ( indexingDisabled( collection ) )
    return processNextNotification();
  NepomukHelpers::addCollectionToNepomuk( collection );
  processNextNotification();
}

void NepomukFeederAgent::collectionRemoved(const Akonadi::Collection& collection)
{
  if( !isOnline() )
      return;

  Nepomuk2::removeResources( QList <QUrl>() << collection.url() );
  processNextNotification();
}

void NepomukFeederAgent::findUnindexed()
{
  if ( !Nepomuk2::ResourceManager::instance()->initialized() ) {
    kWarning() << "Nepomuk is not ready, we can't find unindexed items";
    return;
  }

  if ( m_collectionFetchJob )
      return;

  m_collectionFetchJob = new Akonadi::CollectionFetchJob(Akonadi::Collection::root(),
                                                         Akonadi::CollectionFetchJob::Recursive,
                                                         this);
  connect(m_collectionFetchJob, SIGNAL(finished(KJob*)), this, SLOT(collectionListReceived(KJob*)));
  m_collectionFetchJob->start();
}

void NepomukFeederAgent::collectionListReceived(KJob *job)
{
  if (job->error()) {
    kWarning() << "Failed to fetch collections";
    return;
  }

  Akonadi::Collection::List allCollections = m_collectionFetchJob->collections();
  m_collectionFetchJob = 0;

  Akonadi::Collection::List indexedCollections;
  foreach (const Akonadi::Collection &col, allCollections) {
    if (!indexingDisabled(col)) {
      indexedCollections << col;
    }
  }

  m_findUnindexedItemsJob = new FindUnindexedItemsJob(NEPOMUK_FEEDER_INDEX_COMPAT_LEVEL, this);
  m_findUnindexedItemsJob->setIndexedCollections( indexedCollections );

  connect(m_findUnindexedItemsJob, SIGNAL(result(KJob*)), this, SLOT(foundUnindexedItems(KJob*)));
  m_findUnindexedItemsJob->start();
}

void NepomukFeederAgent::foundUnindexedItems(KJob* job)
{
  if (job->error()) {
    kWarning() << "FindUnindexedItemsJob failed";
    return;
  }
  FindUnindexedItemsJob *findJob = static_cast<FindUnindexedItemsJob*>(job);
  Q_ASSERT( findJob == m_findUnindexedItemsJob );

  mTotalItems = m_findUnindexedItemsJob->totalCount();
  mIndexedItems = findJob->indexedCount();
  const FindUnindexedItemsJob::ItemHash items = findJob->getUnindexed();
  m_findUnindexedItemsJob = 0;

  FindUnindexedItemsJob::ItemHash::const_iterator it = items.constBegin();
  for (;it != items.constEnd(); it++) {
    Akonadi::Item item( it.key() );
    item.setMimeType( it.value().second );
    mQueue.addLowPrioItem( item );
  }

  NepomukCleanerJob *cleanerJob = new NepomukCleanerJob(findJob->staleUris(), this);
  cleanerJob->start();
}

void NepomukFeederAgent::checkForLostChanges()
{
  //Check every hour if we should pick up some unindexed new emails, if currently busy come back a bit later
  //Since we only skip emails, we don't re-run the full check regulary.
  if ( isOnline() && mLostChanges && mQueue.isEmpty() ) {
    findUnindexed();
    mLostChanges = false;
    mInitialIndexingTimer.start( 1800 * 1000 );
  } else {
    mInitialIndexingTimer.start( 60 * 1000 );
  }
}

void NepomukFeederAgent::updateAll()
{
  findUnindexed();
}

void NepomukFeederAgent::collectionsReceived(const Akonadi::Collection::List& collections)
{
  foreach ( const Collection &collection, collections ) {
    mQueue.addCollection( collection );
  }
}

void NepomukFeederAgent::checkMigration()
{
  kDebug();

  // Cleanup agentsrc after migration to 4.9
  AgentManager* agentManager = AgentManager::self();
  const AgentInstance::List allAgents = agentManager->instances();
  const QStringList oldFeeders = QStringList() << "akonadi_nepomuk_email_feeder" << "akonadi_nepomuk_contact_feeder" << "akonadi_nepomuk_calendar_feeder";
  // Cannot use agentManager->instance(oldInstanceName) here, it wouldn't find broken instances.
  Q_FOREACH( const AgentInstance& inst, allAgents ) {
    if ( oldFeeders.contains( inst.identifier() ) ) {
      kDebug() << "Removing old nepomuk feeder" << inst.identifier();
      agentManager->removeInstance( inst );
    }
  }
}

void NepomukFeederAgent::selfTest()
{
    if( !m_indexerConfig->isEnabled() ) {
        setOnline( false );
        emit status( Broken, i18n( "Email Indexing has been disabled" ) );
        return;
    }

    if( !Nepomuk2::ResourceManager::instance()->initialized() ) {
        setOnline( false );
        emit status( Broken, i18n( "Waiting for the Nepomuk to start" ) );
    }
    else {
        setOnline( true );
        emit status( Idle, i18n( "Ready to index data." ) );

        if ( !mInitialUpdateDone ) {
            mInitialUpdateDone = true;
            //TODO postpone this until the computer is idle?
            if ( !mInitialIndexingDisabled ) {
                QTimer::singleShot( 0, this, SLOT(updateAll()) );
            } else {
                kDebug() << "Initial indexing was disabled in the configuration.";
            }
        }
    }
}

void NepomukFeederAgent::disableIdleDetection( bool value )
{
  mIdleDetectionDisabled = value;
  if ( value ) {
    mQueue.setIndexingSpeed( FeederQueue::FullSpeed );
  }
  if ( KIdleTime::instance()->idleTime() ) {
    systemIdle();
  } else {
    systemResumed();
  }
}

void NepomukFeederAgent::slotFullyIndexed()
{
}

void NepomukFeederAgent::doSetOnline(bool online)
{
    if( !online ) {
        if( m_collectionFetchJob ) {
            m_collectionFetchJob->kill();
            m_collectionFetchJob = 0;
        }
        if( m_findUnindexedItemsJob ) {
            m_findUnindexedItemsJob->kill();
            m_findUnindexedItemsJob = 0;
        }
    }

  setRunning( online );
  Akonadi::AgentBase::doSetOnline( online );
}

void NepomukFeederAgent::setRunning( bool running )
{
  mQueue.setOnline( running );
  if ( running ) {
    findUnindexed();
    if ( mQueue.currentCollection().isValid() ) {
      const QString summary = i18n( "Indexing collection '%1'...", mQueue.currentCollection().name() );
      const QPixmap pixmap = KIcon( "nepomuk" ).pixmap( KIconLoader::SizeSmall, KIconLoader::SizeSmall );
      KNotification::event( QLatin1String("startindexingcollection"),
                              summary,
                              pixmap,
                              0,
                              KNotification::CloseOnTimeout,
                              KGlobal::mainComponent());
      emit status( AgentBase::Running, summary );
    }
    else
      emit status( AgentBase::Running, i18n( "Indexing recent changes..." ) );
  }
}

void NepomukFeederAgent::systemIdle()
{
  if ( mIdleDetectionDisabled || !isOnline() )
    return;

  KIdleTime::instance()->catchNextResumeEvent();
  mQueue.setIndexingSpeed( FeederQueue::FullSpeed );
}

void NepomukFeederAgent::systemResumed()
{
  if ( mIdleDetectionDisabled || !isOnline() )
    return;

  mQueue.setIndexingSpeed( FeederQueue::ReducedSpeed );
}

void NepomukFeederAgent::emitIdle(const QString &string)
{
  emit status( AgentBase::Idle, string );
}

void NepomukFeederAgent::emitRunning(const QString &string)
{
  emit status( AgentBase::Running, string );
}

bool NepomukFeederAgent::isDisableIdleDetection() const
{
  return mIdleDetectionDisabled;
}

bool NepomukFeederAgent::queueIsEmpty()
{
  return mQueue.isEmpty();
}

QString NepomukFeederAgent::currentCollectionName()
{
  if(queueIsEmpty()) {
    return QString();
  } else {
    return mQueue.currentCollection().name();
  }
}

QStringList NepomukFeederAgent::listOfCollection() const
{
  QStringList names;
  Akonadi::Collection::List listQueueCollection = mQueue.listOfCollection();
  Q_FOREACH(const Akonadi::Collection& collection, listQueueCollection) {
    names << collection.name();
  }
  return names;
}

qlonglong NepomukFeederAgent::totalitems() const
{
  return mTotalItems;
}

qlonglong NepomukFeederAgent::indexeditems() const
{
  return mIndexedItems;
}

bool NepomukFeederAgent::isIndexing() const
{
  return (status() != Idle);
}

}

AKONADI_AGENT_MAIN( Akonadi::NepomukFeederAgent )


#include "nepomukfeederagent.moc"
