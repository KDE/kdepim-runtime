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

namespace KMime
{
  class Message;
}

namespace KIMAP
{
  class Session;
}

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
  void onLoginDone( KJob *job );
  void onCapabilitiesTestDone( KJob *job );
  void onMailBoxReceived( const QList<QByteArray> &descriptor,
                          const QList<QByteArray> &flags );
  void onSelectDone( KJob *job );
  void onHeadersReceived( const QByteArray &mailBox, qint64 uid, int messageNumber,
                          qint64 size, QList<QByteArray> flags,
                          boost::shared_ptr<KMime::Message> message );
  void onHeadersFetchDone( KJob *job );
  void onMessageReceived( const QByteArray &mailBox, qint64 uid, int messageNumber, boost::shared_ptr<KMime::Message> message );
  void onContentFetchDone( KJob *job );
  void onCreateMailBoxDone( KJob *job );
  void onRenameMailBoxDone( KJob *job );
  void onDeleteMailBoxDone( KJob *job );
  void onAppendMessageDone( KJob *job );
  void onStoreFlagsDone( KJob *job );

  void slotAlert( Imaplib*, const QString& message );

private:
  QString rootRemoteId() const;
  QString mailBoxRemoteId( const QString &path ) const;
  Akonadi::Collection collectionFromRemoteId( const QString &remoteId );
  Akonadi::Item itemFromRemoteId( const Akonadi::Collection &collection, const QString &remoteId );
  void itemsClear( const Akonadi::Collection &collection );

  bool manualAuth( const QString& username, QString &password );
  void startConnect( bool forceManualAuth = false );

  KIMAP::Session *m_session;
  Akonadi::Collection m_collection;
  Akonadi::Item m_itemAdded;
  QHash<QString, QString> m_flagsCache;
  QHash<QString, Akonadi::Item> m_itemCache;
  QString m_server;
  QString m_userName;
};

#endif
