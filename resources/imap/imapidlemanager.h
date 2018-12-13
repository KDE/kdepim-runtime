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

#ifndef RESOURCES_IMAP_IMAPIDLEMANAGER_H
#define RESOURCES_IMAP_IMAPIDLEMANAGER_H

#include "resourcestateinterface.h"

#include <collection.h>

#include <QObject>
#include <QPointer>

namespace KIMAP {
class IdleJob;
class Session;
}

class ImapResourceBase;
class SessionPool;

class KJob;

class QTimer;

class ImapIdleManager : public QObject
{
    Q_OBJECT

public:
    ImapIdleManager(ResourceStateInterface::Ptr state, SessionPool *pool, ImapResourceBase *parent);
    ~ImapIdleManager();
    void stop();

    KIMAP::Session *session() const;

private Q_SLOTS:
    void onConnectionLost(KIMAP::Session *session);
    void onPoolDisconnect();

    void onSessionRequestDone(qint64 requestId, KIMAP::Session *session, int errorCode, const QString &errorString);
    void onSelectDone(KJob *job);
    void onIdleStopped();
    void onStatsReceived(KIMAP::IdleJob *job, const QString &mailBox, int messageCount, int recentCount);
    void onFlagsChanged(KIMAP::IdleJob *job);
    void reconnect();
    void restartIdle();
private:
    void startIdle();

    qint64 m_sessionRequestId;
    SessionPool *m_pool = nullptr;
    KIMAP::Session *m_session = nullptr;
    QPointer<KIMAP::IdleJob> m_idle;
    ImapResourceBase *m_resource = nullptr;
    ResourceStateInterface::Ptr m_state;
    QTimer *m_idleTimeout = nullptr;
    qint64 m_lastMessageCount = -1;
    qint64 m_lastRecentCount = -1;
};

#endif
