/*
  Copyright (c) 2009 Andras Mantia <amantia@kde.org>
  Copyright (c) 2012 Christian Mollekopf <mollekopf@kolabsys.com>

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

#ifndef KOLABPROXY_KOLABPROXYRESOURCE_H
#define KOLABPROXY_KOLABPROXYRESOURCE_H

#include "kolabhandler.h"

#include <Akonadi/ResourceBase>

class FreeBusyUpdateHandler;

namespace Akonadi {
  class Monitor;
}

class KolabProxyResource : public Akonadi::ResourceBase,
                           public Akonadi::AgentBase::ObserverV2
{
  Q_OBJECT

  public:
    KolabProxyResource( const QString &id );
    ~KolabProxyResource();

    /**
     * Creates a new KolabHandler for @p imapCollection given it actually is
     * a Kolab folder.
     *
     * @return @c true if @p imapCollection is a Kolab folder, @c false otherwise.
     */
    bool registerHandlerForCollection( const Akonadi::Collection &imapCollection );

  public Q_SLOTS:
    virtual void configure( WId windowId );

  protected Q_SLOTS:
    void retrieveCollections();

    void retrieveItems( const Akonadi::Collection &col );

    bool retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts );

    void imapItemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection );

    void imapItemMoved( const Akonadi::Item &item,
                        const Akonadi::Collection &collectionSource,
                        const Akonadi::Collection &collectionDestination );

    void imapItemRemoved( const Akonadi::Item &item );

    void imapCollectionAdded( const Akonadi::Collection &collection,
                              const Akonadi::Collection &parent );

    void imapCollectionRemoved( const Akonadi::Collection &collection );

    void imapCollectionChanged( const Akonadi::Collection &collection );

    void imapCollectionMoved( const Akonadi::Collection &collection,
                              const Akonadi::Collection &source,
                              const Akonadi::Collection &destination );

    void itemCreatedDone( KJob *job );
    void collectionFetchDone( KJob *job );
    void retrieveItemFetchDone( KJob * );
    void retrieveCollectionsTreeDone( KJob *job );
    void addImapItem( const Akonadi::Item &item, Akonadi::Entity::Id collectionId );
    void deleteImapItem( const Akonadi::Item &item );

  protected:
    virtual void aboutToQuit();

    virtual void itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection );
    virtual void itemChanged( const Akonadi::Item &item, const QSet<QByteArray> &parts );

    virtual void itemMoved( const Akonadi::Item &item,
                            const Akonadi::Collection &collectionSource,
                            const Akonadi::Collection &collectionDestination );

    virtual void itemRemoved( const Akonadi::Item &item );

    virtual void collectionAdded( const Akonadi::Collection &collection,
                                  const Akonadi::Collection &parent );

    virtual void collectionChanged( const Akonadi::Collection &collection );

    // do not hide the other variant, use implementation from base class
    // which just forwards to the one above
    using Akonadi::AgentBase::ObserverV2::collectionChanged;
    virtual void collectionMoved( const Akonadi::Collection &collection,
                                  const Akonadi::Collection &source,
                                  const Akonadi::Collection &destination );

    virtual void collectionRemoved( const Akonadi::Collection &collection );

  private:
    Akonadi::Collection createCollection( const Akonadi::Collection &imapCollection );

    void applyAttributesToImap( Akonadi::Collection &imapCollection,
                                const Akonadi::Collection &kolabCollection );

    void applyAttributesFromImap( Akonadi::Collection &kolabCollection,
                                  const Akonadi::Collection &imapCollection );

    void updateFreeBusyInformation( const Akonadi::Collection &imapCollection );

  private slots:
    void imapItemCreationResult( KJob *job );
    void imapItemUpdateFetchResult( KJob *job );
    void imapItemUpdateResult( KJob *job );
    void imapItemUpdateCollectionFetchResult( KJob *job );
    void imapFolderCreateResult( KJob *job );
    void kolabFolderChangeResult( KJob *job );

  private:
    Akonadi::Monitor *m_monitor;
    Akonadi::Monitor *m_collectionMonitor;
    QMap<Akonadi::Collection::Id, KolabHandler::Ptr> m_monitoredCollections;
    QMap<Akonadi::Collection::Id, QString> m_resourceIdentifier;
    QMap<KJob *, QString> m_ids;
    QMap<KJob *, Akonadi::Item> m_items;
    QList<Akonadi::Item::Id> m_excludeAppend;
    FreeBusyUpdateHandler *m_freeBusyUpdateHandler;

    enum RetrieveState {
      RetrieveItems,
      RetrieveItem,
      DeleteItem
    };

    RetrieveState m_retrieveState;
};

#endif
