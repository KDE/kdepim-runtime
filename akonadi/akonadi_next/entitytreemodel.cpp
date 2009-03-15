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

#include "entitytreemodel.h"
#include "entitytreemodel_p.h"

#include <QtCore/QHash>
#include <QtCore/QMimeData>
#include <QTimer>

#include <KIcon>
#include <KUrl>
#include <KLocale>

#include <akonadi/attributefactory.h>
#include "collectionutils_p.h"
#include "collectionchildorderattribute.h"
#include <akonadi/entitydisplayattribute.h>
#include "entityupdateadapter.h"
#include <akonadi/monitor.h>

#include "kdebug.h"

using namespace Akonadi;

EntityTreeModel::EntityTreeModel( EntityUpdateAdapter *entityUpdateAdapter,
                                  Monitor *monitor,
                                  QObject *parent
                                )
    : QAbstractItemModel( parent ),
    d_ptr( new EntityTreeModelPrivate( this ) )
{
  Q_D( EntityTreeModel );

  d->m_monitor = monitor;
  d->entityUpdateAdapter = entityUpdateAdapter;

  // monitor collection changes
  connect( monitor, SIGNAL( collectionChanged( const Akonadi::Collection& ) ),
           SLOT( monitoredCollectionChanged( const Akonadi::Collection& ) ) );
  connect( monitor, SIGNAL( collectionAdded( const Akonadi::Collection &, const Akonadi::Collection & ) ),
           SLOT( monitoredCollectionAdded( const Akonadi::Collection &, const Akonadi::Collection & ) ) );
  connect( monitor, SIGNAL( collectionRemoved( const Akonadi::Collection & ) ),
           SLOT( monitoredCollectionRemoved( const Akonadi::Collection &) ) );
  connect( monitor,
            SIGNAL( collectionMoved( const Akonadi::Collection &, const Akonadi::Collection &, const Akonadi::Collection & ) ),
           SLOT( monitoredCollectionMoved( const Akonadi::Collection &, const Akonadi::Collection &, const Akonadi::Collection & ) ) );

  //TODO: Figure out if the monitor emits these signals even without an item fetch scope.
  // Wrap them in an if() if so.
  // Don't want to be adding items to a model if NoItemPopulation is set.
  // If LazyPopulation is set, then we'll have to add items to collections which
  // have already been lazily populated.


  // Monitor item changes.
  connect( monitor, SIGNAL( itemAdded( const Akonadi::Item&, const Akonadi::Collection& ) ),
           SLOT( monitoredItemAdded( const Akonadi::Item&, const Akonadi::Collection& ) ) );
  connect( monitor, SIGNAL( itemChanged( const Akonadi::Item&, const QSet<QByteArray>& ) ),
           SLOT( monitoredItemChanged( const Akonadi::Item&, const QSet<QByteArray>& ) ) );
  connect( monitor, SIGNAL( itemRemoved( const Akonadi::Item& ) ),
           SLOT( monitoredItemRemoved( const Akonadi::Item& ) ) );
  connect( monitor, SIGNAL( itemMoved( const Akonadi::Item, const Akonadi::Collection, const Akonadi::Collection ) ),
           SLOT( monitoredItemMoved( const Akonadi::Item, const Akonadi::Collection, const Akonadi::Collection ) ) );

  connect( monitor, SIGNAL( collectionStatisticsChanged( Akonadi::Collection::Id, const Akonadi::CollectionStatistics &) ),
           SLOT(monitoredCollectionStatisticsChanged( Akonadi::Collection::Id, const Akonadi::CollectionStatistics &) ) );


// TODO:
//   q->connect( monitor, SIGNAL( itemLinked(const Akonadi::Item &item, const Akonadi::Collection &collection)),
//             q, SLOT(itemLinked(const Akonadi::Item &item, const Akonadi::Collection &collection)));
//   q->connect( monitor, SIGNAL( itemUnlinked(const Akonadi::Item &item, const Akonadi::Collection &collection)),
//             q, SLOT(itemUnlinked(const Akonadi::Item &item, const Akonadi::Collection &collection)));

//   connect(q, SIGNAL(modelReset()), q, SLOT(slotModelReset()));

  d->m_rootCollection = Collection::root();
  d->m_rootCollectionDisplayName = QString("[*]");

  // Initializes the model cleanly.
  clearAndReset();

}

EntityTreeModel::~EntityTreeModel()
{
  Q_D( EntityTreeModel );
}

