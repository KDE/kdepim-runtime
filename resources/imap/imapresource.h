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

#ifndef __IMAP_RESOURCE_H__
#define __IMAP_RESOURCE_H__

#include <akonadi/resourcebase.h>

namespace KIMAP
{
  class Session;
}

class ImapAccount;
class ImapIdleManager;
class SessionPool;
class ResourceState;

class ImapResource : public Akonadi::ResourceBase, public Akonadi::AgentBase::ObserverV2
{
  Q_OBJECT
  Q_CLASSINFO( "D-Bus Interface", "org.kde.Akonadi.Imap.Resource" )

  using Akonadi::AgentBase::Observer::collectionChanged;

public:
  ImapResource( const QString &id );
  ~ImapResource();


  int configureDialog( WId windowId );

public Q_SLOTS:
  virtual void configure( WId windowId );

  // DBus method
  Q_SCRIPTABLE void requestManualExpunge( qint64 collectionId );

protected Q_SLOTS:
  void startIdleIfNeeded();
  void startIdle();


  void retrieveCollections();
  void retrieveCollectionAttributes( const Akonadi::Collection &collection );

  void retrieveItems( const Akonadi::Collection &col );
  bool retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts );

protected:
  virtual void itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection );
  virtual void itemChanged( const Akonadi::Item &item, const QSet<QByteArray> &parts );
  virtual void itemRemoved( const Akonadi::Item &item );
  virtual void itemMoved( const Akonadi::Item &item, const Akonadi::Collection &source,
                          const Akonadi::Collection &destination );


  virtual void collectionAdded( const Akonadi::Collection &collection, const Akonadi::Collection &parent );
  virtual void collectionChanged( const Akonadi::Collection &collection, const QSet<QByteArray> &parts );
  virtual void collectionRemoved( const Akonadi::Collection &collection );
  virtual void collectionMoved( const Akonadi::Collection &collection, const Akonadi::Collection &source,
                                const Akonadi::Collection &destination );


  virtual void doSetOnline(bool online);


private Q_SLOTS:
  void reconnect();

  void scheduleConnectionAttempt();
  void startConnect( const QVariant & ); // the parameter is necessary, since this method is used by the task scheduler
  void onConnectDone( int errorCode, const QString &errorMessage );
  void onConnectionLost( KIMAP::Session *session );


  void onIdleCollectionFetchDone( KJob *job );


  void onExpungeCollectionFetchDone( KJob *job );
  void triggerCollectionExpunge( const QVariant &collectionVariant );


  void triggerCollectionExtraInfoJobs( const QVariant &collection );

private:
  friend class ResourceState;

  bool needsNetwork() const;

  friend class ImapIdleManager;

  SessionPool *m_pool;
  ImapIdleManager *m_idle;
};

#endif
