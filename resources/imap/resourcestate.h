/*
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

#ifndef RESOURCESTATE_H
#define RESOURCESTATE_H

#include "resourcestateinterface.h"

class ImapResource;

class ResourceState : public ResourceStateInterface
{
public:
  static ResourceStateInterface::Ptr createRetrieveItemState( ImapResource *resource,
                                                              const Akonadi::Item &item,
                                                              const QSet<QByteArray> &parts );

  static ResourceStateInterface::Ptr createRetrieveItemsState( ImapResource *resource,
                                                               const Akonadi::Collection &collection );

  static ResourceStateInterface::Ptr createRetrieveCollectionsState( ImapResource *resource );

  static ResourceStateInterface::Ptr createRetrieveCollectionMetadataState( ImapResource *resource,
                                                                            const Akonadi::Collection &collection );

  static ResourceStateInterface::Ptr createAddItemState( ImapResource *resource,
                                                         const Akonadi::Item &item,
                                                         const Akonadi::Collection &collection );

  static ResourceStateInterface::Ptr createChangeItemState( ImapResource *resource,
                                                            const Akonadi::Item &item,
                                                            const QSet<QByteArray> &parts );

  static ResourceStateInterface::Ptr createRemoveItemState( ImapResource *resource,
                                                            const Akonadi::Item &item );

  static ResourceStateInterface::Ptr createMoveItemState( ImapResource *resource,
                                                          const Akonadi::Item &item,
                                                          const Akonadi::Collection &sourceCollection,
                                                          const Akonadi::Collection &targetCollection );

private:
  explicit ResourceState( ImapResource *resource );
public:
  ~ResourceState();

  virtual QString resourceName() const;
  virtual QStringList serverCapabilities() const;
  virtual QList<KIMAP::MailBoxDescriptor> serverNamespaces() const;

  virtual bool isAutomaticExpungeEnabled() const;
  virtual bool isSubscriptionEnabled() const;
  virtual bool isDisconnectedModeEnabled() const;
  virtual int intervalCheckTime() const;

  virtual Akonadi::Collection collection() const;
  virtual Akonadi::Item item() const;

  virtual Akonadi::Collection parentCollection() const;

  virtual Akonadi::Collection sourceCollection() const;
  virtual Akonadi::Collection targetCollection() const;

  virtual QSet<QByteArray> parts() const;

  virtual QString rootRemoteId() const;
  virtual QString mailBoxForCollection( const Akonadi::Collection &collection ) const;

  virtual void setIdleCollection( const Akonadi::Collection &collection );
  virtual void applyCollectionChanges( const Akonadi::Collection &collection );

  virtual void itemRetrieved( const Akonadi::Item &item );

  virtual void itemsRetrieved( const Akonadi::Item::List &items );
  virtual void itemsRetrievalDone();

  virtual void changeCommitted( const Akonadi::Item &item );

  virtual void collectionsRetrieved( const Akonadi::Collection::List &collections );
  virtual void collectionsRetrievalDone();

  virtual void changeProcessed();

  virtual void cancelTask( const QString &errorString );
  virtual void deferTask();
  virtual void taskDone();

  virtual void emitWarning( const QString &message );

private:
  ImapResource *m_resource;

  Akonadi::Collection m_collection;
  Akonadi::Item m_item;

  Akonadi::Collection m_parentCollection;

  Akonadi::Collection m_sourceCollection;
  Akonadi::Collection m_targetCollection;

  QSet<QByteArray> m_parts;
};

#endif
