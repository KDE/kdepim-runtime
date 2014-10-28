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

#include "resourcetask.h"

#include <akonadi/kmime/messageflags.h>

#include <KDE/KLocale>

#include "collectionflagsattribute.h"
#include "imapflags.h"
#include "sessionpool.h"
#include "resourcestateinterface.h"

ResourceTask::ResourceTask( ActionIfNoSession action, ResourceStateInterface::Ptr resource, QObject *parent )
  : QObject( parent ),
    m_pool( 0 ),
    m_sessionRequestId( 0 ),
    m_session( 0 ),
    m_actionIfNoSession( action ),
    m_resource( resource ),
    mCancelled( false )
{

}

ResourceTask::~ResourceTask()
{
  if ( m_pool ) {
    if ( m_sessionRequestId )
      m_pool->cancelSessionRequest( m_sessionRequestId );
    if ( m_session )
      m_pool->releaseSession( m_session );
  }
}

void ResourceTask::start( SessionPool *pool )
{
  m_pool = pool;
  connect( m_pool, SIGNAL(sessionRequestDone(qint64,KIMAP::Session*,int,QString)),
           this, SLOT(onSessionRequested(qint64,KIMAP::Session*,int,QString)) );

  m_sessionRequestId = m_pool->requestSession();

  if ( m_sessionRequestId <= 0 ) {
    m_sessionRequestId = 0;

    switch ( m_actionIfNoSession ) {
    case CancelIfNoSession:
      kDebug() << "Cancelling this request. Probably there is no connection.";
      m_resource->cancelTask( i18n( "There is currently no connection to the IMAP server." ) );
      break;

    case DeferIfNoSession:
      kDebug() << "Defering this request. Probably there is no connection.";
      m_resource->deferTask();
      break;
    }

    // In this case we were likely disconnect, try to get the resource online
    m_resource->scheduleConnectionAttempt();
    deleteLater();
  }
}

void ResourceTask::onSessionRequested( qint64 requestId, KIMAP::Session *session,
                                       int errorCode, const QString &/*errorString*/ )
{
  if ( requestId!=m_sessionRequestId ) {
    // Not for us, ignore
    return;
  }

  disconnect( m_pool, SIGNAL(sessionRequestDone(qint64,KIMAP::Session*,int,QString)),
              this, SLOT(onSessionRequested(qint64,KIMAP::Session*,int,QString)) );
  m_sessionRequestId = 0;

  if ( errorCode!=SessionPool::NoError ) {
    switch ( m_actionIfNoSession ) {
    case CancelIfNoSession:
      kDebug() << "Cancelling this request. Probably there is no more session available.";
      m_resource->cancelTask( i18n( "There is currently no session to the IMAP server available." ) );
      break;

    case DeferIfNoSession:
      kDebug() << "Defering this request. Probably there is no more session available.";
      m_resource->deferTask();
      break;
    }

    deleteLater();
    return;
  }

  m_session = session;

  connect( m_pool, SIGNAL(connectionLost(KIMAP::Session*)),
           this, SLOT(onConnectionLost(KIMAP::Session*)) );
  connect( m_pool, SIGNAL(disconnectDone()),
           this, SLOT(onPoolDisconnect()) );

  doStart( m_session );
}

void ResourceTask::onConnectionLost( KIMAP::Session *session )
{
  if ( session == m_session ) {
    // Our session becomes invalid, so get rid of
    // the pointer, we don't need to release it once the
    // task is done
    m_session = 0;
    cancelTask( i18n( "Connection lost" ) );
  }
}

void ResourceTask::onPoolDisconnect()
{
  // All the sessions in the pool we used changed,
  // so get rid of the pointer, we don't need to
  // release our session anymore
  m_pool = 0;

  cancelTask( i18n( "Connection lost" ) );
}

QString ResourceTask::userName() const
{
  return m_resource->userName();
}

QString ResourceTask::resourceName() const
{
  return m_resource->resourceName();
}

QStringList ResourceTask::serverCapabilities() const
{
  return m_resource->serverCapabilities();
}

QList<KIMAP::MailBoxDescriptor> ResourceTask::serverNamespaces() const
{
  return m_resource->serverNamespaces();
}

bool ResourceTask::isAutomaticExpungeEnabled() const
{
  return m_resource->isAutomaticExpungeEnabled();
}

bool ResourceTask::isSubscriptionEnabled() const
{
  return m_resource->isSubscriptionEnabled();
}

bool ResourceTask::isDisconnectedModeEnabled() const
{
  return m_resource->isDisconnectedModeEnabled();
}

int ResourceTask::intervalCheckTime() const
{
  return m_resource->intervalCheckTime();
}

static Akonadi::Collection detatchCollection(const Akonadi::Collection &collection)
{
  //HACK: Attributes are accessed via a const function, and the implicitly shared private pointer thus doesn't detach.
  //We force a detach to avoid surprises. (RetrieveItemsTask used to write back the collection changes, even though the task was canceled)
  //Once this is fixed this function can go away.
  Akonadi::Collection col = collection;
  col.setId(col.id());
  return col;
}

