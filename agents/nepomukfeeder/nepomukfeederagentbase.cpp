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

#include "nepomukfeederagentbase.h"
#include <aneo.h>

#include <akonadi/changerecorder.h>
#include <akonadi/collectionfetchjob.h>
#include <akonadi/collectionfetchscope.h>
#include <akonadi/entityhiddenattribute.h>
#include <akonadi/indexpolicyattribute.h>
#include <akonadi/item.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/mimetypechecker.h>
#include <akonadi/entitydisplayattribute.h>
#include <akonadi/entityhiddenattribute.h>

#include <nepomuk/simpleresource.h>
#include <nepomuk/simpleresourcegraph.h>
#include <nepomuk/tag.h>

#include <KLocale>
#include <KUrl>
#include <KProcess>
#include <KMessageBox>
#include <KStandardDirs>
#include <KIdleTime>
#include <KNotification>

#include <Soprano/Vocabulary/NAO>
#include <Soprano/Vocabulary/RDF>

#include <strigi/analyzerconfiguration.h>
#include <strigi/analysisresult.h>
#include <strigi/indexpluginloader.h>
#include <strigi/indexmanager.h>
#include <strigi/indexwriter.h>
#include <strigi/streamanalyzer.h>
#include <strigi/stringstream.h>

#include <QtCore/QTimer>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusReply>
#include <QtDBus/QDBusConnectionInterface>
#include <QtDBus/QDBusInterface>
#include <KConfigGroup>
#include "nepomukfeederutils.h"

using namespace Akonadi;

static inline bool indexingDisabled( const Collection &collection )
{
  if ( collection.hasAttribute<EntityHiddenAttribute>() )
    return true;

  IndexPolicyAttribute *indexPolicy = collection.attribute<IndexPolicyAttribute>();
  if ( indexPolicy && !indexPolicy->indexingEnabled() )
    return true;

  return false;
}

NepomukFeederAgentBase::NepomukFeederAgentBase(const QString& id) :
  AgentBase(id),
  mTotalAmount( 0 ),
  mProcessedAmount( 0 ),
  mPendingJobs( 0 ),
  mPendingRemoveDataJobs( 0 ),
  mResourceGraph( 0 ),
  mStrigiIndexManager( 0 ),
  mIndexCompatLevel( 2 ),
  mNepomukStartupAttempted( false ),
  mInitialUpdateDone( false ),
  mNeedsStrigi( false ),
  mSelfTestPassed( false ),
  mSystemIsIdle( false ),
  mIdleDetectionDisabled( false ),
  mReIndex( false )
{
  // initialize Nepomuk
  Nepomuk::ResourceManager::instance()->init();

  changeRecorder()->fetchCollection( true );
  changeRecorder()->itemFetchScope().setAncestorRetrieval( ItemFetchScope::Parent );

  mNepomukStartupTimeout.setInterval( 300 * 1000 );
  mNepomukStartupTimeout.setSingleShot( true );
  connect( &mNepomukStartupTimeout, SIGNAL(timeout()), SLOT(selfTest()) );
  connect( Nepomuk::ResourceManager::instance(), SIGNAL(nepomukSystemStarted()), SLOT(selfTest()) );
  connect( Nepomuk::ResourceManager::instance(), SIGNAL(nepomukSystemStopped()), SLOT(selfTest()) );
  connect( this, SIGNAL(fullyIndexed()), this, SLOT(slotFullyIndexed()) );

  connect( KIdleTime::instance(), SIGNAL(timeoutReached(int)), SLOT(systemIdle()) );
  connect( KIdleTime::instance(), SIGNAL(resumingFromIdle()), SLOT(systemResumed()) );
  KIdleTime::instance()->addIdleTimeout( 10 * 1000 );

  checkOnline();
  QTimer::singleShot( 0, this, SLOT(selfTest()) );

  mProcessPipelineTimer.setInterval( 0 );
  mProcessPipelineTimer.setSingleShot( true );
  connect( &mProcessPipelineTimer, SIGNAL(timeout()), SLOT(processPipeline()) );
}

