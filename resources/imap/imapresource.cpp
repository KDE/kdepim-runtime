/*
    Copyright (c) 2007 Till Adam <adam@kde.org>
    Copyright (C) 2008 Omat Holding B.V. <info@omat.nl>
    Copyright (C) 2009 Kevin Ottens <ervin@kde.org>

    Copyright (c) 2010 Klarälvdalens Datakonsult AB,
                       a KDAB Group company <info@kdab.com>
    Author: Kevin Ottens <kevin@kdab.com>

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

#include "imapresource.h"

#include <QHostInfo>

#include <kdebug.h>
#include <klocale.h>
#include <KWindowSystem>

#ifndef IMAPRESOURCE_NO_SOLID
#include <solid/networking.h>
#endif

#include <akonadi/attributefactory.h>
#include <akonadi/collectionfetchjob.h>
#include <akonadi/collectionfetchscope.h>
#include <akonadi/changerecorder.h>
#include <akonadi/itemfetchscope.h>

#include "collectionannotationsattribute.h"
#include "collectionflagsattribute.h"
#include "imapaclattribute.h"
#include "imapquotaattribute.h"
#include "noselectattribute.h"
#include "timestampattribute.h"
#include "uidvalidityattribute.h"
#include "uidnextattribute.h"

#include "setupserver.h"
#include "settings.h"
#include "imapaccount.h"
#include "imapidlemanager.h"
#include "resourcestate.h"

#include "addcollectiontask.h"
#include "additemtask.h"
#include "changecollectiontask.h"
#include "changeitemtask.h"
#include "expungecollectiontask.h"
#include "movecollectiontask.h"
#include "moveitemtask.h"
#include "removecollectiontask.h"
#include "removeitemtask.h"
#include "retrievecollectionmetadatatask.h"
#include "retrievecollectionstask.h"
#include "retrieveitemtask.h"
#include "retrieveitemstask.h"

#include "settingspasswordrequester.h"
#include "sessionpool.h"
#include "sessionuiproxy.h"

#include "resourceadaptor.h"

#ifdef MAIL_SERIALIZER_PLUGIN_STATIC
#include <QtPlugin>

Q_IMPORT_PLUGIN(akonadi_serializer_mail)
#endif

using namespace Akonadi;

ImapResource::ImapResource( const QString &id )
  : ResourceBase( id ), m_pool( new SessionPool( 2, this ) ), m_idle( 0 ), m_fastSync( false )
{
  m_pool->setPasswordRequester( new SettingsPasswordRequester( this, m_pool ) );
  m_pool->setSessionUiProxy( SessionUiProxy::Ptr( new SessionUiProxy ) );

  connect( m_pool, SIGNAL(connectDone(int, QString)),
           this, SLOT(onConnectDone(int, QString)) );
  connect( m_pool, SIGNAL(connectionLost(KIMAP::Session*)),
           this, SLOT(onConnectionLost(KIMAP::Session*)) );

  Akonadi::AttributeFactory::registerAttribute<UidValidityAttribute>();
  Akonadi::AttributeFactory::registerAttribute<UidNextAttribute>();
  Akonadi::AttributeFactory::registerAttribute<NoSelectAttribute>();
  Akonadi::AttributeFactory::registerAttribute<TimestampAttribute>();

  Akonadi::AttributeFactory::registerAttribute<CollectionAnnotationsAttribute>();
  Akonadi::AttributeFactory::registerAttribute<CollectionFlagsAttribute>();

  Akonadi::AttributeFactory::registerAttribute<ImapAclAttribute>();
  Akonadi::AttributeFactory::registerAttribute<ImapQuotaAttribute>();

  changeRecorder()->fetchCollection( true );
  changeRecorder()->collectionFetchScope().setAncestorRetrieval( CollectionFetchScope::All );
  changeRecorder()->collectionFetchScope().setIncludeStatistics( true );
  changeRecorder()->itemFetchScope().fetchFullPayload( true );
  changeRecorder()->itemFetchScope().setAncestorRetrieval( ItemFetchScope::All );

  setHierarchicalRemoteIdentifiersEnabled( true );
  setItemTransactionMode( ItemSync::MultipleTransactions ); // we can recover from incomplete syncs, so we can use a faster mode
  ItemFetchScope scope( changeRecorder()->itemFetchScope() );
  scope.fetchFullPayload( false );
  setItemSynchronizationFetchScope( scope );

  connect( this, SIGNAL(reloadConfiguration()), SLOT(reconnect()) );

  Settings::self(); // make sure the D-Bus settings interface is up
  new ResourceAdaptor( this );
}

ImapResource::~ImapResource()
{
}

void ImapResource::setFastSyncEnabled( bool fastSync )
{
  m_fastSync = fastSync;
}

bool ImapResource::isFastSyncEnabled() const
{
  return m_fastSync;
}

// ----------------------------------------------------------------------------------

int ImapResource::configureDialog( WId windowId )
{
  SetupServer dlg( this, windowId );
  KWindowSystem::setMainWindow( &dlg, windowId );

  dlg.exec();
  if ( dlg.shouldClearCache() ) {
    clearCache();
  }

  int result = dlg.result();

  if ( result==QDialog::Accepted ) {
    Settings::self()->writeConfig();
  }

  return dlg.result();
}

void ImapResource::configure( WId windowId )
{
  if ( configureDialog( windowId ) == QDialog::Accepted ) {
    emit configurationDialogAccepted();
    reconnect();
  } else {
    emit configurationDialogRejected();
  }
}



// ----------------------------------------------------------------------------------

void ImapResource::startConnect( const QVariant& )
{
  if ( Settings::self()->imapServer().isEmpty() ) {
    setOnline( false );
    emit status( Broken, i18n( "No server configured yet." ) );
    taskDone();
    return;
  }

  m_pool->disconnect(); // reset all state, delete any old account
  ImapAccount *account = new ImapAccount;
  Settings::self()->loadAccount( account );

  const bool result = m_pool->connect( account );
  Q_ASSERT( result );
}

void ImapResource::onConnectDone( int errorCode, const QString &errorString )
{
  switch ( errorCode ) {
  case SessionPool::NoError:
    setOnline( true );
    taskDone();
    emit status( Idle, i18n( "Connection established." ) );

    synchronizeCollectionTree();
    break;

  case SessionPool::PasswordRequestError:
  case SessionPool::EncryptionError:
  case SessionPool::LoginFailError:
  case SessionPool::CapabilitiesTestError:
  case SessionPool::IncompatibleServerError:
    setOnline( false );
    emit status( Broken, errorString );
    taskDone();
    return;

  case SessionPool::ReconnectNeededError:
    reconnect();
    return;

  case SessionPool::NoAvailableSessionError:
    kFatal() << "Shouldn't happen";
    return;
  }
}

void ImapResource::onConnectionLost( KIMAP::Session */*session*/ )
{
  if ( !m_pool->isConnected() ) {
    reconnect();
  }
}



