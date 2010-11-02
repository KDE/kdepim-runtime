/*
    Copyright (c) 2006 - 2007 Volker Krause <vkrause@kde.org>

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

#include "strigifeeder.h"

#include "configdialog.h"
#include "settings.h"

#include <akonadi/changerecorder.h>
#include <akonadi/collectionfetchjob.h>
#include <akonadi/entityhiddenattribute.h>
#include <akonadi/item.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>

#include <kconfiggroup.h>
#include <kdebug.h>
#include <kidletime.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kprocess.h>
#include <kstandarddirs.h>
#include <kurl.h>

#include <QtCore/QDateTime>

using namespace Akonadi;

static const int INDEX_COMPAT_LEVEL = 1; // increment when the index format changes

static inline bool entityIsHidden( const Entity &entity )
{
  return entity.hasAttribute<EntityHiddenAttribute>();
}

StrigiFeeder::StrigiFeeder( const QString &id )
  : AgentBase( id ),
    mTotalAmount( 0 ),
    mProcessedAmount( 0 ),
    mPendingJobs( 0 ),
    mIndexCompatLevel( 1 ),
    mStrigiDaemonStartupAttempted( false ),
    mInitialUpdateDone( false ),
    mSelfTestPassed( false ),
    mSystemIsIdle( false )
{
  setIndexCompatibilityLevel( INDEX_COMPAT_LEVEL );

  changeRecorder()->setAllMonitored();
  changeRecorder()->itemFetchScope().fetchFullPayload();
  changeRecorder()->itemFetchScope().setAncestorRetrieval( ItemFetchScope::Parent );

  mStrigiDaemonStartupTimeout.setInterval( 300 * 1000 );
  mStrigiDaemonStartupTimeout.setSingleShot( true );
  connect( &mStrigiDaemonStartupTimeout, SIGNAL( timeout() ), SLOT( selfTest() ) );
  connect( this, SIGNAL( fullyIndexed() ), this, SLOT( slotFullyIndexed() ) );

  connect( KIdleTime::instance(), SIGNAL( timeoutReached( int ) ), SLOT( systemIdle() ) );
  connect( KIdleTime::instance(), SIGNAL( resumingFromIdle() ), SLOT( systemResumed() ) );
  KIdleTime::instance()->addIdleTimeout( 10 * 1000 );

  checkOnline();
  QTimer::singleShot( 0, this, SLOT( selfTest() ) );
}

void StrigiFeeder::configure( WId windowId )
{
  ConfigDialog dlg( windowId );
  if ( dlg.exec() ) {
    Settings::self()->writeConfig();
    emit configurationDialogAccepted();

    QTimer::singleShot( 0, this, SLOT( selfTest() ) );
  } else {
    emit configurationDialogRejected();
  }
}

void StrigiFeeder::setIndexCompatibilityLevel( int level )
{
  mIndexCompatLevel = level;
}

void StrigiFeeder::updateAll()
{
  CollectionFetchJob *collectionFetch = new CollectionFetchJob( Collection::root(), CollectionFetchJob::Recursive, this );
  connect( collectionFetch, SIGNAL( collectionsReceived( Akonadi::Collection::List ) ), SLOT( collectionsReceived( Akonadi::Collection::List ) ) );
}

static KUrl extendedItemUrl( const Item &item )
{
  KUrl url = item.url();
  url.addQueryItem( "collection", QString::number( item.parentCollection().id() ) );

  return url;
}

void StrigiFeeder::indexItem( const Item &item )
{
  const QByteArray data = item.payloadData();
  mStrigi.indexFile( extendedItemUrl( item ).url(), QDateTime::currentDateTime().toTime_t(), data );
}

void StrigiFeeder::itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection )
{
  if ( entityIsHidden( collection ) )
    return;

  if ( item.hasPayload() ) {
    indexItem( item );
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

void StrigiFeeder::itemChanged( const Akonadi::Item &item, const QSet<QByteArray>& )
{
  if ( entityIsHidden( item.parentCollection() ) )
    return;

  if ( item.hasPayload() ) {
    indexItem( item );
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

void StrigiFeeder::itemRemoved( const Akonadi::Item &item )
{
  mStrigi.indexFile( item.url().url(), QDateTime::currentDateTime().toTime_t(), QByteArray() );
}

void StrigiFeeder::collectionsReceived( const Akonadi::Collection::List &collections )
{
  foreach ( const Collection &collection, collections ) {
    if ( entityIsHidden( collection ) )
      continue;

    mCollectionQueue.append( collection );
  }

  if ( mPendingJobs == 0 ) {
    processNextCollection();
  }
}

void StrigiFeeder::processNextCollection()
{
  if ( mCurrentCollection.isValid() )
    return;

  if ( mCollectionQueue.isEmpty() ) {
    emit fullyIndexed();
    return;
  }

  mCurrentCollection = mCollectionQueue.takeFirst();
  emit status( AgentBase::Running, i18n( "Indexing collection '%1'...", mCurrentCollection.name() ) );

  ItemFetchJob *itemFetch = new ItemFetchJob( mCurrentCollection, this );
  itemFetch->fetchScope().setCacheOnly( true );
  connect( itemFetch, SIGNAL( itemsReceived( const Akonadi::Item::List& ) ), SLOT( itemHeadersReceived( const Akonadi::Item::List& ) ) );
  connect( itemFetch, SIGNAL( result( KJob* ) ), SLOT( itemFetchResult( KJob* ) ) );
  ++mPendingJobs;
  mTotalAmount = 0;
}

void StrigiFeeder::itemHeadersReceived( const Akonadi::Item::List &items )
{
  Akonadi::Item::List itemsToUpdate;
  foreach ( const Item &item, items ) {
    if ( item.storageCollectionId() != mCurrentCollection.id() )
      continue; // stay away from links

    itemsToUpdate.append( item );
    // update item if it does not exist
/*TODO: implement my strigi query
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
*/
  }

  if ( !itemsToUpdate.isEmpty() ) {
    ItemFetchJob *itemFetch = new ItemFetchJob( itemsToUpdate, this );
    itemFetch->setFetchScope( fetchScopeForCollection( mCurrentCollection ) );
    connect( itemFetch, SIGNAL( itemsReceived( const Akonadi::Item::List& ) ), SLOT( itemsReceived( const Akonadi::Item::List& ) ) );
    connect( itemFetch, SIGNAL( result( KJob* ) ), SLOT( itemFetchResult( KJob* ) ) );
    ++mPendingJobs;
    mTotalAmount += itemsToUpdate.size();
  }
}

