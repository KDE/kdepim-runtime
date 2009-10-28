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

#include <KDE/KDebug>

#include <kimap/idlejob.h>
#include <kimap/selectjob.h>
#include <kimap/session.h>

#include "imapresource.h"

ImapIdleManager::ImapIdleManager( Akonadi::Collection &col, const QString &mailBox,
                                  KIMAP::Session *session, ImapResource *parent )
  : QObject( parent ), m_session( session ),
    m_idle( 0 ), m_resource( parent ),  m_collection( col ),
    m_lastMessageCount( -1 ), m_lastRecentCount( -1 )
{
  KIMAP::SelectJob *select = new KIMAP::SelectJob( m_session );
  select->setMailBox( mailBox );
  connect( select, SIGNAL(result(KJob*)),
           this, SLOT(onSelectDone(KJob*)) );
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

KIMAP::Session *ImapIdleManager::session() const
{
  return m_session;
}

void ImapIdleManager::onSelectDone( KJob *job )
{
  KIMAP::SelectJob *select = static_cast<KIMAP::SelectJob*>( job );

  m_lastMessageCount = select->messageCount();
  m_lastRecentCount = select->recentCount();
}

void ImapIdleManager::onIdleStopped()
{
  kDebug(5327) << "IDLE dropped maybe we should reconnect?";
  if ( m_resource->isOnline() ) {
    kDebug(5327) << "Reconnecting!";
    m_resource->setOnline( false );
    m_resource->setOnline( true );
  }
}

void ImapIdleManager::onStatsReceived(KIMAP::IdleJob *job, const QString &mailBox,
                                      int messageCount, int recentCount)
{
  kDebug(5327) << "IDLE stats received:" << job << mailBox << messageCount << recentCount;
  kDebug(5327) << "Cached information:" << m_collection.remoteId() << m_collection.id()
           << m_lastMessageCount << m_lastRecentCount;

  // It seems we're not in sync with the cache, resync is needed
  if ( messageCount!=m_lastMessageCount || recentCount!=m_lastRecentCount ) {
    m_lastMessageCount = messageCount;
    m_lastRecentCount = recentCount;

    kDebug(5327) << "Resync needed for" << mailBox << m_collection.id();
    m_resource->synchronizeCollection( m_collection.id() );
  }
}

#include "imapidlemanager.moc"