// ----------------------------------------------------------------------------------

bool ImapResource::retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts )
{
  ResourceStateInterface::Ptr state = ::ResourceState::createRetrieveItemState( this, item, parts );
  RetrieveItemTask *task = new RetrieveItemTask( state, this );
  task->start( m_pool );
  return true;
}

void ImapResource::itemAdded( const Item &item, const Collection &collection )
{
  ResourceStateInterface::Ptr state = ::ResourceState::createAddItemState( this, item, collection );
  AddItemTask *task = new AddItemTask( state, this );
  task->start( m_pool );
}

void ImapResource::itemChanged( const Item &item, const QSet<QByteArray> &parts )
{
  ResourceStateInterface::Ptr state = ::ResourceState::createChangeItemState( this, item, parts );
  ChangeItemTask *task = new ChangeItemTask( state, this );
  task->start( m_pool );
}

void ImapResource::itemRemoved( const Akonadi::Item &item )
{
  ResourceStateInterface::Ptr state = ::ResourceState::createRemoveItemState( this, item );
  RemoveItemTask *task = new RemoveItemTask( state, this );
  task->start( m_pool );
}

void ImapResource::itemMoved( const Akonadi::Item &item, const Akonadi::Collection &source,
                              const Akonadi::Collection &destination )
{
  Q_ASSERT( item.parentCollection() == destination ); // should have been set by the server

  ResourceStateInterface::Ptr state = ::ResourceState::createMoveItemState( this, item, source, destination );
  MoveItemTask *task = new MoveItemTask( state, this );
  task->start( m_pool );
}



