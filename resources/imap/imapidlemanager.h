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

#ifndef __IMAP_IDLEMANAGER_H__
#define __IMAP_IDLEMANAGER_H__

#include <akonadi/collection.h>
#include <QtCore/QObject>

namespace KIMAP
{
  class IdleJob;
  class Session;
}

class ImapResource;

class KJob;

class ImapIdleManager : public QObject
{
  Q_OBJECT

public:
  ImapIdleManager( Akonadi::Collection &col, KIMAP::Session *session, ImapResource *parent );
  ~ImapIdleManager();

  KIMAP::Session *session() const;

private slots:
  void onIdleStopped();
  void onStatsReceived(KIMAP::IdleJob *job, const QString &mailBox,
                       int messageCount, int recentCount);
  void onCollectionFetchDone( KJob *job );

private:
  KIMAP::Session *m_session;
  KIMAP::IdleJob *m_idle;
  ImapResource *m_resource;
  Akonadi::Collection m_collection;
};

#endif