NepomukFeederAgentBase::~NepomukFeederAgentBase()
{
  if (mResourceGraph) {
      delete mResourceGraph;
  }
  if ( mStrigiIndexManager )
    Strigi::IndexPluginLoader::deleteIndexManager( mStrigiIndexManager );
}

void NepomukFeederAgentBase::setParentCollection( const Akonadi::Entity &entity, Nepomuk::SimpleResource& res, Nepomuk::SimpleResourceGraph& graph )
{
  if ( entity.parentCollection().isValid() && entity.parentCollection() != Akonadi::Collection::root() ) {
    Nepomuk::SimpleResource parentResource( entity.parentCollection().url() );
    parentResource.setTypes(QList <QUrl>() << Vocabulary::ANEO::AkonadiDataObject() << Vocabulary::NIE::InformationElement());
    graph << parentResource; //To use the nie::isPartOf relation both parent and child must be in the graph
    res.setProperty( Vocabulary::NIE::isPartOf(), parentResource );
  }
}

void NepomukFeederAgentBase::updateCollection(const Akonadi::Collection& collection, Nepomuk::SimpleResource &r, Nepomuk::SimpleResourceGraph &graph)
{
  if ( collection.hasAttribute<Akonadi::EntityHiddenAttribute>() )
    return;

  const Akonadi::EntityDisplayAttribute *attr = collection.attribute<Akonadi::EntityDisplayAttribute>();

  if ( attr && !attr->displayName().isEmpty() ) {
    r.setProperty( Soprano::Vocabulary::NAO::prefLabel(), attr->displayName() );
  } else {
    r.setProperty( Soprano::Vocabulary::NAO::prefLabel(), collection.name() );
  }
  if ( attr && !attr->iconName().isEmpty() )
    NepomukFeederUtils::setIcon( attr->iconName(), r, graph );
}

void NepomukFeederAgentBase::addCollectionToNepomuk( const Akonadi::Collection &collection ) 
{
  //kWarning() << collection.url();
  Nepomuk::SimpleResourceGraph graph;
  Nepomuk::SimpleResource res( collection.url() );
  res.setTypes(QList <QUrl>() << Vocabulary::ANEO::AkonadiDataObject() << Vocabulary::NIE::InformationElement());
  res.setProperty( Vocabulary::NIE::url(), collection.url() );
  setParentCollection( collection, res, graph);
  updateCollection( collection, res, graph );
  graph.insert(res);
  /*kWarning() << "--------------------------------";
  foreach( const Nepomuk::SimpleResource &res, graph.toList() ) {
      kWarning() << res.property(Soprano::Vocabulary::RDF::type());
  }
  kWarning() << "--------------------------------";*/
  QHash <QUrl, QVariant> additionalMetadata;
  additionalMetadata.insert(Soprano::Vocabulary::RDF::type(), Soprano::Vocabulary::NRL::DiscardableInstanceBase());
  //We overwrite properties, as the resource is not removed on update, and there are not subproperties on collections (if there were, we would remove them first too)
  KJob *job = Nepomuk::storeResources(graph, Nepomuk::IdentifyNew, Nepomuk::OverwriteProperties, additionalMetadata, KGlobal::mainComponent());
  connect( job, SIGNAL( result( KJob* ) ), SLOT( jobResult( KJob* ) ) );
}

void NepomukFeederAgentBase::addItemToGraph( const Akonadi::Item &item, Nepomuk::SimpleResourceGraph &graph ) 
{
  //kWarning() << item.url();
  Nepomuk::SimpleResource res( item.url() );
  res.setTypes(QList <QUrl>() << Vocabulary::ANEO::AkonadiDataObject() << Vocabulary::NIE::InformationElement());
  res.setProperty( Vocabulary::NIE::url(), QUrl(item.url()) );
  Q_ASSERT(res.property(Vocabulary::NIE::url()).first().toUrl() == QUrl(item.url()));
  res.setProperty( Vocabulary::ANEO::akonadiItemId(), QString::number( item.id() ) );
  setParentCollection( item, res, graph);
  updateItem(item, res, graph);
  graph << res;
}

