/*
    Copyright (c) 2007 Tobias Koenig <tokoe@kde.org>
                  2008 Sebastian Trueg <trueg@kde.org>
                  2009 Volker Krause <vkrause@kde.org>
                  2011 Christian Mollekopf <chrigi_1@fastmail.fm>
                  2013 Vishesh Handa <me@vhanda.in>

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
#include "nepomukhelpers.h"
#include "findunindexeditemsjob.h"
#include "nepomukfeeder-config.h"
#include "nepomukfeederadaptor.h"

#include <aneo.h>

#include <akonadi/agentmanager.h>
#include <akonadi/collectionfetchjob.h>
#include <akonadi/collectionfetchscope.h>
#include <akonadi/item.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/entitydisplayattribute.h>
#include <akonadi/changerecorder.h>

#include <nepomuk2/resourcemanager.h>

#include <KLocale>
#include <KUrl>
#include <KProcess>
#include <KStandardDirs>
#include <KConfigGroup>
#include <KWindowSystem>

#include <QtCore/QTimer>
#include <nepomukfeederutils.h>

typedef QSharedPointer< QMultiHash< Akonadi::Collection::Id,  Akonadi::Item::Id> > MultiHashPointer;
Q_DECLARE_METATYPE(MultiHashPointer)

namespace Akonadi {

static inline QString emailMimetype()
{
    return QLatin1String("message/rfc822");
}

NepomukFeederAgent::NepomukFeederAgent(const QString& id) :
  AgentBase(id),
  mInitialUpdateDone( false ),
  mInitialIndexingDisabled( false ),
  mTotalItems(0),
  mIndexedItems(0),
  m_findUnindexedItemsJob(0)
{
  KGlobal::locale()->insertCatalog( "akonadi_nepomukfeeder" ); //TODO do we really need this?

  new NepomukFeederAdaptor( this );

  connect( Nepomuk2::ResourceManager::instance(), SIGNAL(nepomukSystemStarted()), SLOT(selfTest()) );
  connect( Nepomuk2::ResourceManager::instance(), SIGNAL(nepomukSystemStopped()), SLOT(selfTest()) );
  connect( this, SIGNAL(reloadConfiguration()), SLOT(selfTest()) );

  //mInitialIndexingDisabled = cfgGrp.readEntry( "DisableInitialIndexing", false );

  m_indexerConfig = new IndexerConfig( this );
  connect( m_indexerConfig, SIGNAL(configChanged()), this, SLOT(selfTest()) );

  setOnline( false );
  QTimer::singleShot( 0, this, SLOT(selfTest()) );
  QTimer::singleShot( 1000, this, SLOT(checkMigration()) );

  //mScheduler.setIndexingSpeed( mIdleDetectionDisabled ? IndexScheduler::FullSpeed : IndexScheduler::ReducedSpeed );

  connect(&mScheduler, SIGNAL(progress(int)), SIGNAL(percent(int)));
  connect(&mScheduler, SIGNAL(idle(QString)), this, SLOT(emitIdle(QString)));
  connect(&mScheduler, SIGNAL(running(QString)), this, SLOT(emitRunning(QString)));

  changeRecorder()->fetchCollection( true );
  changeRecorder()->itemFetchScope().setAncestorRetrieval( ItemFetchScope::Parent );
  changeRecorder()->setAllMonitored( true );
  changeRecorder()->itemFetchScope().setCacheOnly( true );
  changeRecorder()->setChangeRecordingEnabled( false );
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
  mScheduler.addItem( Akonadi::Item(id) );
}

void NepomukFeederAgent::processNextNotification()
{
  //check if still running otherwise stop
  if ( isOnline() ) {
    changeProcessed();
  }
}

void NepomukFeederAgent::itemAdded(const Akonadi::Item& item, const Akonadi::Collection& collection)
{
  if( !isOnline() )
      return;

  kDebug() << item.id();
  if ( indexingDisabled( collection ) )
    return processNextNotification();

  Q_ASSERT( item.parentCollection() == collection );
  mScheduler.addItem( item );
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

  //kDebug() << item.id() << partIdentifiers;
  mScheduler.addItem( item );
  processNextNotification();
}

void NepomukFeederAgent::itemRemoved(const Akonadi::Item& item)
{
  if( !isOnline() )
      return;

  //kDebug() << item.url();
  mScheduler.removeItem( item );
  processNextNotification();
}

void NepomukFeederAgent::collectionAdded(const Akonadi::Collection& collection, const Akonadi::Collection& parent)
{
  if( !isOnline() )
      return;
  Q_UNUSED( parent );
  if ( indexingDisabled( collection ) )
    return processNextNotification();

  mScheduler.addCollection( collection );
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

  mScheduler.removeCollection( collection );
  processNextNotification();
}

void NepomukFeederAgent::findUnindexed()
{
  if ( !Nepomuk2::ResourceManager::instance()->initialized() ) {
    kWarning() << "Nepomuk is not ready, we can't find unindexed items";
    return;
  }

  if (m_findUnindexedItemsJob) {
      return;
  }

  m_findUnindexedItemsJob = new FindUnindexedItemsJob(NEPOMUK_FEEDER_INDEX_COMPAT_LEVEL, this);

  connect(m_findUnindexedItemsJob, SIGNAL(result(KJob*)), this, SLOT(foundUnindexedItems(KJob*)));
  m_findUnindexedItemsJob->start();

  emit status( Running, i18n( "Calculating Emails to index" ) );
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
    mScheduler.addLowPrioItem( item );
  }

  mScheduler.removeNepomukUris( findJob->staleUris() );
}

void NepomukFeederAgent::updateAll()
{
  findUnindexed();
}

void NepomukFeederAgent::collectionsReceived(const Akonadi::Collection::List& collections)
{
  foreach ( const Collection &collection, collections ) {
    mScheduler.addCollection( collection );
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

void NepomukFeederAgent::doSetOnline(bool online)
{
    mScheduler.setOnline( online );

    if ( online ) {
        findUnindexed();
    }
    else {
        if( m_findUnindexedItemsJob ) {
            m_findUnindexedItemsJob->kill();
            m_findUnindexedItemsJob = 0;
        }

        mScheduler.clear();
    }

    Akonadi::AgentBase::doSetOnline( online );
}

void NepomukFeederAgent::emitIdle(const QString &string)
{
  emit status( AgentBase::Idle, string );
}

void NepomukFeederAgent::emitRunning(const QString &string)
{
  emit status( AgentBase::Running, string );
}

bool NepomukFeederAgent::queueIsEmpty()
{
  return mScheduler.isEmpty();
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
