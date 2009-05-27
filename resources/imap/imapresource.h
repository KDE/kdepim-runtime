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

#ifndef __IMAP_RESOURCE_H__
#define __IMAP_RESOURCE_H__

class Imaplib;
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

class ImapResource : public Akonadi::ResourceBase, public Akonadi::AgentBase::Observer
{
  Q_OBJECT
  Q_CLASSINFO( "D-Bus Interface", "org.kde.Akonadi.Imap.Resource" )

public:
  ImapResource( const QString &id );
  ~ImapResource();

public Q_SLOTS:
  virtual void configure( WId windowId );

protected Q_SLOTS:
  void retrieveCollections();
  void retrieveItems( const Akonadi::Collection &col );
  bool retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts );

protected:
  virtual void itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection );
  virtual void itemChanged( const Akonadi::Item &item, const QSet<QByteArray> &parts );
  virtual void itemRemoved( const Akonadi::Item &item );

  virtual void collectionAdded( const Akonadi::Collection &collection, const Akonadi::Collection &parent );
  virtual void collectionChanged( const Akonadi::Collection &collection );
  virtual void collectionRemoved( const Akonadi::Collection &collection );

private Q_SLOTS:
  void onConnectSuccess();
  void onConnectError( int code, const QString &message );
  void onMailBoxesReceived( const QList<KIMAP::MailBoxDescriptor> &descriptors,
                            const QList< QList<QByteArray> > &flags );
  void onGetAclDone( KJob *job );
  void onRightsReceived( KJob *job );
  void onQuotasReceived( KJob *job );
  void onGetMetaDataDone( KJob *job );
  void onSelectDone( KJob *job );
  void onHeadersReceived( const QString &mailBox, const QMap<qint64, qint64> &uids,
                          const QMap<qint64, qint64> &sizes, const QMap<qint64, KIMAP::MessageFlags> &flags,
                          const QMap<qint64, KIMAP::MessagePtr> &messages );
  void onHeadersFetchDone( KJob *job );
  void onMessagesReceived( const QString &mailBox, const QMap<qint64, qint64> &uids,
                           const QMap<qint64, KIMAP::MessagePtr> &messages );
  void onContentFetchDone( KJob *job );
  void onCreateMailBoxDone( KJob *job );
  void onRenameMailBoxDone( KJob *job );
  void onDeleteMailBoxDone( KJob *job );
  void onAppendMessageDone( KJob *job );
  void onStoreFlagsDone( KJob *job );

private:
  QString rootRemoteId() const;
  QString remoteIdForMailBox( const QString &path ) const;
  QString mailBoxForRemoteId( const QString &remoteId ) const;

  Akonadi::Collection collectionFromRemoteId( const QString &remoteId );
  Akonadi::Item itemFromRemoteId( const Akonadi::Collection &collection, const QString &remoteId );
  void itemsClear( const Akonadi::Collection &collection );

  bool manualAuth( const QString& username, QString &password );
  void startConnect( bool forceManualAuth = false );

  ImapAccount *m_account;
};

#endif