// Could be a problem here. We can't control what slot will get called first when the
// signal is emitted, but we require that this slot is called first.
// Solved by calling this instead of reset()?
void EntityTreeModel::clearAndReset()
{
  Q_D( EntityTreeModel );
  d->m_collections.clear();
  d->m_items.clear();
  d->m_childEntities.clear();
  reset();
  d->startFirstListJob();
}

Collection EntityTreeModel::getCollection( Collection::Id id )
{
  Q_D( EntityTreeModel );
  return d->m_collections.value(id);
}

Item EntityTreeModel::getItem( qint64 id )
{
  Q_D( EntityTreeModel );
  if (id > 0) id *= -1;
  return d->m_items.value(id);
}

int EntityTreeModel::columnCount( const QModelIndex & parent ) const
{
// TODO: Subscriptions? Statistics?
  if ( parent.isValid() && parent.column() != 0 )
    return 0;
  return 1;
}


QVariant EntityTreeModel::getData(Item item, int column, int role) const
{
  if (column == 0)
  {
    // return the eda or the remoteId.

    switch ( role ) {
    case Qt::DisplayRole:
    case Qt::EditRole:
      if ( item.hasAttribute<EntityDisplayAttribute>() &&
          ! item.attribute<EntityDisplayAttribute>()->displayName().isEmpty() )
        return item.attribute<EntityDisplayAttribute>()->displayName();
      return item.remoteId();

    case Qt::DecorationRole:
      if ( item.hasAttribute<EntityDisplayAttribute>() &&
          ! item.attribute<EntityDisplayAttribute>()->iconName().isEmpty() )
        return item.attribute<EntityDisplayAttribute>()->icon();

    case MimeTypeRole:
      return item.mimeType();

    case RemoteIdRole:
      return item.remoteId();

    case ItemRole:
      return QVariant::fromValue( item );

    case ItemIdRole:
      return item.id();

    default:
      break;
    }
  }
  return QVariant();
}

QVariant EntityTreeModel::getData(Collection col, int column, int role) const
{
  Q_D(const EntityTreeModel);
  if (column >0)
    return QString();

  if (col == Collection::root())
  {
//     Only display the root collection. It may not be edited.
    if ( role == Qt::DisplayRole)
      return d->m_rootCollectionDisplayName;

    if ( role == Qt::EditRole)
      return QVariant();
  }

  if ( column == 0 && ( role == Qt::DisplayRole || role == Qt::EditRole ) ) {
    if ( col.hasAttribute<EntityDisplayAttribute>() &&
          !col.attribute<EntityDisplayAttribute>()->displayName().isEmpty() )
      return col.attribute<EntityDisplayAttribute>()->displayName();
    return col.name();
  }
  switch ( role ) {

  case Qt::DisplayRole:
  case Qt::EditRole:
    if ( column == 0 ) {
      if ( col.hasAttribute<EntityDisplayAttribute>() &&
            !col.attribute<EntityDisplayAttribute>()->displayName().isEmpty() ) {
        return col.attribute<EntityDisplayAttribute>()->displayName();
      }
      return col.name();
    }
    break;

  case Qt::DecorationRole:
    if ( col.hasAttribute<EntityDisplayAttribute>() &&
          ! col.attribute<EntityDisplayAttribute>()->iconName().isEmpty() ) {
      return col.attribute<EntityDisplayAttribute>()->icon();
    }
    return KIcon( CollectionUtils::defaultIconName( col ) );

  case MimeTypeRole:
    return col.mimeType();

  case RemoteIdRole:
    return col.remoteId();

  case CollectionIdRole:
    return col.id();

  case CollectionRole: {
    return QVariant::fromValue( col );
  }
  default:
    break;
  }
  return QVariant();
}


QVariant EntityTreeModel::data( const QModelIndex & index, int role ) const
{
  if ( !index.isValid() )
    return QVariant();
  Q_D( const EntityTreeModel );

  if (d->m_collections.contains(index.internalId()))
  {
    const Collection col = d->m_collections.value(index.internalId());

    if (!col.isValid())
      return QVariant();
//     kDebug();
    return getData(col, index.column(), role);

  }
  else if (d->m_items.contains(index.internalId()))
  {
    const Item item = d->m_items.value(index.internalId());
    if ( !item.isValid() )
      return QVariant();

    QVariant v = getData(item, index.column(), role);
//     kDebug() << v;
    return v;
  }
  return QVariant();

}