void NepomukFeederAgentBase::addGraphToNepomuk( const Nepomuk::SimpleResourceGraph &graph ) 
{
  /*kWarning() << "--------------------------------";
  foreach( const Nepomuk::SimpleResource &res, mResourceGraph->toList() ) {
    if (res.contains(Vocabulary::NIE::url())) {
        Q_ASSERT(res.property(Vocabulary::NIE::url()).size() == 1);
        kWarning() << res.property(Vocabulary::NIE::url()).first().toUrl();
        kWarning() << res.property(Soprano::Vocabulary::NAO::prefLabel());
    }
  }
  kWarning() << "--------------------------------";*/
  QHash <QUrl, QVariant> additionalMetadata;
  additionalMetadata.insert(Soprano::Vocabulary::RDF::type(), Soprano::Vocabulary::NRL::DiscardableInstanceBase());
  //FIXME sometimes there are warning about the cardinality, maybe the old values are not always removed before the new ones are (although there is no failing removejob)?
  KJob *job = Nepomuk::storeResources(graph, Nepomuk::IdentifyNew, Nepomuk::NoStoreResourcesFlags, additionalMetadata, KGlobal::mainComponent());
  connect( job, SIGNAL( result( KJob* ) ), SLOT( jobResult( KJob* ) ) );
}

void NepomukFeederAgentBase::itemAdded(const Akonadi::Item& item, const Akonadi::Collection& collection)
{
  if ( indexingDisabled( collection ) )
    return;

  if ( item.hasPayload() ) {
    mItemPipeline.enqueue( item );
    mProcessPipelineTimer.start();
  } else {
    const ItemFetchScope scope = fetchScopeForCollection( collection );
    if ( scope.fullPayload() || !scope.payloadParts().isEmpty() ) {
      ItemFetchJob *job = new ItemFetchJob( item );
      job->setFetchScope( scope );
      connect( job, SIGNAL( itemsReceived( Akonadi::Item::List ) ),
               SLOT( notificationItemsReceived( Akonadi::Item::List ) ) );
    }
  }
}

void NepomukFeederAgentBase::itemChanged(const Akonadi::Item& item, const QSet< QByteArray >& partIdentifiers)
{
  Q_UNUSED( partIdentifiers );
  if ( indexingDisabled( item.parentCollection() ) )
    return;
  // TODO: check part identfiers if anything interesting changed at all
  if ( item.hasPayload() ) {
    mItemPipeline.enqueue( item );
    mProcessPipelineTimer.start();
  } else {
    const Collection collection = item.parentCollection();
    const ItemFetchScope scope = fetchScopeForCollection( collection );
    if ( scope.fullPayload() || !scope.payloadParts().isEmpty() ) {
      ItemFetchJob *job = new ItemFetchJob( item );
      job->setFetchScope( scope );
      connect( job, SIGNAL( itemsReceived( Akonadi::Item::List ) ),
               SLOT( notificationItemsReceived( Akonadi::Item::List ) ) );
    }
  }
}

void NepomukFeederAgentBase::itemRemoved(const Akonadi::Item& item)
{
  Nepomuk::removeResources(QList <QUrl>() << item.url());
}

void NepomukFeederAgentBase::collectionAdded(const Akonadi::Collection& collection, const Akonadi::Collection& parent)
{
  Q_UNUSED( parent );
  if ( indexingDisabled( collection ) )
    return;
  addCollectionToNepomuk(collection);
}

void NepomukFeederAgentBase::collectionChanged(const Akonadi::Collection& collection, const QSet< QByteArray >& partIdentifiers)
{
  Q_UNUSED( partIdentifiers );
  if ( indexingDisabled( collection ) )
    return;
  addCollectionToNepomuk(collection);
}

