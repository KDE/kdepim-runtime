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

class KJob;

#include <akonadi/resourcebase.h>
#include <boost/shared_ptr.hpp>
#include <QtCore/QStringList>

#include <kimap/fetchjob.h>
#include <kimap/listjob.h>

namespace KMime
{
  class Message;
}

namespace KIMAP
{
  class Session;
}

class ImapAccount;
class ImapIdleManager;
class SessionPool;

class ImapResource : public Akonadi::ResourceBase, public Akonadi::AgentBase::ObserverV2
{
  Q_OBJECT
  Q_CLASSINFO( "D-Bus Interface", "org.kde.Akonadi.Imap.Resource" )

  using Akonadi::AgentBase::Observer::collectionChanged;

public:
  ImapResource( const QString &id );
  ~ImapResource();
  void renameRootCollection( const QString &newName );

public Q_SLOTS:
  virtual void configure( WId windowId );
  // DBus methods
  void requestManualExpunge( qint64 collectionId );
  void startIdle();

protected Q_SLOTS:
  void retrieveCollections();
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
  void onConnectDone( int errorCode, const QString &errorMessage );
  void onMainSessionRequested( qint64 requestId, KIMAP::Session *session, int errorCode, const QString &errorMessage );
  void scheduleConnectionAttempt();

  void onMailBoxesReceived( const QList<KIMAP::MailBoxDescriptor> &descriptors,
                            const QList< QList<QByteArray> > &flags );
  void onMailBoxesReceiveDone( KJob *job );
  void onGetAclDone( KJob *job );
  void onRightsReceived( KJob *job );
  void onQuotasReceived( KJob *job );
  void onGetMetaDataDone( KJob *job );
  void onSelectDone( KJob *job );
  void onCollectionStatisticsReceived( KJob *job );
  void onHeadersReceived( const QString &mailBox, const QMap<qint64, qint64> &uids,
                          const QMap<qint64, qint64> &sizes, const QMap<qint64, KIMAP::MessageFlags> &flags,
                          const QMap<qint64, KIMAP::MessagePtr> &messages );
  void onHeadersFetchDone( KJob *job );
  void onFlagsReceived( const QString &mailBox, const QMap<qint64, qint64> &uids,
                        const QMap<qint64, qint64> &sizes, const QMap<qint64, KIMAP::MessageFlags> &flags,
                        const QMap<qint64, KIMAP::MessagePtr> &messages );
  void onFlagsFetchDone( KJob *job );
  void onMessagesReceived( const QString &mailBox, const QMap<qint64, qint64> &uids,
                           const QMap<qint64, KIMAP::MessagePtr> &messages );
  void onContentFetchDone( KJob *job );
  void onCreateMailBoxDone( KJob *job );
  void onRenameMailBoxDone( KJob *job );
  void onSetAclDone( KJob *job );
  void onSetMetaDataDone( KJob *job );
  void onDeleteMailBoxDone( KJob *job );
  void onMailBoxMoveDone( KJob *job );
  void onSubscribeDone( KJob *job );
  void onAppendMessageDone( KJob *job );
  void onStoreFlagsDone( KJob *job );
  void onPreItemMoveSelectDone( KJob *job );
  void onCopyMessageDone( KJob *job );
  void onPostItemMoveStoreFlagsDone( KJob *job );
  void onIdleCollectionFetchDone( KJob *job );

  void startConnect( QVariant v ); // the parameter is necessary, since this method is used by the task scheduler
  void reconnect();

  void expungeRequested( const QVariant &collectionArgument );
  void onExpungeCollectionFetchDone( KJob *job );
  void onRootCollectionFetched( KJob *job );
  void triggerCollectionExtraInfoJobs( const QVariant &collection );

private:
  void onSelectDone( const QString&, int, qint64, int, qint64, qint64, bool );
  void triggerNextCollectionChangeJob( const Akonadi::Collection &collection,
                                       const QStringList &remainingParts );

  void triggerExpunge( const QString &mailBox );
  void listFlagsForImapSet( const KIMAP::ImapSet& set );
  void selectIfNeeded( const QString &mailBox );

  QString rootRemoteId() const;
  QString mailBoxForCollection( const Akonadi::Collection &col ) const;
  bool needsNetwork() const;
  bool isSessionAvailable() const;
  bool ensureSessionAvailableOrDefer();

  friend class ImapIdleManager;

  SessionPool *m_pool;
  qint64 m_mainSessionRequestId;
  KIMAP::Session *m_mainSession;

  int m_finishedMetaDataJobs;

  ImapIdleManager *m_idle;
};

#endif