Qt::ItemFlags EntityTreeModel::flags( const QModelIndex & index ) const
{
  Q_D( const EntityTreeModel );

  // Pass modeltest.
  // http://labs.trolltech.com/forums/topic/79
  if ( !index.isValid() )
    return 0;

  Qt::ItemFlags flags = QAbstractItemModel::flags( index );

//   Only show and enable items in columns other than 0.
  if ( index.column() != 0 )
    return flags;

  if (d->m_collections.contains(index.internalId()))
  {
    const Collection col = d->m_collections.value(index.internalId());
    if ( col.isValid() ) {

      if (col == Collection::root())
      {
        // Selectable and displayable only.
        return flags;
      }

      int rights = col.rights();
      if ( rights & Collection::CanChangeCollection ) {
        flags |= Qt::ItemIsEditable;
        // Changing the collection includes changing the metadata (child entityordering).
        // Need to allow this by drag and drop.
        flags |= Qt::ItemIsDropEnabled;
      }

      if ( rights & Collection::CanDeleteCollection ) {
        // If this collection is moved, it will need to be deleted
        flags |= Qt::ItemIsDragEnabled;
      }
      if ( rights & ( Collection::CanCreateCollection | Collection::CanCreateItem ) ) {
//           Can we drop new collections and items into this collection?
        flags |= Qt::ItemIsDropEnabled;
      }
    }
  } else if (d->m_items.contains(index.internalId()))
  {
    // Rights come from the parent collection.

    // TODO: Is this right for the root collection? I think so, but only by chance.
    // But will it work if m_rootCollection is different from Collection::root?
    // Should probably rely on index.parent().isValid() for that.
    const Collection parentCol = d->m_collections.value( index.parent().internalId() );
    if ( parentCol.isValid() ) {
      int rights = parentCol.rights();
      // Can't drop onto items.
      if ( rights & Collection::CanChangeItem ) {
        flags = flags | Qt::ItemIsEditable;
      }
      if ( rights & Collection::CanDeleteItem ) {
        // If this item is moved, it will need to be deleted from its parent.
        flags = flags | Qt::ItemIsDragEnabled;
      }
    }
  }
  return flags;
}

// void EntityTreeModelPrivate::moveEntity(const QModelIndex &src, int srcRow, const QModelIndex &dest, int destRow)
// {
//   layoutAboutToBeChanged();
//   // change persistant indexes.
//   layoutChanged();
// }
//
//
// void EntityTreeModelPrivate::entityMoved(Collection col, Collection src, Collection dest)
// {
//   Q_Q(EntityTreeModel);
//
//   QModelIndex entityIndex = indexForCollection(col);
//   QModelIndex srcIndex = entityIndex.parent();
//   int srcRow = entityIndex.row();
// }
//
// void EntityTreeModelPrivate::entityMoved(Item item, Collection src, Collection dest)
// {
//
// }

Qt::DropActions EntityTreeModel::supportedDropActions() const
{
  return Qt::CopyAction | Qt::MoveAction;
}

QStringList EntityTreeModel::mimeTypes() const
{
  // TODO: Should this return the mimetypes that the items provide? Allow dragging a contact from here for example.
  return QStringList() << QLatin1String( "text/uri-list" );
}