void StrigiFeeder::itemFetchResult( KJob *job )
{
  if ( job->error() )
    kDebug() << job->errorString();

  --mPendingJobs;
  if ( mPendingJobs == 0 ) {
    mCurrentCollection = Collection();
    emit status( Idle, i18n( "Indexing completed." ) );
    processNextCollection();
    return;
  }
}

void StrigiFeeder::itemsReceived( const Akonadi::Item::List &items )
{
  foreach ( const Item &item, items ) {
    // we only get here if the item is not anywhere in strigi yet, so no need to delete it
    indexItem( item );
  }
  mProcessedAmount += items.count();
  emit percent( (mProcessedAmount * 100) / (mTotalAmount * 100) );
}

void StrigiFeeder::notificationItemsReceived( const Akonadi::Item::List &items )
{
  foreach ( const Item &item, items ) {
    if ( !item.hasPayload() )
      continue;

    indexItem( item );
  }
}

void StrigiFeeder::selfTest()
{
  QStringList errorMessages;
  mSelfTestPassed = false;

  QDBusInterface strigiInterface( QLatin1String( "org.freedesktop.xesam.searcher" ),
                                  QLatin1String( "/org/freedesktop/xesam/searcher/main" ),
                                  QLatin1String( "org.freedesktop.xesam.Search" ),
                                  QDBusConnection::sessionBus(), this );

  // if strigidaemon is not running, try to start it
  if ( !mStrigiDaemonStartupAttempted && !strigiInterface.isValid() ) {
    KProcess process;
    const QString strigidaemon = KStandardDirs::findExe( QLatin1String( "strigidaemon" ) );
    if ( process.startDetached( strigidaemon ) == 0 ) {
      errorMessages.append( i18n( "Unable to start the Strigi daemon." ) );
    } else {
      mStrigiDaemonStartupAttempted = true;
      mStrigiDaemonStartupTimeout.start();
      // wait for strigidaemon to start
      checkOnline();
      emit status( Broken, i18n( "Waiting for the Strigi daemon to start..." ) );
      return;
    }
  }

  if ( strigiInterface.isValid() ) {
    // everything ok
  } else if ( mStrigiDaemonStartupAttempted && mStrigiDaemonStartupTimeout.isActive() ) {
    // still waiting for strigidaemon to start
    setOnline( false );
    emit status( Broken, i18n( "Waiting for the Strigi daemon to start..." ) );
    return;
  } else {
    errorMessages.append( i18n( "Strigi daemon is not running." ) );
  }

  if ( errorMessages.isEmpty() ) {
    mSelfTestPassed = true;
    mStrigiDaemonStartupAttempted = false; // everything worked, we can try again if the server goes down later
    mStrigiDaemonStartupTimeout.stop();
    checkOnline();
    if ( !mInitialUpdateDone && needsReIndexing() ) {
      mInitialUpdateDone = true;
      QTimer::singleShot( 0, this, SLOT( updateAll() ) );
    } else {
      emit status( Idle, i18n( "Ready to index data." ) );
    }
    return;
  }

  checkOnline();

  QString message = i18n( "<b>Strigi Indexing Agent Has Been Disabled</b><br/>"
                          "The Strigi service is not available or fully operational and attempts to rectify this have failed. "
                          "Therefore indexing of all data stored in the Akonadi PIM service has been disabled, which will "
                          "severely limit the capabilities of any application using this data.<br/><br/>"
                          "The following problems were detected:<ul><li>%1</li></ul>"
                          "Additional help can be found here: <a href=\"http://userbase.kde.org/Akonadi\">userbase.kde.org/Akonadi</a>",
                          errorMessages.join( QLatin1String( "</li><li>" ) ) );

  emit status( Broken, i18n( "Strigi not operational" ) );
  KMessageBox::error( 0, message, i18n( "Strigi Indexing Disabled" ), KMessageBox::Notify | KMessageBox::AllowLink );
}