Akonadi::Collection ResourceTask::collection() const
{
  return detatchCollection(m_resource->collection());
}

Akonadi::Item ResourceTask::item() const
{
  return m_resource->item();
}

Akonadi::Item::List ResourceTask::items() const
{
  return m_resource->items();
}

Akonadi::Collection ResourceTask::parentCollection() const
{
  return detatchCollection(m_resource->parentCollection());
}

Akonadi::Collection ResourceTask::sourceCollection() const
{
  return detatchCollection(m_resource->sourceCollection());
}

Akonadi::Collection ResourceTask::targetCollection() const
{
  return detatchCollection(m_resource->targetCollection());
}

QSet<QByteArray> ResourceTask::parts() const
{
  return m_resource->parts();
}

QSet< QByteArray > ResourceTask::addedFlags() const
{
  return m_resource->addedFlags();
}

QSet< QByteArray > ResourceTask::removedFlags() const
{
  return m_resource->removedFlags();
}

QString ResourceTask::rootRemoteId() const
{
  return m_resource->rootRemoteId();
}

QString ResourceTask::mailBoxForCollection( const Akonadi::Collection &collection ) const
{
  return m_resource->mailBoxForCollection( collection );
}

void ResourceTask::setIdleCollection( const Akonadi::Collection &collection )
{
  if (!mCancelled) {
    m_resource->setIdleCollection( collection );
  }
}

void ResourceTask::applyCollectionChanges( const Akonadi::Collection &collection )
{
  if (!mCancelled) {
    m_resource->applyCollectionChanges( collection );
  }
}

void ResourceTask::itemRetrieved( const Akonadi::Item &item )
{
  if (!mCancelled) {
    m_resource->itemRetrieved( item );
    emitPercent(100);
  }
  deleteLater();
}

void ResourceTask::itemsRetrieved( const Akonadi::Item::List &items )
{
  if (!mCancelled) {
    m_resource->itemsRetrieved( items );
  }
}

void ResourceTask::itemsRetrievedIncremental( const Akonadi::Item::List &changed,
                                              const Akonadi::Item::List &removed )
{
  if (!mCancelled) {
    m_resource->itemsRetrievedIncremental( changed, removed );
  }
}

void ResourceTask::itemsRetrievalDone()
{
  if (!mCancelled) {
    m_resource->itemsRetrievalDone();
  }
  deleteLater();
}

void ResourceTask::setTotalItems(int totalItems)
{
  if (!mCancelled) {
    m_resource->setTotalItems(totalItems);
  }
}

void ResourceTask::changeCommitted( const Akonadi::Item &item )
{
  if (!mCancelled) {
    m_resource->itemChangeCommitted( item );
  }
  deleteLater();
}

void ResourceTask::changesCommitted(const Akonadi::Item::List& items)
{
  if (!mCancelled) {
    m_resource->itemsChangesCommitted( items );
  }
  deleteLater();
}

void ResourceTask::searchFinished( const QVector<qint64> &result, bool isRid )
{
  if (!mCancelled) {
    m_resource->searchFinished( result, isRid );
  }
  deleteLater();
}

void ResourceTask::collectionsRetrieved( const Akonadi::Collection::List &collections )
{
  if (!mCancelled) {
    m_resource->collectionsRetrieved( collections );
  }
  deleteLater();
}

void ResourceTask::collectionAttributesRetrieved(const Akonadi::Collection& col)
{
  if (!mCancelled) {
    m_resource->collectionAttributesRetrieved( col );
  }
  deleteLater();
}

void ResourceTask::changeCommitted( const Akonadi::Collection &collection )
{
  if (!mCancelled) {
    m_resource->collectionChangeCommitted( collection );
  }
  deleteLater();
}

void ResourceTask::changeProcessed()
{
  if (!mCancelled) {
    m_resource->changeProcessed();
  }
  deleteLater();
}

void ResourceTask::cancelTask( const QString &errorString )
{
  if (!mCancelled) {
    mCancelled = true;
    m_resource->cancelTask( errorString );
  }
  deleteLater();
}

void ResourceTask::deferTask()
{
  if (!mCancelled) {
    mCancelled = true;
    m_resource->deferTask();
  }
  deleteLater();
}

void ResourceTask::restartItemRetrieval(Akonadi::Entity::Id col)
{
  if (!mCancelled) {
    m_resource->restartItemRetrieval(col);
  }
  deleteLater();
}

void ResourceTask::taskDone()
{
  m_resource->taskDone();
  deleteLater();
}

void ResourceTask::emitPercent( int percent )
{
  m_resource->emitPercent( percent );
}

void ResourceTask::emitError( const QString &message )
{
  m_resource->emitError( message );
}

void ResourceTask::emitWarning( const QString &message )
{
  m_resource->emitWarning( message );
}

void ResourceTask::synchronizeCollectionTree()
{
  m_resource->synchronizeCollectionTree();
}