void NepomukFeederAgentBase::collectionRemoved(const Akonadi::Collection& collection)
{
  Nepomuk::removeResources(QList <QUrl>() << collection.url());
}

void NepomukFeederAgentBase::addSupportedMimeType( const QString &mimeType )
{
  mSupportedMimeTypes.append( mimeType );
  changeRecorder()->setMimeTypeMonitored( mimeType );
  mMimeTypeChecker.setWantedMimeTypes( mSupportedMimeTypes );
}

void NepomukFeederAgentBase::updateAll()
{
  mReIndex = true;
  CollectionFetchJob *collectionFetch = new CollectionFetchJob( Collection::root(), CollectionFetchJob::Recursive, this );
  collectionFetch->fetchScope().setContentMimeTypes( mSupportedMimeTypes );
  connect( collectionFetch, SIGNAL(collectionsReceived(Akonadi::Collection::List)), SLOT(collectionsReceived(Akonadi::Collection::List)) );
}

void NepomukFeederAgentBase::collectionsReceived(const Akonadi::Collection::List& collections)
{
  mMimeTypeChecker.setWantedMimeTypes( mSupportedMimeTypes );
  foreach( const Collection &collection, collections ) {
    if ( !mMimeTypeChecker.isWantedCollection( collection ) )
      continue;

    if ( indexingDisabled( collection ) )
      continue;

    mCollectionQueue.append( collection );
  }

  if ( mPendingJobs == 0 ) {
    processNextCollection();
  }
}

void NepomukFeederAgentBase::processNextCollection()
{
  if ( mCurrentCollection.isValid() )
    return;
  mTotalAmount = 0;
  if ( mCollectionQueue.isEmpty() ) {
    //kWarning() << "fully indexed";
    mReIndex = false;
    emit fullyIndexed();
    return;
  }
  mCurrentCollection = mCollectionQueue.takeFirst();
  emit status( AgentBase::Running, i18n( "Indexing collection '%1'...", mCurrentCollection.name() ) );
  kDebug() << "Indexing collection" << mCurrentCollection.name();
  //TODO maybe reindex anyways to be sure that type etc is correct
  if ( !Nepomuk::ResourceManager::instance()->mainModel()->containsAnyStatement( Soprano::Node(), Vocabulary::NIE::url(), mCurrentCollection.url() ) ) {
    addCollectionToNepomuk(mCurrentCollection);
  }

  ItemFetchJob *itemFetch = new ItemFetchJob( mCurrentCollection, this );
  itemFetch->fetchScope().setCacheOnly( true );
  connect( itemFetch, SIGNAL(itemsReceived(Akonadi::Item::List)), SLOT(itemHeadersReceived(Akonadi::Item::List)) );
  connect( itemFetch, SIGNAL(result(KJob*)), SLOT(itemFetchResult(KJob*)) );
  ++mPendingJobs;
}

void NepomukFeederAgentBase::itemHeadersReceived(const Akonadi::Item::List& items)
{
  kDebug() << items.count();
  Akonadi::Item::List itemsToUpdate;
  foreach( const Item &item, items ) {
    if ( item.storageCollectionId() != mCurrentCollection.id() )
      continue; // stay away from links
    if ( !mMimeTypeChecker.isWantedItem( item ) )
      continue;
    // update item if it does not exist
    if ( !Nepomuk::ResourceManager::instance()->mainModel()->containsAnyStatement( Soprano::Node(),  Vocabulary::NIE::url(), item.url() ) ) {
      itemsToUpdate.append( item );

    // the item exists. Check if it has an item ID property, otherwise re-index it.
    } else { //TODO maybe reindex anyways to be sure that type etc is correct
      if ( !Nepomuk::ResourceManager::instance()->mainModel()->containsAnyStatement( Soprano::Node(),
                                   Vocabulary::ANEO::akonadiItemId(), Soprano::LiteralValue( QUrl( QString::number( item.id() ) ) ) ) || mReIndex ) {
        itemsToUpdate.append( item );
      }
    }
  }

  if ( !itemsToUpdate.isEmpty() ) {
    ItemFetchJob *itemFetch = new ItemFetchJob( itemsToUpdate, this );
    itemFetch->setFetchScope( fetchScopeForCollection( mCurrentCollection ) );
    connect( itemFetch, SIGNAL(itemsReceived(Akonadi::Item::List)), SLOT(itemsReceived(Akonadi::Item::List)) );
    connect( itemFetch, SIGNAL(result(KJob*)), SLOT(itemFetchResult(KJob*)) );
    ++mPendingJobs;
    mTotalAmount += itemsToUpdate.size();
  }
}

