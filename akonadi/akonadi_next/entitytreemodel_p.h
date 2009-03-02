/*
    Copyright (c) 2008 Stephen Kelly <steveire@gmail.com>

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

#ifndef ENTITYTREEMODELPRIVATE_H
#define ENTITYTREEMODELPRIVATE_H

#include <akonadi/item.h>
#include <KJob>

#include <akonadi/collectionfetchjob.h>
#include <akonadi/itemfetchscope.h>

#include "entitytreemodel.h"

namespace Akonadi
{
// class ClientSideEntityStorage;
/**
 * @internal
 */
class EntityTreeModelPrivate
{
public:

  EntityTreeModelPrivate( EntityTreeModel *parent );
  EntityTreeModel *q_ptr;

//   void collectionChanged( const Akonadi::Collection& );
//   void itemChanged( const Item&, const QSet<QByteArray>& );

//   void collectionStatisticsChanged( Collection::Id, const Akonadi::CollectionStatistics& );

  bool mimetypeMatches( const QStringList &mimetypes, const QStringList &other );

//   void rowsAboutToBeInserted( Collection::Id colId, int start, int end );
//   void rowsAboutToBeRemoved( Collection::Id colId, int start, int end );
//   void rowsInserted();
//   void rowsRemoved();

  enum RetrieveDepth{
    Base,
    Recursive
  };

  void fetchCollections( Collection col, CollectionFetchJob::Type = CollectionFetchJob::FirstLevel );
  // TODO: Remove retrieveDepth?
  void fetchItems( Collection col, int retrieveDepth = Base );
  void collectionsFetched( const Akonadi::Collection::List& );
  void itemsFetched( const Akonadi::Item::List& );

  void monitoredCollectionAdded( const Akonadi::Collection&, const Akonadi::Collection& );
  void monitoredCollectionRemoved( const Akonadi::Collection& );
  void monitoredCollectionChanged( const Akonadi::Collection& );
  void monitoredCollectionStatisticsChanged( Akonadi::Collection::Id, const Akonadi::CollectionStatistics &);
  void monitoredCollectionMoved( const Akonadi::Collection&, const Akonadi::Collection&, const Akonadi::Collection&);
  void monitoredItemAdded( const Akonadi::Item&, const Akonadi::Collection& );
  void monitoredItemRemoved( const Akonadi::Item& );
  void monitoredItemChanged( const Akonadi::Item&, const QSet<QByteArray>& );
  void monitoredItemMoved( const Akonadi::Item&, const Akonadi::Collection&, const Akonadi::Collection& );


  Collection getParentCollection( qint64 id ) const;
  Collection getParentCollection( Item item ) const;
  Collection getParentCollection( Collection col ) const;
  qint64 childAt(Collection::Id, int position, bool *ok) const;
  int indexOf(Collection::Id parent, Collection::Id col) const;
  Item getItem(qint64) const;

  QHash< Collection::Id, Collection > m_collections;
  QHash< qint64, Item > m_items;
  QHash< Collection::Id, QList< qint64 > > m_childEntities;
  QSet<Collection::Id> m_populatedCols;

  Monitor *m_monitor;
//   int m_entitiesToFetch;
  Collection m_rootCollection;
  QString m_rootCollectionDisplayName;
  QStringList m_mimeTypeFilter;
  int m_collectionFetchStrategy;
  int m_itemPopulation;
  bool m_includeUnsubscribed;
  bool m_showRootCollection;

  void startFirstListJob();

  void fetchJobDone( KJob *job );
  void updateJobDone( KJob *job );

  bool passesFilter( const QStringList &mimetypes );

  /**
  The id of the collection which starts an item fetch job. This is part of a hack with QObject::sender
  in itemsReceivedFromJob to correctly insert items into the model.
  */
  static QByteArray ItemFetchCollectionId() {
    return "ItemFetchCollectionId";
  }

  EntityUpdateAdapter *entityUpdateAdapter;
//   ClientSideEntityStorage *m_clientSideEntityStorage;

  Q_DECLARE_PUBLIC( EntityTreeModel )
};

}

#endif