bool EntityTreeModel::dropMimeData( const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent )
{
  Q_D( EntityTreeModel );

  // TODO Use action and collection rights and return false if neccessary

// if row and column are -1, then the drop was on parent directly.
// data should then be appended on the end of the items of the collections as appropriate.
// That will mean begin insert rows etc.
// Otherwise it was a sibling of the row^th item of parent.
// That will need to be handled by a proxy model. This one can't handle ordering.
// if parent is invalid the drop occured somewhere on the view that is no model, and corresponds to the root.
  kDebug() << "ismove" << ( action == Qt::MoveAction );
  if ( action == Qt::IgnoreAction )
    return true;

// Shouldn't do this. Need to be able to drop vcards for example.
//   if (!data->hasFormat("text/uri-list"))
//       return false;

// TODO This is probably wrong and unneccessary.
  if ( column > 0 )
    return false;

  if (d->m_items.contains(parent.internalId()))
  {
    // Can't drop data onto an item, although we can drop data between items.
    return false;
    // TODO: Maybe if it's a drop on an item I should drop below the item instead?
    // Find out what others do.
  }

  if (d->m_collections.contains(parent.internalId()))
  {
    Collection destCol = d->m_collections.value( parent.internalId() );

    // Applications can't create new collections in root. Only resources can.
    if (destCol == Collection::root())
      return false;

    if ( data->hasFormat( "text/uri-list" ) ) {
      QHash<Collection::Id, Collection::List> dropped_cols;
      QHash<Collection::Id, Item::List> dropped_items;

      KUrl::List urls = KUrl::List::fromMimeData( data );
      foreach( const KUrl &url, urls ) {
        Collection col = d->m_collections.value( Collection::fromUrl( url ).id() );
        if ( col.isValid() ) {
          if ( !d->mimetypeMatches( destCol.contentMimeTypes(), col.contentMimeTypes() ) )
            return false;

          dropped_cols[ col.parent()].append( col );
        } else {
          Item item = d->m_items.value( Item::fromUrl( url ).id() );
          if ( item.isValid() ) {
            Collection col = d->getParentCollection( item );
            dropped_items[ col.id()].append( item );
          } else {
            // A uri, but not an akonadi url. What to do?
            // Should handle known mimetypes like vcards first.
            // That should make any remaining uris meaningless at this point.
          }
        }
      }
      QHashIterator<Collection::Id, Item::List> item_iter( dropped_items );
      d->entityUpdateAdapter->beginTransaction();
      while ( item_iter.hasNext() ) {
        item_iter.next();
        Collection srcCol = d->m_collections.value(item_iter.key());
        if ( action == Qt::MoveAction ) {
          d->entityUpdateAdapter->moveEntities( item_iter.value(), dropped_cols.value( item_iter.key() ), srcCol, destCol, row );
        } else if ( action == Qt::CopyAction ) {
          d->entityUpdateAdapter->addEntities( item_iter.value(), dropped_cols.value( item_iter.key() ), destCol, row );
        }
        dropped_cols.remove( item_iter.key() );
      }
      QHashIterator<Collection::Id, Collection::List> col_iter( dropped_cols );
      while ( col_iter.hasNext() ) {
        col_iter.next();
        Collection srcCol = d->m_collections.value(item_iter.key());
        if ( action == Qt::MoveAction ) {
          // Empty Item::List() because I know I've already dealt with the items of this parent.
          d->entityUpdateAdapter->moveEntities( Item::List(), col_iter.value(), srcCol, destCol, row );
        } else if ( action == Qt::CopyAction ) {
          d->entityUpdateAdapter->addEntities( Item::List(), col_iter.value(), destCol, row );
        }
      }
      d->entityUpdateAdapter->endTransaction();
      return false; // ### Return false so that the view does not update with the dropped
      // in place where they were dropped. That will be done when the monitor notifies the model
      // through collectionsReceived that the move was successful.
    } else {
//       not a set of uris. Maybe vcards etc. Check if the parent supports them, and maybe do
      // fromMimeData for them. Hmm, put it in the same transaction with the above?
      // TODO: This should be handled first, not last.
    }
  }
  return false;
}

QModelIndex EntityTreeModel::index( int row, int column, const QModelIndex & parent ) const
{

  Q_D( const EntityTreeModel );

  //TODO: don't use column count here. Use some d-> func.
  if ( column >= columnCount() || column < 0 )
    return QModelIndex();

  QList<Entity::Id> childEntities;

  if (!parent.isValid())
  {
    if (d->m_showRootCollection)
    {
      childEntities << d->m_rootCollection.id();
    } else {
      childEntities = d->m_childEntities.value(d->m_rootCollection.id());
    }
  } else {
    childEntities = d->m_childEntities.value(parent.internalId());
  }


//   if (!parent.isValid() && d->m_showRootCollection)
//   {
//     childEntities << m_rootCollection.id();
//   } else
//   {
//     childEntities = d->m_childEntities.value(parent.internalId());
//   }

  int size = childEntities.size();
  if ( row < 0 || row >= size )
    return QModelIndex();

  qint64 internalIdentifier = childEntities.at(row);

//   bool found;
//   qint64 internalIdentifier = d->childAt( parent.internalId(), row, &found );
//
//   if (!found)
//     return QModelIndex();
//
  return createIndex( row, column, reinterpret_cast<void*>( internalIdentifier ) );

}

