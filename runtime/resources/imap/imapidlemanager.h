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

#ifndef __IMAP_IDLEMANAGER_H__
#define __IMAP_IDLEMANAGER_H__

#include <akonadi/collection.h>
#include <QtCore/QObject>

#include "resourcestateinterface.h"

namespace KIMAP
{
  class IdleJob;
  class Session;
}

class ImapResource;
class SessionPool;

class KJob;

class ImapIdleManager : public QObject
{
  Q_OBJECT

public:
  ImapIdleManager( ResourceStateInterface::Ptr state, SessionPool *pool, ImapResource *parent );
  ~ImapIdleManager();

  KIMAP::Session *session() const;

private slots:
  void onSessionRequestDone( qint64 requestId, KIMAP::Session *session,
                             int errorCode, const QString &errorString );
  void onSelectDone( KJob *job );
  void onIdleStopped();
  void onStatsReceived( KIMAP::IdleJob *job, const QString &mailBox,
                        int messageCount, int recentCount );

private:
  qint64 m_sessionRequestId;
  SessionPool *m_pool;
  KIMAP::Session *m_session;
  KIMAP::IdleJob *m_idle;
  ImapResource *m_resource;
  ResourceStateInterface::Ptr m_state;
  qint64 m_lastMessageCount;
  qint64 m_lastRecentCount;
};

#endif
