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

#include <akonadi/agentsearchinterface.h>
#include <kmessagebox.h>

ResourceState::ResourceState( ImapResourceBase *resource, const TaskArguments &args )
  : m_resource( resource ),
  m_arguments( args )
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
  return m_resource->settings()->automaticExpungeEnabled();
}

bool ResourceState::isSubscriptionEnabled() const
{
  return m_resource->settings()->subscriptionEnabled();
}

bool ResourceState::isDisconnectedModeEnabled() const
{
  return m_resource->settings()->disconnectedModeEnabled();
}

int ResourceState::intervalCheckTime() const
{
  if ( m_resource->settings()->intervalCheckEnabled() )
    return m_resource->settings()->intervalCheckTime();
  else
    return -1; // -1 for never
}

Akonadi::Collection ResourceState::collection() const
{
  return m_arguments.collection;
}

Akonadi::Item ResourceState::item() const
{
  if (m_arguments.items.count() > 1) {
    kWarning() << "Called item() while state holds multiple items!";
  }

  return m_arguments.items.first();
}

Akonadi::Item::List ResourceState::items() const
{
  return m_arguments.items;
}

Akonadi::Collection ResourceState::parentCollection() const
{
  return m_arguments.parentCollection;
}

Akonadi::Collection ResourceState::sourceCollection() const
{
  return m_arguments.sourceCollection;
}

Akonadi::Collection ResourceState::targetCollection() const
{
  return m_arguments.targetCollection;
}

QSet<QByteArray> ResourceState::parts() const
{
  return m_arguments.parts;
}

QSet<QByteArray> ResourceState::addedFlags() const
{
  return m_arguments.addedFlags;
}

QSet<QByteArray> ResourceState::removedFlags() const
{
  return m_arguments.removedFlags;
}

QString ResourceState::rootRemoteId() const
{
  return m_resource->settings()->rootRemoteId();
}

void ResourceState::setIdleCollection( const Akonadi::Collection &collection )
{
  QStringList ridPath;

  Akonadi::Collection curCol = collection;
  while ( curCol != Akonadi::Collection::root() && !curCol.remoteId().isEmpty() ) {
    ridPath.append( curCol.remoteId() );
    curCol = curCol.parentCollection();
  }

  m_resource->settings()->setIdleRidPath( ridPath );
  m_resource->settings()->writeConfig();
}

void ResourceState::applyCollectionChanges( const Akonadi::Collection &collection )
{
  m_resource->modifyCollection(collection);
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
  emitPercent(100);
}

void ResourceState::setTotalItems(int items)
{
  m_resource->setTotalItems(items);
}

void ResourceState::itemChangeCommitted( const Akonadi::Item &item )
{
  m_resource->changeCommitted( item );
}

void ResourceState::itemsChangesCommitted(const Akonadi::Item::List& items)
{
  m_resource->changesCommitted( items );
}

void ResourceState::collectionsRetrieved( const Akonadi::Collection::List &collections )
{
  m_resource->collectionsRetrieved( collections );

  if ( m_resource->settings()->retrieveMetadataOnFolderListing() ) {
    QStringList oldMailBoxes = m_resource->settings()->knownMailBoxes();
    QStringList newMailBoxes;

    foreach ( const Akonadi::Collection &c, collections ) {
      const QString mailBox = mailBoxForCollection( c );

      if ( !c.hasAttribute<NoSelectAttribute>()
        && !oldMailBoxes.contains( mailBox ) ) {
        m_resource->synchronizeCollectionAttributes(c.id());
      }

      newMailBoxes << mailBox;
    }

    m_resource->settings()->setKnownMailBoxes( newMailBoxes );
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

void ResourceState::searchFinished( const QVector<qint64> &result, bool isRid )
{
  m_resource->searchFinished( result, isRid ? Akonadi::AgentSearchInterface::Rid :
                                              Akonadi::AgentSearchInterface::Uid );
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
  collections << m_arguments.collection
              << m_arguments.parentCollection
              << m_arguments.sourceCollection
              << m_arguments.targetCollection;

  foreach ( const Akonadi::Item &item, m_arguments.items ) {
    if  ( item.isValid() && item.parentCollection().isValid() ) {
        collections << item.parentCollection();
    }
  }

  if ( m_arguments.collection.isValid() && m_arguments.collection.parentCollection().isValid() ) {
    collections << m_arguments.collection.parentCollection();
  }

  const QStringList oldMailBoxes = m_resource->settings()->knownMailBoxes();
  QStringList newMailBoxes = oldMailBoxes;

  foreach ( const Akonadi::Collection &collection, collections ) {
    if ( collection.isValid()
      && collection.hasAttribute<TimestampAttribute>() ) {

      Akonadi::Collection c = collection;
      c.removeAttribute<TimestampAttribute>();

      m_resource->modifyCollection( c );
      newMailBoxes.removeAll( mailBoxForCollection( c ) );
    }
  }

  if ( oldMailBoxes.size()!=newMailBoxes.size() ) {
    m_resource->settings()->setKnownMailBoxes( newMailBoxes );
  }
}

void ResourceState::deferTask()
{
  m_resource->deferTask();
}

void ResourceState::restartItemRetrieval(Akonadi::Collection::Id col)
{
  //This ensures the collection fetch job is rerun (it isn't when using deferTask)
  //The task will be appended
  //TODO: deferTask should rerun the collectionfetchjob
  m_resource->synchronizeCollection(col);
  cancelTask("Restarting item retrieval.");
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

void ResourceState::emitPercent( int percent )
{
  emit m_resource->percent( percent );
}

void ResourceState::synchronizeCollection(Akonadi::Entity::Id id)
{
    m_resource->synchronizeCollection(id);
}

void ResourceState::synchronizeCollectionTree()
{
  m_resource->synchronizeCollectionTree();
}

void ResourceState::scheduleConnectionAttempt()
{
  m_resource->scheduleConnectionAttempt();
}

QChar ResourceState::separatorCharacter() const
{
  return m_resource->separatorCharacter();
}

void ResourceState::setSeparatorCharacter( const QChar &separator )
{
  m_resource->setSeparatorCharacter( separator );
}

void ResourceState::showInformationDialog( const QString &message, const QString &title, const QString &dontShowAgainName )
{
  KMessageBox::information( 0, message, title, dontShowAgainName );
}

int ResourceState::batchSize() const
{
  return m_resource->itemSyncBatchSize();
}

MessageHelper::Ptr ResourceState::messageHelper() const
{
  return MessageHelper::Ptr(new MessageHelper());
}