QModelIndex EntityTreeModel::parent( const QModelIndex & index ) const
{
  Q_D( const EntityTreeModel );

  if ( !index.isValid() )
    return QModelIndex();

  Collection col;
  if (d->m_collections.contains(index.internalId()))
  {
    Collection childCol = d->m_collections.value( index.internalId() );
    col = d->getParentCollection( childCol );
  } else if ( d->m_items.contains(index.internalId() ) )
  {
    Item item = d->getItem(index.internalId());
    col = d->getParentCollection( item );
  }

  if ( !col.isValid() )
    return QModelIndex();

  if ( col.id() == d->m_rootCollection.id() )
  {
    if (!d->m_showRootCollection)
      return QModelIndex();
    else
      return createIndex(0, 0, reinterpret_cast<void *>(d->m_rootCollection.id()));
  }

  int row = d->m_childEntities.value(col.parent()).indexOf(col.id());

  return createIndex( row, 0, reinterpret_cast<void*>( col.id() ) );

}

int EntityTreeModel::rowCount( const QModelIndex & parent ) const
{
  Q_D( const EntityTreeModel );

//   kDebug() << parent;
  qint64 id;
  if (!parent.isValid())
  {
    // If we're showing the root collection then it will be the only child of the root.
    if (d->m_showRootCollection)
      return d->m_childEntities.value(-1).size();

    id = d->m_rootCollection.id();
  } else {
    id = parent.internalId();
  }

//   kDebug() << d->m_childEntities.value(id);
  if (parent.column() == 0)
    return d->m_childEntities.value(id).size();
  return 0;
}

QVariant EntityTreeModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
  if ( section == 0 && orientation == Qt::Horizontal && role == Qt::DisplayRole )
    return i18nc( "@title:column, name of a thing", "Name" );
  return QAbstractItemModel::headerData( section, orientation, role );
}

QMimeData *EntityTreeModel::mimeData( const QModelIndexList &indexes ) const
{
  Q_D( const EntityTreeModel );
  QMimeData *data = new QMimeData();
  KUrl::List urls;
  foreach( const QModelIndex &index, indexes ) {
    if ( index.column() != 0 )
      continue;

    if (!index.isValid())
      continue;

    if (d->m_collections.contains(index.internalId()))
    {
      urls << d->m_collections.value(index.internalId()).url();
    } else if (d->m_items.contains(index.internalId()))
    {
      urls << d->m_items.value( index.internalId() ).url( Item::UrlWithMimeType );
    }
  }
  urls.populateMimeData( data );

  return data;
}

// Always return false for actions which take place asyncronously, eg via a Job.
bool EntityTreeModel::setData( const QModelIndex &index, const QVariant &value, int role )
{
  Q_D( EntityTreeModel );
  // Delegate to something? Akonadi updater, or the manager classes? I think akonadiUpdater. entityUpdateAdapter
  if ( index.column() == 0 && ( role & (Qt::EditRole | ItemRole | CollectionRole) ) ) {
    if (d->m_collections.contains(index.internalId()))
    {
      // rename collection
//       Collection col = d->collectionForIndex( index );
      Collection col = d->m_collections.value( index.internalId() );
      if ( !col.isValid() || value.toString().isEmpty() )
        return false;

// //       if ( col.hasAttribute< EntityDisplayAttribute >() )
// //       {
//       EntityDisplayAttribute *displayAttribute = col.attribute<EntityDisplayAttribute>( Entity::AddIfMissing );
//       displayAttribute->setDisplayName( value.toString() );
//           col.addAttribute(displayAttribute);
// //       }

      d->entityUpdateAdapter->updateEntities( Collection::List() << col );
      return false;
    }
    if (d->m_items.contains(index.internalId()))
    {
      Item i = value.value<Item>();
//       Item item = d->m_items.value( index.internalId() );
//       if ( !item.isValid() || value.toString().isEmpty() )
//         return false;

//       if ( item.hasAttribute< EntityDisplayAttribute >() )
//       {
//       EntityDisplayAttribute *displayAttribute = item.attribute<EntityDisplayAttribute>( Entity::AddIfMissing );
//       displayAttribute->setDisplayName( value.toString() );
//       }
//           item.addAttribute(displayAttribute);
      d->entityUpdateAdapter->updateEntities( Item::List() << i );
//       d->entityUpdateAdapter->updateEntities( Item::List() << item );
        return false;
    }
  }

  return QAbstractItemModel::setData( index, value, role );
}


bool EntityTreeModel::canFetchMore ( const QModelIndex & parent ) const
{
  Q_D( const EntityTreeModel );
  Item item = parent.data(ItemRole).value<Item>();
  if (item.isValid())
  {
    // items can't have more rows.
    return false;
  } else {
    // but collections can...
    return true;
  }
  // TODO: It might be possible to get akonadi to tell us if a collection is empty or not and use that information instead of assuming all collections are not empty.
}

