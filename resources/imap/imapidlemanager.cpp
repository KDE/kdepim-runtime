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

#include "imapidlemanager.h"

#include "imapresource_debug.h"

#include <kimap/idlejob.h>
#include <kimap/selectjob.h>
#include <kimap/session.h>

#include <QTimer>

#include "imapresource.h"
#include "sessionpool.h"

ImapIdleManager::ImapIdleManager(ResourceStateInterface::Ptr state,
                                 SessionPool *pool, ImapResourceBase *parent)
    : QObject(parent), m_sessionRequestId(0), m_pool(pool), m_session(nullptr),
      m_idle(nullptr), m_resource(parent), m_state(state),
      m_lastMessageCount(-1), m_lastRecentCount(-1)
{
    connect(pool, &SessionPool::sessionRequestDone, this, &ImapIdleManager::onSessionRequestDone);
    m_sessionRequestId = m_pool->requestSession();
}

ImapIdleManager::~ImapIdleManager()
{
    stop();
    if (m_pool) {
        if (m_sessionRequestId) {
            m_pool->cancelSessionRequest(m_sessionRequestId);
        }
        if (m_session) {
            m_pool->releaseSession(m_session);
        }
    }
}

void ImapIdleManager::stop()
{
    if (m_idle) {
        m_idle->stop();
        disconnect(m_idle, nullptr, this, nullptr);
        m_idle = nullptr;
    }
    if (m_pool) {
        disconnect(m_pool, nullptr, this, nullptr);
    }
}

KIMAP::Session *ImapIdleManager::session() const
{
    return m_session;
}

void ImapIdleManager::reconnect()
{
    qCDebug(IMAPRESOURCE_LOG) << "attempting to reconnect IDLE session";
    if (m_session == nullptr && m_pool->isConnected() && m_sessionRequestId == 0) {
        m_sessionRequestId = m_pool->requestSession();
    }
}

void ImapIdleManager::onSessionRequestDone(qint64 requestId, KIMAP::Session *session,
        int errorCode, const QString &/*errorString*/)
{
    if (requestId != m_sessionRequestId || session == nullptr || errorCode != SessionPool::NoError) {
        return;
    }

    m_session = session;
    m_sessionRequestId = 0;

    connect(m_pool, &SessionPool::connectionLost, this, &ImapIdleManager::onConnectionLost);
    connect(m_pool, &SessionPool::disconnectDone, this, &ImapIdleManager::onPoolDisconnect);

    KIMAP::SelectJob *select = new KIMAP::SelectJob(m_session);
    select->setMailBox(m_state->mailBoxForCollection(m_state->collection()));
    connect(select, &KIMAP::SelectJob::result, this, &ImapIdleManager::onSelectDone);
    select->start();

    m_idle = new KIMAP::IdleJob(m_session);
    connect(m_idle.data(), &KIMAP::IdleJob::mailBoxStats, this, &ImapIdleManager::onStatsReceived);
    connect(m_idle.data(), &KIMAP::IdleJob::mailBoxMessageFlagsChanged, this, &ImapIdleManager::onFlagsChanged);
    connect(m_idle.data(), &KIMAP::IdleJob::result, this, &ImapIdleManager::onIdleStopped);
    m_idle->start();
}

void ImapIdleManager::onConnectionLost(KIMAP::Session *session)
{
    if (session == m_session) {
        // Our session becomes invalid, so get ride of
        // the pointer, we don't need to release it once the
        // task is done
        m_session = nullptr;
        QMetaObject::invokeMethod(this, "reconnect", Qt::QueuedConnection);
    }
}

void ImapIdleManager::onPoolDisconnect()
{
    // All the sessions in the pool we used changed,
    // so get ride of the pointer, we don't need to
    // release our session anymore
    m_pool = nullptr;
}

void ImapIdleManager::onSelectDone(KJob *job)
{
    KIMAP::SelectJob *select = static_cast<KIMAP::SelectJob *>(job);

    m_lastMessageCount = select->messageCount();
    m_lastRecentCount = select->recentCount();
}

void ImapIdleManager::onIdleStopped()
{
    qCDebug(IMAPRESOURCE_LOG) << "IDLE dropped maybe we should reconnect?";
    m_idle = nullptr;
    if (m_session) {
        qCDebug(IMAPRESOURCE_LOG) << "Restarting the IDLE session!";
        m_idle = new KIMAP::IdleJob(m_session);
        connect(m_idle.data(), &KIMAP::IdleJob::mailBoxStats, this, &ImapIdleManager::onStatsReceived);
        connect(m_idle.data(), &KIMAP::IdleJob::result, this, &ImapIdleManager::onIdleStopped);
        m_idle->start();
    }
}

void ImapIdleManager::onStatsReceived(KIMAP::IdleJob *job, const QString &mailBox,
                                      int messageCount, int recentCount)
{
    qCDebug(IMAPRESOURCE_LOG) << "IDLE stats received:" << job << mailBox << messageCount << recentCount;
    qCDebug(IMAPRESOURCE_LOG) << "Cached information:" << m_state->collection().remoteId() << m_state->collection().id()
                              << m_lastMessageCount << m_lastRecentCount;

    // It seems we're not in sync with the cache, resync is needed
    if (messageCount != m_lastMessageCount || recentCount != m_lastRecentCount) {
        m_lastMessageCount = messageCount;
        m_lastRecentCount = recentCount;

        qCDebug(IMAPRESOURCE_LOG) << "Resync needed for" << mailBox << m_state->collection().id();
        m_resource->synchronizeCollection(m_state->collection().id());
    }
}

void ImapIdleManager::onFlagsChanged(KIMAP::IdleJob *job)
{
    Q_UNUSED(job);
    qCDebug(IMAPRESOURCE_LOG) << "IDLE flags changed in" << m_session->selectedMailBox();
    m_resource->synchronizeCollection(m_state->collection().id());
}
