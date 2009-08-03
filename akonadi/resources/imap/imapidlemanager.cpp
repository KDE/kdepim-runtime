/*
    Copyright (c) 2007 Till Adam <adam@kde.org>
    Copyright (C) 2008 Omat Holding B.V. <info@omat.nl>
    Copyright (C) 2009 Kevin Ottens <ervin@kde.org>

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

#include "imapidlemanager.h"

#include <akonadi/collectionfetchjob.h>
#include <akonadi/collectionfetchscope.h>
#include <akonadi/collectionstatistics.h>

#include <KDE/KDebug>

#include <kimap/idlejob.h>
#include <kimap/selectjob.h>
#include <kimap/session.h>

#include "imapresource.h"

ImapIdleManager::ImapIdleManager( KIMAP::Session *session, ImapResource *parent )
  : QObject( parent ), m_session( session ), m_idle( 0 ), m_resource( parent )
{
  KIMAP::SelectJob *select = new KIMAP::SelectJob( m_session );
  select->setMailBox( "INBOX" );
  select->start();

  m_idle = new KIMAP::IdleJob( m_session );
  connect( m_idle, SIGNAL(mailBoxStats(KIMAP::IdleJob*, QString, int, int)),
           this, SLOT(onStatsReceived(KIMAP::IdleJob*, QString, int, int)) );
  connect( m_idle, SIGNAL(result(KJob*)),
           this, SLOT(onIdleStopped()) );
  m_idle->start();
}

ImapIdleManager::~ImapIdleManager()
{
}

KIMAP::Session * ImapIdleManager::session() const
{
  return m_session;
}

void ImapIdleManager::onIdleStopped()
{
  kDebug() << "IDLE dropped maybe we should reconnect?";
  if ( m_resource->isOnline() ) {
    kDebug() << "Reconnecting!";
    m_resource->setOnline( false );
    m_resource->setOnline( true );
  }
}

void ImapIdleManager::onStatsReceived(KIMAP::IdleJob *job, const QString &mailBox,
                                      int messageCount, int recentCount)
{
  kDebug() << "IDLE stats received:" << job << mailBox << messageCount << recentCount;

  Akonadi::Collection c;
  c.setRemoteId( m_resource->remoteIdForMailBox( mailBox ) );

  Akonadi::CollectionFetchScope scope;
  scope.setIncludeStatistics( true );
  scope.setResource( m_resource->identifier() );

  Akonadi::CollectionFetchJob *fetch
    = new Akonadi::CollectionFetchJob( c, Akonadi::CollectionFetchJob::Base, this );
  fetch->setFetchScope( scope );
  fetch->setProperty( "mailBox", mailBox );
  fetch->setProperty( "messageCount", messageCount );
  fetch->setProperty( "recentCount", recentCount );
  connect( fetch, SIGNAL(result(KJob*)), this, SLOT(onCollectionFetchDone(KJob*)) );
}

void ImapIdleManager::onCollectionFetchDone( KJob *job )
{
  QString mailBox = job->property( "mailBox" ).toString();
  int messageCount = job->property( "messageCount" ).toInt();
  int recentCount = job->property( "recentCount" ).toInt();

  Akonadi::CollectionFetchJob *fetch = static_cast<Akonadi::CollectionFetchJob*>( job );

  if ( fetch->error() == 0 ) {
    // Get collection from job
    Akonadi::Collection c = fetch->collections().first();
    Akonadi::CollectionStatistics stats = c.statistics();

    // It seems we're not in sync with the cache, resync is needed
    if ( messageCount!=stats.count() || recentCount!=stats.unreadCount() ) {
      kDebug() << "Resync needed for" << mailBox << c.id();
      m_resource->synchronizeCollection( c.id() );
    }
  } else {
    kError() << "CollectionFetch for mail box " << mailBox << "failed. error="
             << job->error() << ", errorString=" << job->errorString();
  }
}

#include "imapidlemanager.moc"