void NepomukFeederAgentBase::itemFetchResult(KJob* job)
{
  if ( job->error() )
    kDebug() << job->errorString();

  --mPendingJobs;
  if ( mPendingJobs == 0 && mItemPipeline.isEmpty() ) {
    mCurrentCollection = Collection();
    emit status( Idle, i18n( "Indexing completed." ) );
    processNextCollection();
    return;
  }
}

void NepomukFeederAgentBase::itemsReceived(const Akonadi::Item::List& items)
{
  kDebug() << items.size();
  foreach ( Item item, items ) {
    item.setParentCollection( mCurrentCollection );
    mItemPipeline.enqueue( item );
  }
  mProcessPipelineTimer.start();
}

void NepomukFeederAgentBase::notificationItemsReceived(const Akonadi::Item::List& items)
{
  kDebug() << items.size();
  foreach ( const Item &item, items ) {
    if ( !item.hasPayload() ) {
      continue;
    }
    mItemPipeline.enqueue( item );
  }
  mProcessPipelineTimer.start();
}


void NepomukFeederAgentBase::selfTest()
{
  QStringList errorMessages;
  mSelfTestPassed = false;

  // if Nepomuk is not running, try to start it
  if ( !mNepomukStartupAttempted && !Nepomuk::ResourceManager::instance()->initialized() ) {
    KProcess process;
    const QString nepomukserver = KStandardDirs::findExe( QLatin1String("nepomukserver") );
    if ( process.startDetached( nepomukserver ) == 0 ) {
      errorMessages.append( i18n( "Unable to start the Nepomuk server." ) );
    } else {
      mNepomukStartupAttempted = true;
      mNepomukStartupTimeout.start();
      // wait for Nepomuk to start
      checkOnline();
      emit status( Broken, i18n( "Waiting for the Nepomuk server to start..." ) );
      return;
    }
  }

  // if it is already running, check if the backend is correct
  if ( Nepomuk::ResourceManager::instance()->initialized() ) {
    static const QStringList backendBlacklist = QStringList() << QLatin1String( "redland" );
    // check which backend is used
    QDBusInterface interface( QLatin1String( "org.kde.NepomukStorage" ), QLatin1String( "/nepomukstorage" ) );
    QDBusReply<QString> reply = interface.call( QLatin1String( "usedSopranoBackend" ) );
    if ( reply.isValid() ) {
      const QString backend = reply.value().toLower();
      if ( backendBlacklist.contains( backend ) )
        errorMessages.append( i18n( "A blacklisted backend is used: '%1'.", backend ) );
    } else {
      errorMessages.append( i18n( "Calling the Nepomuk storage service failed: '%1'.", reply.error().message() ) );
    }
  } else if ( mNepomukStartupAttempted && mNepomukStartupTimeout.isActive() ) {
    // still waiting for Nepomuk to start
    setOnline( false );
    emit status( Broken, i18n( "Waiting for the Nepomuk server to start..." ) );
    return;
  } else {
    errorMessages.append( i18n( "Nepomuk is not running." ) );
  }

  // try to obtain a Strigi index manager with a Soprano backend
  if ( !mStrigiIndexManager && mNeedsStrigi ) {
    mStrigiIndexManager = Strigi::IndexPluginLoader::createIndexManager( "nepomukbackend", 0 );
    if ( !mStrigiIndexManager ) {
      errorMessages.append( i18n( "Nepomuk backend for Strigi is not available." ) );
      foreach ( const std::string &plugin, Strigi::IndexPluginLoader::indexNames() )
        kWarning() << "Available plugins are: " << plugin.c_str();
    }
  }

  if ( errorMessages.isEmpty() ) {
    mSelfTestPassed = true;
    mNepomukStartupAttempted = false; // everything worked, we can try again if the server goes down later
    mNepomukStartupTimeout.stop();
    checkOnline();
    if ( !mInitialUpdateDone && needsReIndexing() ) {
      mInitialUpdateDone = true;
      QTimer::singleShot( 0, this, SLOT(updateAll()) );
    } else {
      emit status( Idle, i18n( "Ready to index data." ) );
    }
    return;
  }

  checkOnline();

  QString message = i18n( "<b>Nepomuk Indexing Agents Have Been Disabled</b><br/>"
                          "The Nepomuk service is not available or fully operational and attempts to rectify this have failed. "
                          "Therefore indexing of all data stored in the Akonadi PIM service has been disabled, which will "
                          "severely limit the capabilities of any application using this data.<br/><br/>"
                          "The following problems were detected:<ul><li>%1</li></ul>"
                          "Additional help can be found here: <a href=\"http://userbase.kde.org/Akonadi\">userbase.kde.org/Akonadi</a>",
                          errorMessages.join( QLatin1String( "</li><li>" ) ) );

  // prevent a message storm from all agents
  emit status( Broken, i18n( "Nepomuk not operational" ) );
  if ( !QDBusConnection::sessionBus().registerService( QLatin1String( "org.kde.pim.nepomukfeeder.selftestreport" ) ) )
    return;
  KNotification::event( KNotification::Warning, i18n( "Nepomuk Indexing Disabled" ), message );
  QDBusConnection::sessionBus().unregisterService( QLatin1String( "org.kde.pim.nepomukfeeder.selftestreport" ) );
}