bool StrigiFeeder::needsReIndexing() const
{
  const KConfigGroup group( componentData().config(), "InitialIndexing" );
  return mIndexCompatLevel > group.readEntry( "IndexCompatLevel", 0 );
}

ItemFetchScope StrigiFeeder::fetchScopeForCollection( const Akonadi::Collection &collection )
{
  ItemFetchScope scope = changeRecorder()->itemFetchScope();

  switch ( Settings::self()->indexAggressiveness() ) {
    case Settings::LocalAndCached:
    {
      const QStringList localResources = QStringList()
        << QLatin1String( "akonadi_mixedmaildir_resource" )
        << QLatin1String( "akonadi_maildir_resource" )
        << QLatin1String( "akonadi_mbox_resource" )
        << QLatin1String( "akonadi_contacts_resource" );
      scope.setCacheOnly( !localResources.contains( collection.resource() ) );
      break;
    }
    case Settings::Everything:
      scope.setCacheOnly( false );
      break;
    case Settings::CachedOnly:
    default:
      scope.setCacheOnly( true );
  }

  scope.fetchFullPayload( true );

  return scope;
}

void StrigiFeeder::slotFullyIndexed()
{
  KConfigGroup group( componentData().config(), "InitialIndexing" );
  group.writeEntry( "IndexCompatLevel", mIndexCompatLevel );
  group.sync();
}

void StrigiFeeder::doSetOnline( bool online )
{
  changeRecorder()->setChangeRecordingEnabled( !online );
  Akonadi::AgentBase::doSetOnline( online );
}

void StrigiFeeder::checkOnline()
{
  setOnline( mSelfTestPassed && mSystemIsIdle );
}

void StrigiFeeder::systemIdle()
{
  emit status( Idle, i18n( "System idle, ready to index data." ) );
  mSystemIsIdle = true;
  KIdleTime::instance()->catchNextResumeEvent();
  checkOnline();
}

void StrigiFeeder::systemResumed()
{
  emit status( Idle, i18n( "System busy, indexing suspended." ) );
  mSystemIsIdle = false;
  checkOnline();
}

AKONADI_AGENT_MAIN( StrigiFeeder )

#include "strigifeeder.moc"