void ResourceTask::showInformationDialog( const QString &message, const QString &title, const QString &dontShowAgainName )
{
  m_resource->showInformationDialog( message, title, dontShowAgainName );
}

QList<QByteArray> ResourceTask::fromAkonadiToSupportedImapFlags( const QList<QByteArray> &flags,
                                                                 const Akonadi::Collection &collection )
{
  QList<QByteArray> imapFlags = fromAkonadiFlags( flags );

  const Akonadi::CollectionFlagsAttribute *flagAttr = collection.attribute<Akonadi::CollectionFlagsAttribute>();
  // the server does not support arbitrary flags, so filter out those it can't handle
  if ( flagAttr && !flagAttr->flags().isEmpty() && !flagAttr->flags().contains( "\\*" ) ) {
    for ( QList< QByteArray >::iterator it = imapFlags.begin(); it != imapFlags.end(); ) {
      if ( flagAttr->flags().contains( *it ) ) {
        ++it;
      } else {
        kDebug() << "Server does not support flag" << *it;
        it = imapFlags.erase( it );
      }
    }
  }

  return imapFlags;
}

QList<QByteArray> ResourceTask::fromAkonadiFlags( const QList<QByteArray> &flags )
{
  QList<QByteArray> newFlags;

  foreach ( const QByteArray &oldFlag, flags ) {
    if ( oldFlag == Akonadi::MessageFlags::Seen ) {
      newFlags.append( ImapFlags::Seen );
    } else if ( oldFlag == Akonadi::MessageFlags::Deleted ) {
      newFlags.append( ImapFlags::Deleted );
    } else if ( oldFlag == Akonadi::MessageFlags::Answered || oldFlag == Akonadi::MessageFlags::Replied ) {
      newFlags.append( ImapFlags::Answered );
    } else if ( oldFlag == Akonadi::MessageFlags::Flagged ) {
      newFlags.append( ImapFlags::Flagged );
    } else {
      newFlags.append( oldFlag );
    }
  }

  return newFlags;
}

QList<QByteArray> ResourceTask::toAkonadiFlags( const QList<QByteArray> &flags )
{
  QList<QByteArray> newFlags;

  foreach ( const QByteArray &oldFlag, flags ) {
    if ( oldFlag == ImapFlags::Seen ) {
      newFlags.append( Akonadi::MessageFlags::Seen );
    } else if ( oldFlag == ImapFlags::Deleted ) {
      newFlags.append( Akonadi::MessageFlags::Deleted );
    } else if ( oldFlag == ImapFlags::Answered ) {
      newFlags.append( Akonadi::MessageFlags::Answered );
    } else if ( oldFlag == ImapFlags::Flagged ) {
      newFlags.append( Akonadi::MessageFlags::Flagged );
    } else if ( oldFlag.isEmpty() ) {
      // filter out empty flags, to avoid isNull/isEmpty confusions higher up
      continue;
    } else {
      newFlags.append( oldFlag );
    }
  }

  return newFlags;
}

void ResourceTask::kill()
{
  kDebug();
  cancelTask(i18n("killed"));
}

const QChar ResourceTask::separatorCharacter() const
{
  const QChar separator = m_resource->separatorCharacter();
  if ( !separator.isNull() ) {
    return separator;
  } else {
    //If we request the separator before first folder listing, then try to guess
    //the separator:
    //If we create a toplevel folder, assume the separator to be '/'. This is not perfect, but detecting the right
    //IMAP separator is not straightforward for toplevel folders, and fixes bug 292418 and maybe other, where
    //subfolders end up with remote id's starting with "i" (the first letter of imap:// ...)

    QString remoteId;
    // We don't always have parent collection set (for example for CollectionChangeTask),
    // in such cases however we can use current collection's remoteId to get the separator
    const Akonadi::Collection parent = parentCollection();
    if ( parent.isValid() ) {
        remoteId = parent.remoteId();
    } else {
        remoteId = collection().remoteId();
    }
    return ( ( remoteId != rootRemoteId() ) && !remoteId.isEmpty() ) ? remoteId.at( 0 ) : QLatin1Char('/');
  }
}

void ResourceTask::setSeparatorCharacter( const QChar& separator )
{
    m_resource->setSeparatorCharacter( separator );
}

bool ResourceTask::serverSupportsAnnotations() const
{
    return serverCapabilities().contains( QLatin1String( "METADATA" ) )
            || serverCapabilities().contains( QLatin1String( "ANNOTATEMORE" ) );
}

bool ResourceTask::serverSupportsCondstore() const
{
    // Don't enable CONDSTORE for GMail (X-GM-EXT-1 is a GMail-specific capability)
    // because it breaks changes synchronization when using labels.
    return serverCapabilities().contains( QLatin1String( "CONDSTORE" ) ) &&
            !serverCapabilities().contains( QLatin1String( "X-GM-EXT-1" ) );
}

int ResourceTask::batchSize() const
{
    return m_resource->batchSize();
}

ResourceStateInterface::Ptr ResourceTask::resourceState()
{
    return m_resource;
}