void EntityTreeModel::fetchMore ( const QModelIndex & parent )
{
  Q_D( EntityTreeModel );

  if ( d->m_itemPopulation == ImmediatePopulation )
    // Nothing to do. The items are already in the model.
    return;
  else if ( d->m_itemPopulation == LazyPopulation )
  {
    Collection col = parent.data(CollectionRole).value<Collection>();

    if (!col.isValid())
      return;

    kDebug() << col.id() << col.name();
    d->fetchItems( col, EntityTreeModelPrivate::Base );
  }
//   kDebug() << parent;
}

bool EntityTreeModel::hasChildren(const QModelIndex &parent ) const
{
  Q_D( const EntityTreeModel );
  // TODO: Empty collections right now will return true and get a little + to expand.
  // There is probably no way to tell if a collection
  // has child items in akonadi without first attempting an itemFetchJob...
  // Figure out a way to fix this.
  return ((rowCount(parent) > 0) || (canFetchMore(parent) && d->m_itemPopulation == LazyPopulation));
}

bool EntityTreeModel::insertRows(int , int, const QModelIndex&)
{
    return false;
}

bool EntityTreeModel::insertColumns(int, int, const QModelIndex&)
{
    return false;
}

bool EntityTreeModel::removeRows(int, int, const QModelIndex&)
{
    return false;
}

bool EntityTreeModel::removeColumns(int, int, const QModelIndex&)
{
    return false;
}

void EntityTreeModel::setRootCollection(Collection col)
{
  Q_D(EntityTreeModel);

  Q_ASSERT(col.isValid());
  d->m_rootCollection = col;
  clearAndReset();
}

Collection EntityTreeModel::rootCollection() const
{
  Q_D(const EntityTreeModel);
  return d->m_rootCollection;
}

QModelIndex EntityTreeModel::indexForCollection(Collection col) const
{
  Q_D(const EntityTreeModel);

  // TODO: will this work for collection::root while showing it?

  int row = d->m_childEntities.value(col.parent()).indexOf(col.id());
  if (row < 0)
    return QModelIndex();
  int column = 0;
  return createIndex(row, column, reinterpret_cast<void *>(col.id()));
}

QModelIndex EntityTreeModel::indexForItem(Item item) const
{
  Q_D(const EntityTreeModel);
  Collection col = d->getParentCollection(item);
  qint64 id = item.id() * - 1;
  int row = d->m_childEntities.value(col.id()).indexOf(id);
  int column = 0;
  return createIndex(row, column, reinterpret_cast<void *>(id));

}

void EntityTreeModel::fetchMimeTypes(QStringList mimeTypes)
{
  Q_D(EntityTreeModel);
  d->m_mimeTypeFilter = mimeTypes;
  clearAndReset();
}

QStringList EntityTreeModel::mimeTypesToFetch() const
{
  Q_D(const EntityTreeModel);
  return d->m_mimeTypeFilter;
}

void EntityTreeModel::setItemPopulationStrategy(int type)
{
  Q_D(EntityTreeModel);
  d->m_itemPopulation = type;
  clearAndReset();
}

int EntityTreeModel::itemPopulationStrategy() const
{
  Q_D(const EntityTreeModel);
  return d->m_itemPopulation;
}

void EntityTreeModel::setIncludeRootCollection(bool include)
{
  Q_D(EntityTreeModel);
  d->m_showRootCollection = include;
  clearAndReset();
}

bool EntityTreeModel::includeRootCollection() const
{
  Q_D(const EntityTreeModel);
  return d->m_showRootCollection;
}

void EntityTreeModel::setRootCollectionDisplayName(const QString &displayName)
{
  Q_D(EntityTreeModel);
  d->m_rootCollectionDisplayName = displayName;

  // TODO: Emit datachanged if it is being shown.
}

QString EntityTreeModel::rootCollectionDisplayName() const
{
  Q_D( const EntityTreeModel);
  return d->m_rootCollectionDisplayName;
}

void EntityTreeModel::setCollectionFetchStrategy(int type)
{
  Q_D( EntityTreeModel);
  d->m_collectionFetchStrategy = type;
  clearAndReset();
}

int EntityTreeModel::collectionFetchStrategy() const
{
  Q_D( const EntityTreeModel);
  return d->m_collectionFetchStrategy;
}

#include "entitytreemodel.moc"