void NepomukFeederAgentBase::setNeedsStrigi(bool enableStrigi)
{
  mNeedsStrigi = enableStrigi;
}

void NepomukFeederAgentBase::indexData(const KUrl& url, const QByteArray& data, const QDateTime& mtime)
{
  Q_ASSERT( mNeedsStrigi );
  if ( !mStrigiIndexManager ) {
    emit status( Broken, i18n( "Strigi is not available for indexing." ) );
    return;
  }

  Strigi::IndexWriter* writer = mStrigiIndexManager->indexWriter();
  Strigi::AnalyzerConfiguration ic;
  Strigi::StreamAnalyzer streamindexer( ic );
  streamindexer.setIndexWriter( *writer );
  Strigi::StringInputStream sr( data.constData(), data.size(), false );
  Strigi::AnalysisResult idx( url.url().toLatin1().constData(), mtime.toTime_t(), *writer, streamindexer );
  idx.index( &sr );
}

ItemFetchScope NepomukFeederAgentBase::fetchScopeForCollection(const Akonadi::Collection& collection)
{
  Q_UNUSED( collection );
  return changeRecorder()->itemFetchScope();
}

void NepomukFeederAgentBase::setIndexCompatibilityLevel(int level)
{
  mIndexCompatLevel = level;
}

void NepomukFeederAgentBase::disableIdleDetection( bool value )
{
  mIdleDetectionDisabled = value;
}

bool NepomukFeederAgentBase::needsReIndexing() const
{
  const KConfigGroup grp( componentData().config(), "InitialIndexing" );
  return mIndexCompatLevel > grp.readEntry( "IndexCompatLevel", 0 );
}

void NepomukFeederAgentBase::slotFullyIndexed()
{
  KConfigGroup grp( componentData().config(), "InitialIndexing" );
  grp.writeEntry( "IndexCompatLevel", mIndexCompatLevel );
  grp.sync();
}

void NepomukFeederAgentBase::doSetOnline(bool online)
{
  changeRecorder()->setChangeRecordingEnabled( !online );
  Akonadi::AgentBase::doSetOnline( online );
}

