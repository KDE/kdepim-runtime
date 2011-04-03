/*
    Copyright (c) 2007 Tobias Koenig <tokoe@kde.org>
                  2008 Sebastian Trueg <trueg@kde.org>
                  2009 Volker Krause <vkrause@kde.org>

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
#include <nie.h>
#include <nco.h>
#include <personcontact.h>
#include <emailaddress.h>

#include <akonadi/changerecorder.h>
#include <akonadi/collectionfetchjob.h>
#include <akonadi/collectionfetchscope.h>
#include <akonadi/entityhiddenattribute.h>
#include <akonadi/indexpolicyattribute.h>
#include <akonadi/item.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/mimetypechecker.h>
#include <Akonadi/ItemSearchJob>

#include <nepomuk/resource.h>
#include <nepomuk/tag.h>
#include <nepomuk/andterm.h>
#include <nepomuk/comparisonterm.h>
#include <nepomuk/literalterm.h>
#include <nepomuk/resourcetypeterm.h>

#include <KLocale>
#include <KUrl>
#include <KProcess>
#include <KMessageBox>
#include <KStandardDirs>
#include <KIdleTime>
#include <KNotification>

#include <Soprano/Vocabulary/NAO>

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
  mNrlModel( 0 ),
  mStrigiIndexManager( 0 ),
  mIndexCompatLevel( 1 ),
  mNepomukStartupAttempted( false ),
  mInitialUpdateDone( false ),
  mNeedsStrigi( false ),
  mSelfTestPassed( false ),
  mSystemIsIdle( false ),
  mIdleDetectionDisabled( false )
{
  // initialize Nepomuk
  Nepomuk::ResourceManager::instance()->init();
  mNrlModel = new Soprano::NRLModel( Nepomuk::ResourceManager::instance()->mainModel() );

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
  delete mNrlModel;
  if ( mStrigiIndexManager )
    Strigi::IndexPluginLoader::deleteIndexManager( mStrigiIndexManager );
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
  if ( indexingDisabled( item.parentCollection() ) )
    return;
  // TODO: check part identfiers if anything interesting changed at all
  if ( item.hasPayload() ) {
    removeEntityFromNepomuk( item );
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
  removeEntityFromNepomuk( item );
}

void NepomukFeederAgentBase::collectionAdded(const Akonadi::Collection& collection, const Akonadi::Collection& parent)
{
  Q_UNUSED( parent );
  if ( indexingDisabled( collection ) )
    return;
  updateCollection( collection, createGraphForEntity( collection ) );
}

void NepomukFeederAgentBase::collectionChanged(const Akonadi::Collection& collection, const QSet< QByteArray >& partIdentifiers)
{
  Q_UNUSED( partIdentifiers );
  removeEntityFromNepomuk( collection );
  if ( indexingDisabled( collection ) )
    return;
  updateCollection( collection, createGraphForEntity( collection ) );
}

void NepomukFeederAgentBase::collectionRemoved(const Akonadi::Collection& collection)
{
  removeEntityFromNepomuk( collection );
}

void NepomukFeederAgentBase::addSupportedMimeType( const QString &mimeType )
{
  mSupportedMimeTypes.append( mimeType );
  changeRecorder()->setMimeTypeMonitored( mimeType );
  mMimeTypeChecker.setWantedMimeTypes( mSupportedMimeTypes );
}

void NepomukFeederAgentBase::updateAll()
{
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
    emit fullyIndexed();
    return;
  }
  mCurrentCollection = mCollectionQueue.takeFirst();
  emit status( AgentBase::Running, i18n( "Indexing collection '%1'...", mCurrentCollection.name() ) );
  kDebug() << "Indexing collection" << mCurrentCollection.name();
  if ( !Nepomuk::ResourceManager::instance()->mainModel()->containsAnyStatement( mCurrentCollection.url(), Soprano::Node(), Soprano::Node() ) )
    updateCollection( mCurrentCollection, createGraphForEntity( mCurrentCollection ) );
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
    if ( !Nepomuk::ResourceManager::instance()->mainModel()->containsAnyStatement( item.url(), Soprano::Node(), Soprano::Node() ) )
      itemsToUpdate.append( item );

    // the item exists. Check if it has an item ID property, otherwise re-index it.
    else {
      if ( !Nepomuk::ResourceManager::instance()->mainModel()->containsAnyStatement( item.url(),
                                   Akonadi::ItemSearchJob::akonadiItemIdUri(), Soprano::Node() ) ) {
        removeEntityFromNepomuk( item );
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
    // we only get here if the item is not anywhere in Nepomuk yet, so no need to delete it
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
    removeEntityFromNepomuk( item );
    mItemPipeline.enqueue( item );
  }
  mProcessPipelineTimer.start();
}

void NepomukFeederAgentBase::tagsFromCategories(NepomukFast::Resource& resource, const QStringList& categories)
{
  foreach ( const QString &category, categories ) {
    Nepomuk::Tag tag( category );
    if ( tag.label().isEmpty() )
      tag.setLabel( category );
    resource.addProperty( Soprano::Vocabulary::NAO::hasTag(), tag.resourceUri() );
  }
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

NepomukFast::PersonContact NepomukFeederAgentBase::findOrCreateContact(const QString& emailAddress, const QString& name, const QUrl& graphUri, bool* found)
{
  //
  // Querying with the exact address string is not perfect since email addresses
  // are case insensitive. But for the moment we stick to it and hope Nepomuk
  // alignment fixes any duplicates
  //
  Nepomuk::Query::Query query;
  Nepomuk::Query::AndTerm andTerm;
  Nepomuk::Query::ResourceTypeTerm personTypeTerm( Vocabulary::NCO::PersonContact() );
  andTerm.addSubTerm( personTypeTerm );
  if ( emailAddress.isEmpty() ) {
    const Nepomuk::Query::ComparisonTerm nameTerm( Vocabulary::NCO::fullname(), Nepomuk::Query::LiteralTerm( name ),
                                                   Nepomuk::Query::ComparisonTerm::Equal );
    andTerm.addSubTerm( nameTerm );
  } else {
    const Nepomuk::Query::ComparisonTerm addrTerm( Vocabulary::NCO::emailAddress(), Nepomuk::Query::LiteralTerm( emailAddress ),
                                                   Nepomuk::Query::ComparisonTerm::Equal );
    const Nepomuk::Query::ComparisonTerm mailTerm( Vocabulary::NCO::hasEmailAddress(), addrTerm );
    andTerm.addSubTerm( mailTerm );
  }
  query.setTerm( andTerm );
  query.setLimit( 1 );
  
  Soprano::QueryResultIterator it = Nepomuk::ResourceManager::instance()->mainModel()->executeQuery( query.toSparqlQuery(), Soprano::Query::QueryLanguageSparql );

  if ( it.next() ) {
    if ( found ) *found = true;
    const QUrl uri = it.binding( 0 ).uri();
    it.close();
    return NepomukFast::PersonContact( uri, graphUri );
  }
  if ( found ) *found = false;
  // create a new contact
  kDebug() << "Did not find " << name << emailAddress << ", creating a new PersonContact";
  NepomukFast::PersonContact contact( QUrl(), graphUri );
  contact.setLabel( name.isEmpty() ? emailAddress : name );
  if ( !emailAddress.isEmpty() ) {
    NepomukFast::EmailAddress emailRes( QUrl( QLatin1String( "mailto:" ) + emailAddress ), graphUri );
    emailRes.setEmailAddress( emailAddress );
    contact.addEmailAddress( emailRes );
  }
  if ( !name.isEmpty() )
    contact.setFullname( name );
  return contact;
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
    const QUrl graph = createGraphForEntity( item );
    mNrlModel->addStatement( item.url(), Akonadi::ItemSearchJob::akonadiItemIdUri(),
                             QUrl( QString::number( item.id() ) ), graph );
    updateItem( item, graph );
    ++mProcessedAmount;
    if ( (mProcessedAmount % 10) == 0 && mTotalAmount > 0 && mProcessedAmount <= mTotalAmount )
      emit percent( (mProcessedAmount * 100) / mTotalAmount );
    if ( !mItemPipeline.isEmpty() ) {
      // go to eventloop before processing the next one, otherwise we miss the idle status change
      mProcessPipelineTimer.start();
    }
  }
  processing = false;

  if ( mItemPipeline.isEmpty() )
    emit status( AgentBase::Idle, QString() );

  if ( !isOnline() )
    return;

  if ( mPendingJobs == 0 && mCurrentCollection.isValid() && mItemPipeline.isEmpty() ) {
    mCurrentCollection = Collection();
    emit status( Idle, i18n( "Indexing completed." ) );
    processNextCollection();
    return;
  }
}

#include "nepomukfeederagentbase.moc"