// ----------------------------------------------------------------------------------

void ImapResource::retrieveCollections()
{
  ResourceStateInterface::Ptr state = ::ResourceState::createRetrieveCollectionsState( this );
  RetrieveCollectionsTask *task = new RetrieveCollectionsTask( state, this );
  task->start( m_pool );
}

void ImapResource::retrieveCollectionAttributes( const Akonadi::Collection &collection )
{
  ResourceStateInterface::Ptr state = ::ResourceState::createRetrieveCollectionMetadataState( this, collection );

  RetrieveCollectionMetadataTask *task = new RetrieveCollectionMetadataTask( state, this );
  task->setSpontaneous( false );
  task->start( m_pool );
}

void ImapResource::triggerCollectionExtraInfoJobs( const QVariant &collectionVariant )
{
  const Collection collection( collectionVariant.value<Collection>() );
  ResourceStateInterface::Ptr state = ::ResourceState::createRetrieveCollectionMetadataState( this, collection );

  RetrieveCollectionMetadataTask *task = new RetrieveCollectionMetadataTask( state, this );
  task->start( m_pool );
}

void ImapResource::retrieveItems( const Collection &col )
{
  scheduleCustomTask( this, "triggerCollectionExtraInfoJobs", QVariant::fromValue( col ), ResourceBase::Append );

  setItemStreamingEnabled( true );

  ResourceStateInterface::Ptr state = ::ResourceState::createRetrieveItemsState( this, col );
  RetrieveItemsTask *task = new RetrieveItemsTask( state, this );
  task->setFastSyncEnabled( m_fastSync );
  task->start( m_pool );
}



// ----------------------------------------------------------------------------------

void ImapResource::collectionAdded( const Collection & collection, const Collection &parent )
{
  ResourceStateInterface::Ptr state = ::ResourceState::createAddCollectionState( this, collection, parent );
  AddCollectionTask *task = new AddCollectionTask( state, this );
  task->start( m_pool );
}

void ImapResource::collectionChanged( const Collection &collection, const QSet<QByteArray> &parts )
{
  ResourceStateInterface::Ptr state = ::ResourceState::createChangeCollectionState( this, collection, parts );
  ChangeCollectionTask *task = new ChangeCollectionTask( state, this );
  task->start( m_pool );
}

void ImapResource::collectionRemoved( const Collection &collection )
{
  ResourceStateInterface::Ptr state = ::ResourceState::createRemoveCollectionState( this, collection );
  RemoveCollectionTask *task = new RemoveCollectionTask( state, this );
  task->start( m_pool );
}

void ImapResource::collectionMoved( const Akonadi::Collection &collection, const Akonadi::Collection &source,
                                    const Akonadi::Collection &destination )
{
  ResourceStateInterface::Ptr state = ::ResourceState::createMoveCollectionState( this, collection, source, destination );
  MoveCollectionTask *task = new MoveCollectionTask( state, this );
  task->start( m_pool );
}



// ----------------------------------------------------------------------------------

void ImapResource::scheduleConnectionAttempt()
{
  // block all other tasks, until we are connected
  scheduleCustomTask( this, "startConnect", QVariant(), ResourceBase::Prepend );
}

void ImapResource::doSetOnline(bool online)
{
#ifndef IMAPRESOURCE_NO_SOLID
  kDebug() << "online=" << online;
#endif
  if ( !online && m_pool->isConnected() ) {
    m_pool->disconnect();
    delete m_idle;
    m_idle = 0;
  } else if ( online && !m_pool->isConnected() ) {
    scheduleConnectionAttempt();
  }
  ResourceBase::doSetOnline( online );
}

bool ImapResource::needsNetwork() const
{
  const QString hostName = Settings::self()->imapServer().section( ':', 0, 0 );
  // ### is there a better way to do this?
  if ( hostName == QLatin1String( "127.0.0.1" ) ||
       hostName == QLatin1String( "localhost" ) ||
       hostName == QHostInfo::localHostName() ) {
    return false;
  }
  return true;
}