void NepomukFeederAgentBase::checkOnline()
{
  if ( mIdleDetectionDisabled )
    setOnline( mSelfTestPassed );
  else
    setOnline( mSelfTestPassed && mSystemIsIdle );

  if ( isOnline() && !mItemPipeline.isEmpty() ) {
    if ( mCurrentCollection.isValid() )
      emit status( AgentBase::Running, i18n( "Indexing collection '%1'...", mCurrentCollection.name() ) );
    else
      emit status( AgentBase::Running, i18n( "Indexing recent changes..." ) );
    mProcessPipelineTimer.start();
  }
}

void NepomukFeederAgentBase::systemIdle()
{
  if ( mIdleDetectionDisabled )
    return;

  emit status( Idle, i18n( "System idle, ready to index data." ) );
  mSystemIsIdle = true;
  KIdleTime::instance()->catchNextResumeEvent();
  checkOnline();
}

void NepomukFeederAgentBase::systemResumed()
{
  if ( mIdleDetectionDisabled )
    return;

  emit status( Idle, i18n( "System busy, indexing suspended." ) );
  mSystemIsIdle = false;
  checkOnline();
}

void NepomukFeederAgentBase::processPipeline()
{
  static bool processing = false; // guard against sub-eventloop reentrancy
  if ( processing )
    return;
  processing = true;
  if ( !mItemPipeline.isEmpty() && isOnline() ) {
    const Akonadi::Item &item = mItemPipeline.dequeue();

    if ( !mResourceGraph ) {
      mResourceGraph = new Nepomuk::SimpleResourceGraph();
    }

    //removeDataByApplication, to ensure also subproperties are removed, i.e. in case addresses of a contact have changed
    KJob *job = Nepomuk::removeDataByApplication( QList <QUrl>() << item.url(), Nepomuk::RemoveSubResoures, KGlobal::mainComponent() );
    //kWarning() << item.url();
    ++mPendingRemoveDataJobs;
    connect( job, SIGNAL( finished( KJob* ) ), this, SLOT( removeDataResult( KJob* ) ) );

    addItemToGraph( item, *mResourceGraph );
    //The actual saving to the nepomukstore is deferred until a complete pipeline is processed and all deletejobs have finished to ensure the old items have been removed
    //and to minimize disk access

    ++mProcessedAmount;
    if ( (mProcessedAmount % 10) == 0 && mTotalAmount > 0 && mProcessedAmount <= mTotalAmount )
      emit percent( (mProcessedAmount * 100) / mTotalAmount );
    if ( !mItemPipeline.isEmpty() ) {
      // go to eventloop before processing the next one, otherwise we miss the idle status change
      mProcessPipelineTimer.start();
    }
  }
  processing = false;

  if ( mItemPipeline.isEmpty() ) {
    emit status( AgentBase::Idle, QString() );
  }

  if ( !isOnline() )
    return;

  if ( mPendingJobs == 0 && mCurrentCollection.isValid() && mItemPipeline.isEmpty() ) {
    //kWarning() << "indexing completed";
    mCurrentCollection = Collection();
    emit status( Idle, i18n( "Indexing completed." ) );
    processNextCollection();
    return;
  }
}

void NepomukFeederAgentBase::removeDataResult(KJob* job)
{
  //kWarning() << mPendingRemoveDataJobs;
  if ( job->error() )
    kWarning() << job->errorString();

  if ( mPendingRemoveDataJobs <= 0 )
    return;

  --mPendingRemoveDataJobs;
  if ( mPendingRemoveDataJobs == 0 ) { //All old items have been removed, so we can now store the new items
    //kWarning() << "Saving Graph";
    Q_ASSERT( mResourceGraph );
    addGraphToNepomuk( *mResourceGraph );
    delete mResourceGraph;
    mResourceGraph = 0;
  }
}

void NepomukFeederAgentBase::jobResult(KJob* job)
{
  //kWarning();
  if (job->error())
    kWarning() << job->errorString();
}




#include "nepomukfeederagentbase.moc"
