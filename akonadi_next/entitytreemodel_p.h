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

struct Node
{
  qint64 id;
  qint64 parent;

  enum Type
  {
    Item,
    Collection
  };
  int type;
};

namespace Akonadi
{
/**
 * @internal
 */
class EntityTreeModelPrivate
{
public:

  EntityTreeModelPrivate( EntityTreeModel *parent );
  EntityTreeModel *q_ptr;

//   void collectionStatisticsChanged( Collection::Id, const Akonadi::CollectionStatistics& );

  bool mimetypeMatches( const QStringList &mimetypes, const QStringList &other );

  enum RetrieveDepth{
    Base,
    Recursive
  };

  void fetchCollections( Collection col, CollectionFetchJob::Type = CollectionFetchJob::FirstLevel );
  // TODO: Remove retrieveDepth?
  void fetchItems( Collection col, int retrieveDepth = Base );
  void collectionsFetched( const Akonadi::Collection::List& );
//   void resourceTopCollectionsFetched( const Akonadi::Collection::List& );
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

  void monitoredItemLinked( const Akonadi::Item&, const Akonadi::Collection& );
  void monitoredItemUnlinked( const Akonadi::Item&, const Akonadi::Collection& );

  Collection getParentCollection( qint64 id ) const;
  Collection::List getParentCollections( Item item ) const;
  Collection getParentCollection( Collection col ) const;
  qint64 childAt(Collection::Id, int position, bool *ok) const;
  int indexOf(Collection::Id parent, Collection::Id col) const;
  Item getItem(qint64) const;

  QHash< Collection::Id, Collection > m_collections;
  QHash< qint64, Item > m_items;
  QHash< Collection::Id, QList< Node * > > m_childEntities;
  QSet<Collection::Id> m_populatedCols;

  Monitor *m_monitor;
  Collection m_rootCollection;
  Node *m_rootNode;
  QString m_rootCollectionDisplayName;
  QStringList m_mimeTypeFilter;
  EntityTreeModel::CollectionFetchStrategy m_collectionFetchStrategy;
  EntityTreeModel::ItemPopulationStrategy m_itemPopulation;
  bool m_includeUnsubscribed;
  bool m_showRootCollection;

  void startFirstListJob();

  void fetchJobDone( KJob *job );
  void updateJobDone( KJob *job );

  bool passesFilter( const QStringList &mimetypes );

  /**
    Returns the index of the node in @p list with the id @p id. Returns -1 if not found.
  */
  int indexOf(QList<Node*> list, qint64 id) const;

  /**
  The id of the collection which starts an item fetch job. This is part of a hack with QObject::sender
  in itemsReceivedFromJob to correctly insert items into the model.
  */
  static QByteArray ItemFetchCollectionId() {
    return "ItemFetchCollectionId";
  }

  Session *m_session;

  Q_DECLARE_PUBLIC( EntityTreeModel )
};

}

#endif

