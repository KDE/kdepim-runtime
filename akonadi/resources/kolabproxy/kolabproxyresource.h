/*
    Copyright (c) 2009 Andras Mantia <amantia@kde.org>

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

#ifndef KOLABPROXYRESOURCE_H
#define KOLABPROXYRESOURCE_H

#include <akonadi/resourcebase.h>
#include <QStringList>


namespace Akonadi {
  class Monitor;
}

class KolabHandler;

class KolabProxyResource : public Akonadi::ResourceBase,
                           public Akonadi::AgentBase::Observer
{
  Q_OBJECT

  public:
    KolabProxyResource( const QString &id );
    ~KolabProxyResource();

    /**
      Creates a new KolabHandler for @p imapCollection given it actually is a Kolab
      folder.
      @return @c true if @p imapCollection is a Kolab folder, @c false otherwise.
    */
    bool registerHandlerForCollection( const Akonadi::Collection& imapCollection );

    /**
      Returns the root collection.
    */
    inline Akonadi::Collection root() const { return m_root; }

  public Q_SLOTS:
    virtual void configure( WId windowId );

  protected Q_SLOTS:
    void retrieveCollections();
    void retrieveItems( const Akonadi::Collection &col );
    bool retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts );
    void imapItemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection);
    void imapItemRemoved(const Akonadi::Item &item);
    void imapCollectionAdded(const Akonadi::Collection &collection, const Akonadi::Collection &parent);
    void imapCollectionRemoved(const Akonadi::Collection &collection);
    void imapCollectionChanged(const Akonadi::Collection &collection);
    void itemCreatedDone(KJob *job);
    void collectionFetchDone(KJob *job);
    void retrieveItemFetchDone(KJob *);
    void retrieveCollectionsTreeDone(KJob* job);
    void addImapItem(const Akonadi::Item& item, Akonadi::Entity::Id collectionId);
    void deleteImapItem(const Akonadi::Item& item);


  protected:
    virtual void aboutToQuit();

    virtual void itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection );
    virtual void itemChanged( const Akonadi::Item &item, const QSet<QByteArray> &parts );
    virtual void itemRemoved( const Akonadi::Item &item );
    virtual void collectionAdded(const Akonadi::Collection& collection, const Akonadi::Collection& parent);
    virtual void collectionChanged(const Akonadi::Collection& collection);
    virtual void collectionRemoved(const Akonadi::Collection& collection);

  private:
    Akonadi::Collection createCollection(const Akonadi::Collection& imapCollection);

  private slots:
    void imapItemCreationResult( KJob* job );
    void imapFolderCreateResult( KJob* job );
    void kolabFolderChangeResult( KJob* job );

  private:
    Akonadi::Monitor *m_monitor;
    Akonadi::Monitor *m_collectionMonitor;
    QMap<Akonadi::Item::Id, KolabHandler*> m_monitoredCollections;
    QMap<KJob*, QString> m_ids;
    QMap<KJob*, Akonadi::Item> m_items;
    QList<Akonadi::Item::Id> m_excludeAppend;
    Akonadi::Collection m_root;

    enum RetrieveState {
      RetrieveItems,
      RetrieveItem,
      DeleteItem
    };

    RetrieveState m_retrieveState;
};

#endif
