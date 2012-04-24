/*
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

#include "resourcestate.h"

#include "imapaccount.h"
#include "imapresource.h"
#include "sessionpool.h"
#include "settings.h"
#include "noselectattribute.h"
#include "timestampattribute.h"

#include <akonadi/collectionmodifyjob.h>
#include <kmessagebox.h>

ResourceStateInterface::Ptr ResourceState::createRetrieveItemState( ImapResource *resource,
                                                                    const Akonadi::Item &item,
                                                                    const QSet<QByteArray> &parts )
{
  ResourceState *state = new ResourceState( resource );

  state->m_item = item;
  state->m_parts = parts;

  return ResourceStateInterface::Ptr( state );
}

ResourceStateInterface::Ptr ResourceState::createRetrieveItemsState( ImapResource *resource,
                                                                     const Akonadi::Collection &collection )
{
  ResourceState *state = new ResourceState( resource );

  state->m_collection = collection;

  return ResourceStateInterface::Ptr( state );
}

ResourceStateInterface::Ptr ResourceState::createRetrieveCollectionsState( ImapResource *resource )
{
  return ResourceStateInterface::Ptr( new ResourceState( resource ) );
}

ResourceStateInterface::Ptr ResourceState::createRetrieveCollectionMetadataState( ImapResource *resource,
                                                                                  const Akonadi::Collection &collection )
{
  ResourceState *state = new ResourceState( resource );

  state->m_collection = collection;

  return ResourceStateInterface::Ptr( state );
}

ResourceStateInterface::Ptr ResourceState::createAddItemState( ImapResource *resource,
                                                               const Akonadi::Item &item,
                                                               const Akonadi::Collection &collection )
{
  ResourceState *state = new ResourceState( resource );

  state->m_item = item;
  state->m_collection = collection;

  return ResourceStateInterface::Ptr( state );
}

ResourceStateInterface::Ptr ResourceState::createChangeItemState( ImapResource *resource,
                                                                  const Akonadi::Item &item,
                                                                  const QSet<QByteArray> &parts )
{
  ResourceState *state = new ResourceState( resource );

  state->m_item = item;
  state->m_parts = parts;

  return ResourceStateInterface::Ptr( state );
}

ResourceStateInterface::Ptr ResourceState::createRemoveItemState( ImapResource *resource,
                                                                  const Akonadi::Item &item )
{
  ResourceState *state = new ResourceState( resource );

  state->m_item = item;

  return ResourceStateInterface::Ptr( state );
}

ResourceStateInterface::Ptr ResourceState::createMoveItemState( ImapResource *resource,
                                                                const Akonadi::Item &item,
                                                                const Akonadi::Collection &sourceCollection,
                                                                const Akonadi::Collection &targetCollection )
{
  ResourceState *state = new ResourceState( resource );

  state->m_item = item;
  state->m_sourceCollection = sourceCollection;
  state->m_targetCollection = targetCollection;

  return ResourceStateInterface::Ptr( state );
}

ResourceStateInterface::Ptr ResourceState::createAddCollectionState( ImapResource *resource,
                                                                     const Akonadi::Collection &collection,
                                                                     const Akonadi::Collection &parentCollection )
{
  ResourceState *state = new ResourceState( resource );

  state->m_collection = collection;
  state->m_parentCollection = parentCollection;

  return ResourceStateInterface::Ptr( state );
}

ResourceStateInterface::Ptr ResourceState::createChangeCollectionState( ImapResource *resource,
                                                                        const Akonadi::Collection &collection,
                                                                        const QSet<QByteArray> &parts )
{
  ResourceState *state = new ResourceState( resource );

  state->m_collection = collection;
  state->m_parts = parts;

  return ResourceStateInterface::Ptr( state );
}

ResourceStateInterface::Ptr ResourceState::createRemoveCollectionState( ImapResource *resource,
                                                                        const Akonadi::Collection &collection )
{
  ResourceState *state = new ResourceState( resource );

  state->m_collection = collection;

  return ResourceStateInterface::Ptr( state );
}

ResourceStateInterface::Ptr ResourceState::createMoveCollectionState( ImapResource *resource,
                                                                      const Akonadi::Collection &collection,
                                                                      const Akonadi::Collection &sourceCollection,
                                                                      const Akonadi::Collection &targetCollection )
{
  ResourceState *state = new ResourceState( resource );

  state->m_collection = collection;
  state->m_sourceCollection = sourceCollection;
  state->m_targetCollection = targetCollection;

  return ResourceStateInterface::Ptr( state );
}

ResourceStateInterface::Ptr ResourceState::createExpungeCollectionState( ImapResource *resource,
                                                                         const Akonadi::Collection &collection )
{
  ResourceState *state = new ResourceState( resource );

  state->m_collection = collection;

  return ResourceStateInterface::Ptr( state );
}

ResourceStateInterface::Ptr ResourceState::createIdleState( ImapResource *resource,
                                                            const Akonadi::Collection &collection )
{
  ResourceState *state = new ResourceState( resource );

  state->m_collection = collection;

  return ResourceStateInterface::Ptr( state );
}


ResourceState::ResourceState( ImapResource *resource )
  : m_resource( resource )
{

}

ResourceState::~ResourceState()
{

}

QString ResourceState::userName() const
{
  return m_resource->m_pool->account()->userName();
}

QString ResourceState::resourceName() const
{
  return m_resource->name();
}

QStringList ResourceState::serverCapabilities() const
{
  return m_resource->m_pool->serverCapabilities();
}

QList<KIMAP::MailBoxDescriptor> ResourceState::serverNamespaces() const
{
  return m_resource->m_pool->serverNamespaces();
}

bool ResourceState::isAutomaticExpungeEnabled() const
{
  return Settings::self()->automaticExpungeEnabled();
}

bool ResourceState::isSubscriptionEnabled() const
{
  return Settings::self()->subscriptionEnabled();
}

bool ResourceState::isDisconnectedModeEnabled() const
{
  return Settings::self()->disconnectedModeEnabled();
}

int ResourceState::intervalCheckTime() const
{
  if ( Settings::self()->intervalCheckEnabled() )
    return Settings::self()->intervalCheckTime();
  else
    return -1; // -1 for never
}

Akonadi::Collection ResourceState::collection() const
{
  return m_collection;
}

Akonadi::Item ResourceState::item() const
{
  return m_item;
}

Akonadi::Collection ResourceState::parentCollection() const
{
  return m_parentCollection;
}

Akonadi::Collection ResourceState::sourceCollection() const
{
  return m_sourceCollection;
}

Akonadi::Collection ResourceState::targetCollection() const
{
  return m_targetCollection;
}

QSet<QByteArray> ResourceState::parts() const
{
  return m_parts;
}

QString ResourceState::rootRemoteId() const
{
  return Settings::self()->rootRemoteId();
}

QString ResourceState::mailBoxForCollection( const Akonadi::Collection &collection, bool showWarnings ) const
{
  if ( collection.remoteId().isEmpty() ) { //This should never happen, investigate why a collection without remoteId made it this far
    if ( showWarnings )
      kWarning() << "Got incomplete ancestor chain due to empty remoteId:" << collection;
    return QString();
  }

  if ( collection.parentCollection() == Akonadi::Collection::root() ) {
    if ( showWarnings )
      kWarning( collection.remoteId() != rootRemoteId() ) << "RID mismatch, is " << collection.remoteId() << " expected " << rootRemoteId();
    return QString( "" );
  }
  const QString parentMailbox = mailBoxForCollection( collection.parentCollection() );
  if ( parentMailbox.isNull() ) // invalid, != isEmpty() here!
    return QString();

  const QString mailbox =  parentMailbox + collection.remoteId();
  if ( parentMailbox.isEmpty() )
    return mailbox.mid( 1 ); // strip of the separator on top-level mailboxes
  return mailbox;
}

void ResourceState::setIdleCollection( const Akonadi::Collection &collection )
{
  QStringList ridPath;

  Akonadi::Collection curCol = collection;
  while ( curCol != Akonadi::Collection::root() && !curCol.remoteId().isEmpty() ) {
    ridPath.append( curCol.remoteId() );
    curCol = curCol.parentCollection();
  }

  Settings::self()->setIdleRidPath( ridPath );
  Settings::self()->writeConfig();
}

void ResourceState::applyCollectionChanges( const Akonadi::Collection &collection )
{
  new Akonadi::CollectionModifyJob( collection );
}

void ResourceState::collectionAttributesRetrieved( const Akonadi::Collection &collection )
{
  m_resource->collectionAttributesRetrieved( collection );
}

void ResourceState::itemRetrieved( const Akonadi::Item &item )
{
  m_resource->itemRetrieved( item );
}

void ResourceState::itemsRetrieved( const Akonadi::Item::List &items )
{
  m_resource->itemsRetrieved( items );
}

void ResourceState::itemsRetrievedIncremental( const Akonadi::Item::List &changed, const Akonadi::Item::List &removed )
{
  m_resource->itemsRetrievedIncremental( changed, removed );
}

void ResourceState::itemsRetrievalDone()
{
  m_resource->itemsRetrievalDone();
}

void ResourceState::itemChangeCommitted( const Akonadi::Item &item )
{
  m_resource->changeCommitted( item );
}

void ResourceState::collectionsRetrieved( const Akonadi::Collection::List &collections )
{
  m_resource->collectionsRetrieved( collections );

  if ( Settings::self()->retrieveMetadataOnFolderListing() ) {
    QStringList oldMailBoxes = Settings::self()->knownMailBoxes();
    QStringList newMailBoxes;

    foreach ( const Akonadi::Collection &c, collections ) {
      const QString mailBox = mailBoxForCollection( c );

      if ( !c.hasAttribute<NoSelectAttribute>()
        && !oldMailBoxes.contains( mailBox ) ) {
        m_resource->scheduleCustomTask( m_resource,
                                        "triggerCollectionExtraInfoJobs",
                                        QVariant::fromValue( c ),
                                        Akonadi::ResourceBase::Append );
      }

      newMailBoxes << mailBox;
    }

    Settings::self()->setKnownMailBoxes( newMailBoxes );
  }

  m_resource->startIdleIfNeeded();
}

void ResourceState::collectionChangeCommitted( const Akonadi::Collection &collection )
{
  m_resource->changeCommitted( collection );
}

void ResourceState::changeProcessed()
{
  m_resource->changeProcessed();
}

void ResourceState::cancelTask( const QString &errorString )
{
  m_resource->cancelTask( errorString );

  // We get here in case of some error during the task. In such a case that can have
  // been provoked by the fact that some of the metadata we had was wrong (most notably
  // ACL and we took a wrong decision.
  // So reset the timestamp of all the collections involved in the task, and also
  // remove them from the "known mailboxes" list so that we get a chance to refresh
  // the metadata about them ASAP.

  Akonadi::Collection::List collections;
  collections << m_collection
              << m_parentCollection
              << m_sourceCollection
              << m_targetCollection;

  if ( m_item.isValid() && m_item.parentCollection().isValid() ) {
    collections << m_item.parentCollection();
  }

  if ( m_collection.isValid() && m_collection.parentCollection().isValid() ) {
    collections << m_collection.parentCollection();
  }

  const QStringList oldMailBoxes = Settings::self()->knownMailBoxes();
  QStringList newMailBoxes = oldMailBoxes;

  foreach ( const Akonadi::Collection &collection, collections ) {
    if ( collection.isValid()
      && collection.hasAttribute<TimestampAttribute>() ) {

      Akonadi::Collection c = collection;
      c.removeAttribute<TimestampAttribute>();

      new Akonadi::CollectionModifyJob( c );
      newMailBoxes.removeAll( mailBoxForCollection( c ) );
    }
  }

  if ( oldMailBoxes.size()!=newMailBoxes.size() ) {
    Settings::self()->setKnownMailBoxes( newMailBoxes );
  }
}

void ResourceState::deferTask()
{
  m_resource->deferTask();
}

void ResourceState::taskDone()
{
  m_resource->taskDone();
}

void ResourceState::emitError( const QString &message )
{
  emit m_resource->error( message );
}

void ResourceState::emitWarning( const QString &message )
{
  emit m_resource->warning( message );
}

void ResourceState::synchronizeCollectionTree()
{
  m_resource->synchronizeCollectionTree();
}

void ResourceState::scheduleConnectionAttempt()
{
  m_resource->scheduleConnectionAttempt();
}

void ResourceState::showInformationDialog( const QString &message, const QString &title, const QString &dontShowAgainName )
{
  KMessageBox::information( 0, message, title, dontShowAgainName );
}