void ImapResource::reconnect()
{
  setNeedsNetwork( needsNetwork() );
  setOnline( false ); // we are not connected initially

  setOnline( !needsNetwork()
#ifndef IMAPRESOURCE_NO_SOLID
			 ||
             Solid::Networking::status() == Solid::Networking::Unknown ||
             Solid::Networking::status() == Solid::Networking::Connected
#endif
             );

}



// ----------------------------------------------------------------------------------

void ImapResource::startIdleIfNeeded()
{
  if (!m_idle) {
    startIdle();
  }
}

void ImapResource::startIdle()
{
  delete m_idle;
  m_idle = 0;

  if ( !m_pool->serverCapabilities().contains( "IDLE" ) )
    return;

  const QStringList ridPath = Settings::self()->idleRidPath();
  if ( ridPath.size() < 2 )
    return;

  Collection c, p;
  p.setParentCollection( Collection::root() );
  for ( int i = ridPath.size() - 1; i > 0; --i ) {
    p.setRemoteId( ridPath.at( i ) );
    c.setParentCollection( p );
    p = c;
  }
  c.setRemoteId( ridPath.first() );

  Akonadi::CollectionFetchScope scope;
  scope.setResource( identifier() );
  scope.setAncestorRetrieval( Akonadi::CollectionFetchScope::All );

  Akonadi::CollectionFetchJob *fetch
    = new Akonadi::CollectionFetchJob( c, Akonadi::CollectionFetchJob::Base, this );
  fetch->setFetchScope( scope );

  connect( fetch, SIGNAL(result(KJob*)),
           this, SLOT(onIdleCollectionFetchDone(KJob*)) );
}

void ImapResource::onIdleCollectionFetchDone( KJob *job )
{
  if ( job->error() == 0 ) {
    Akonadi::CollectionFetchJob *fetch = static_cast<Akonadi::CollectionFetchJob*>( job );
    Akonadi::Collection c = fetch->collections().first();

    const QString password = Settings::self()->password();
    if ( password.isEmpty() )
      return;

    ResourceStateInterface::Ptr state = ::ResourceState::createIdleState( this, c );
    m_idle = new ImapIdleManager( state, m_pool, this );

  } else {
    kWarning() << "CollectionFetch for idling failed."
               << "error=" << job->error()
               << ", errorString=" << job->errorString();
  }
}



// ----------------------------------------------------------------------------------

void ImapResource::requestManualExpunge( qint64 collectionId )
{
  if ( !Settings::self()->automaticExpungeEnabled() ) {
    Collection collection( collectionId );

    Akonadi::CollectionFetchScope scope;
    scope.setResource( identifier() );
    scope.setAncestorRetrieval( Akonadi::CollectionFetchScope::All );

    Akonadi::CollectionFetchJob *fetch
      = new Akonadi::CollectionFetchJob( collection,
                                         Akonadi::CollectionFetchJob::Base,
                                         this );
    fetch->setFetchScope( scope );

    connect( fetch, SIGNAL(result(KJob*)),
             this, SLOT(onExpungeCollectionFetchDone(KJob*)) );
  }
}

void ImapResource::onExpungeCollectionFetchDone( KJob *job )
{
  if ( job->error() == 0 ) {
    Akonadi::CollectionFetchJob *fetch = static_cast<Akonadi::CollectionFetchJob*>( job );
    Akonadi::Collection collection = fetch->collections().first();

    scheduleCustomTask( this, "triggerCollectionExpunge",
                        QVariant::fromValue( collection ) );

  } else {
    kWarning() << "CollectionFetch for expunge failed."
               << "error=" << job->error()
               << ", errorString=" << job->errorString();
  }
}

void ImapResource::triggerCollectionExpunge( const QVariant &collectionVariant )
{
  const Collection collection = collectionVariant.value<Collection>();

  ResourceStateInterface::Ptr state = ::ResourceState::createExpungeCollectionState( this, collection );
  ExpungeCollectionTask *task = new ExpungeCollectionTask( state, this );
  task->start( m_pool );
}



// ----------------------------------------------------------------------------------

AKONADI_RESOURCE_MAIN( ImapResource )

#include "imapresource.moc"
