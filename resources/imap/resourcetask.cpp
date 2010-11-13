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

#include "imapflags.h"
#include "sessionpool.h"
#include "resourcestateinterface.h"

ResourceTask::ResourceTask( ActionIfNoSession action, ResourceStateInterface::Ptr resource, QObject *parent )
  : QObject( parent ),
    m_pool( 0 ),
    m_sessionRequestId( 0 ),
    m_session( 0 ),
    m_actionIfNoSession( action ),
    m_resource( resource )
{

}

ResourceTask::~ResourceTask()
{
  if ( m_pool && m_session ) {
    m_pool->releaseSession( m_session );
  }
}

void ResourceTask::start( SessionPool *pool )
{
  m_pool = pool;
  connect( m_pool, SIGNAL(sessionRequestDone(qint64, KIMAP::Session*, int, QString)),
           this, SLOT(onSessionRequested(qint64, KIMAP::Session*, int, QString)) );

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

  disconnect( m_pool, SIGNAL(sessionRequestDone(qint64, KIMAP::Session*, int, QString)),
              this, SLOT(onSessionRequested(qint64, KIMAP::Session*, int, QString)) );
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
    // Our session becomes invalid, so get ride of
    // the pointer, we don't need to release it once the
    // task is done
    m_session = 0;
  }
}

void ResourceTask::onPoolDisconnect()
{
  // All the sessions in the pool we used changed,
  // so get ride of the pointer, we don't need to
  // release our session anymore
  m_pool = 0;
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

Akonadi::Collection ResourceTask::collection() const
{
  return m_resource->collection();
}

Akonadi::Item ResourceTask::item() const
{
  return m_resource->item();
}

Akonadi::Collection ResourceTask::parentCollection() const
{
  return m_resource->parentCollection();
}

Akonadi::Collection ResourceTask::sourceCollection() const
{
  return m_resource->sourceCollection();
}

Akonadi::Collection ResourceTask::targetCollection() const
{
  return m_resource->targetCollection();
}

QSet<QByteArray> ResourceTask::parts() const
{
  return m_resource->parts();
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
  m_resource->setIdleCollection( collection );
}

void ResourceTask::applyCollectionChanges( const Akonadi::Collection &collection )
{
  m_resource->applyCollectionChanges( collection );
}

void ResourceTask::collectionAttributesRetrieved( const Akonadi::Collection &collection )
{
  m_resource->collectionAttributesRetrieved( collection );
  deleteLater();
}

void ResourceTask::itemRetrieved( const Akonadi::Item &item )
{
  m_resource->itemRetrieved( item );
  deleteLater();
}

void ResourceTask::itemsRetrieved( const Akonadi::Item::List &items )
{
  m_resource->itemsRetrieved( items );
}

void ResourceTask::itemsRetrievedIncremental( const Akonadi::Item::List &changed,
                                              const Akonadi::Item::List &removed )
{
  m_resource->itemsRetrievedIncremental( changed, removed );
}

void ResourceTask::itemsRetrievalDone()
{
  m_resource->itemsRetrievalDone();
  deleteLater();
}

void ResourceTask::changeCommitted( const Akonadi::Item &item )
{
  m_resource->itemChangeCommitted( item );
  deleteLater();
}

void ResourceTask::collectionsRetrieved( const Akonadi::Collection::List &collections )
{
  m_resource->collectionsRetrieved( collections );
  deleteLater();
}

void ResourceTask::changeCommitted( const Akonadi::Collection &collection )
{
  m_resource->collectionChangeCommitted( collection );
  deleteLater();
}

void ResourceTask::changeProcessed()
{
  m_resource->changeProcessed();
  deleteLater();
}

void ResourceTask::cancelTask( const QString &errorString )
{
  m_resource->cancelTask( errorString );
  deleteLater();
}

void ResourceTask::deferTask()
{
  m_resource->deferTask();
  deleteLater();
}

void ResourceTask::taskDone()
{
  m_resource->taskDone();
  deleteLater();
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

QList<QByteArray> ResourceTask::fromAkonadiFlags( const QList<QByteArray> &flags )
{
  QList<QByteArray> newFlags;

  foreach ( const QByteArray &oldFlag, flags ) {
    if( oldFlag == Akonadi::MessageFlags::Seen ) {
      newFlags.append( ImapFlags::Seen );
    } else if( oldFlag == Akonadi::MessageFlags::Deleted ) {
      newFlags.append( ImapFlags::Deleted );
    } else if( oldFlag == Akonadi::MessageFlags::Answered ) {
      newFlags.append( ImapFlags::Answered );
    } else if( oldFlag == Akonadi::MessageFlags::Flagged ) {
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
    if( oldFlag == ImapFlags::Seen ) {
      newFlags.append( Akonadi::MessageFlags::Seen );
    } else if( oldFlag == ImapFlags::Deleted ) {
      newFlags.append( Akonadi::MessageFlags::Deleted );
    } else if( oldFlag == ImapFlags::Answered ) {
      newFlags.append( Akonadi::MessageFlags::Answered );
    } else if( oldFlag == ImapFlags::Flagged ) {
      newFlags.append( Akonadi::MessageFlags::Flagged );
    } else if( oldFlag.isEmpty() ) {
      // filter out empty flags, to avoid isNull/isEmpty confusions higher up
      continue;
    } else {
      newFlags.append( oldFlag );
    }
  }

  return newFlags;
}

#include "resourcetask.moc"


