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

#ifndef AKONADI_ENTITYTREEMODEL_H
#define AKONADI_ENTITYTREEMODEL_H

#include "abstractitemmodel.h"

// #include <QtCore/QAbstractItemModel>
#include <QtCore/QStringList>

#include <akonadi/collection.h>
#include <akonadi/item.h>
#include "akonadi_next_export.h"

// TODO (Applies to all these 'new' models, not just EntityTreeModel):
// * Figure out how LazyPopulation and signals from monitor containing items should
//     fit together. Possibly store a list of collections whose items have already
//     been lazily fetched.
// * Fgure out whether DescendantEntitiesProxyModel needs to use fetchMore.
// * Profile this and DescendantEntitiesProxyModel. Make sure it's faster than
//     FlatCollectionProxyModel. See if the cache in that class can be cleared less often.
// * Unit tests. Much of the stuff here is not covered by modeltest, and some of
//     it is akonadi specific, such as setting root collection etc.
// * Implement support for includeUnsubscribed.
// * Use CollectionStatistics for item count stuff. Find out if I can get stats by mimetype.
// * Make sure there are applications using it before committing to it until KDE5.
//     Some API/ virtual methods might need to be added when real applications are made.
// * Fix compiler warnings about unused variables. Could be mistakes about.
// * Implement ordering support.
// * Implement some proxy models for time-table like uses, eg KOrganizer events.
// * Apidox++
// * Handle 'structural' collections better. Possibly make the server tell us
//     the mimetypes of descendants of a collection. Eg,
//         Col0-0 (contentMimeTypes: text/foo, Collection::mimeType())
//         -> Col0-1 (contentMimeTypes: Collection::mimeType())
//         -> -> Col 0-2 (contentMimeTypes: text/foo, Collection::mimeType())
//
//     If a tree in the akonadi server had the above structure, and
//     fetchMimeTypes(text/foo) was set, Col0-2 would not make it into the model.
//     This is because the model does Breadth first traversal and never fetches anything
//     after Col0-1 because it doesn't match the mimetype we're interested in.
// * The API for tying proxies together is currently a bit cumbersome. Write a
//     glue class to make stringing them together easier.

namespace Akonadi
{
class Item;
class CollectionStatistics;
class Monitor;
class Session;
class ItemFetchScope;

class EntityTreeModelPrivate;

/**
 * @short A model for collections and items together.
 *
 * This class provides the interface of QAbstractItemModel for the
 * collection and item tree of the Akonadi storage.
 *
 * Child elements of a collection consist of the child collections
 * followed by the items. This arrangement can be modified using a proxy model.
 *
 * @code
 *
 *   Monitor *monitor = new Monitor(this);
 *   monitor->setCollectionMonitored(Collection::root());
 *
 *   EntityUpdateAdapter *eua = new EntityUpdateAdapter(this);
 *
 *   EntityTreeModel *model = new EntityTreeModel( eua, monitor, this );
 *   model->fetchMimeTypes( QStringList() << FooMimeType << BarMimeType );
 *
 *   EntityTreeView *view = new EntityTreeView( this );
 *   view->setModel( model );
 *
 * @endcode
 *
 * @author Stephen Kelly <steveire@gmail.com>
 * @since 4.3
 */
class AKONADI_NEXT_EXPORT EntityTreeModel : public AbstractItemModel
{
  Q_OBJECT

public:
  /**
   * Describes the roles for items. Roles for collections are defined by the superclass.
   */
  enum Roles {
    CollectionRole = Qt::UserRole,          ///< The collection.
    CollectionIdRole,                       ///< The collection id.
    ItemRole,                               ///< The Item
    ItemIdRole,                             ///< The item id
    MimeTypeRole,                           ///< The mimetype of the entity
    RemoteIdRole,                           ///< The remoteId of the entity
    CollectionChildOrderRole,               ///< Ordered list of child items if available
    UserRole = Qt::UserRole + 1000,         ///< Role for user extensions.
    TerminalUserRole = 1000000              ///< Last role for user extensions. Don't use a role beyond this or headerData will break.
  };

  enum HeaderGroup {
    EntityTreeHeaders,
    CollectionTreeHeaders,
    ItemListHeaders
    // Could we need more here?
  };

  // EntityTreeModel( EntityUpdateAdapter,
  //                  MonitorAdapter,
  //                  QStringList mimeFilter = QStringList(), QObject *parent = 0);

  /**
   * Creates a new EntityTreeModel
   *
   * @param parent The parent object.
   * @param mimeTypes The list of mimetypes to be retrieved in the model.
   */
  EntityTreeModel( Session *session,
                   Monitor *monitor,
                   QObject *parent = 0
                  );

  /**
   * Destroys the entityTreeModel.
   */
  virtual ~EntityTreeModel();

  /**
  How the model should be populated with items.
  */
  enum ItemPopulationStrategy {
    NoItemPopulation, ///< Do not include items in the model.
    ImmediatePopulation, ///< Retrieve items immediately when their parent is in the model. This is the default.
    LazyPopulation ///< Fetch items only when requested (using canFetchMore/fetchMore)
  };

  void setItemPopulationStrategy(int type);
  int itemPopulationStrategy() const;

  /**
  The root collection to create an entity tree for. By default the Collection::root() is used.

  The Collection @p col must be valid.
  */
  void setRootCollection(Collection col);
  Collection rootCollection() const;

  void setIncludeRootCollection(bool include);
  bool includeRootCollection() const;

  /**
  If Collection::root() is shown in the model, set a displayName for it.
  Default is "[*]"
  If another collection (ie not Collection::root()), is at the root, it's name
  or display attribute is automatically used instead and this method as no effect.
  */
  void setRootCollectionDisplayName(const QString &displayName);
  QString rootCollectionDisplayName() const;

  /**
  What to fetch and represent in the model.
  */
  enum CollectionsToFetch {
    FetchNoCollections,                     /// Fetch nothing. This creates an empty model.
    FetchFirstLevelChildCollections,        /// Fetch first level collections in the root collection.
    FetchCollectionsRecursive               /// Fetch collections in the root collection recursively. This is the default.
  };

  void setCollectionFetchStrategy(int type);
  int collectionFetchStrategy() const;

  void setIncludeUnsubscribed(bool include);
  bool includeUnsubscribed() const;

  QModelIndex indexForCollection(Collection col) const;
  QModelIndex indexForItem(Item item) const;

  Collection getCollection(Collection::Id);
  Item getItem(Item::Id);

  virtual int columnCount( const QModelIndex & parent = QModelIndex() ) const;
  virtual int rowCount( const QModelIndex & parent = QModelIndex() ) const;

  virtual QVariant data( const QModelIndex & index, int role = Qt::DisplayRole ) const;
  virtual QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;

  virtual Qt::ItemFlags flags( const QModelIndex &index ) const;
  virtual QStringList mimeTypes() const;

  virtual Qt::DropActions supportedDropActions() const;
  virtual QMimeData *mimeData( const QModelIndexList &indexes ) const;
  virtual bool dropMimeData( const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent );
  virtual bool setData( const QModelIndex &index, const QVariant &value, int role = Qt::EditRole );

  virtual QModelIndex index( int row, int column, const QModelIndex & parent = QModelIndex() ) const;
  virtual QModelIndex parent( const QModelIndex & index ) const;

// TODO: Review the implementations of these. I think they could be better.
  virtual bool canFetchMore ( const QModelIndex & parent ) const;
  virtual void fetchMore ( const QModelIndex & parent );
  virtual bool hasChildren(const QModelIndex &parent = QModelIndex() ) const;

protected:
  /**
  Clears and resets the model. Always call this instead of the reset method in the superclass. Using the reset method will not reliably clear or refill the model
  */
  void clearAndReset();

  virtual QVariant getData(Item item, int column, int role=Qt::DisplayRole) const;

  virtual QVariant getData(Collection collection, int column, int role=Qt::DisplayRole) const;

  virtual QVariant getHeaderData(int section, Qt::Orientation orientation, int role, int headerSet) const;

private:
  Q_DECLARE_PRIVATE( EntityTreeModel )
  //@cond PRIVATE
  EntityTreeModelPrivate *d_ptr;

  // Make these private, they shouldn't be called by applications
  virtual bool insertRows(int , int, const QModelIndex & = QModelIndex());
  virtual bool insertColumns(int, int, const QModelIndex & = QModelIndex());
  virtual bool removeRows(int, int, const QModelIndex & = QModelIndex());
  virtual bool removeColumns(int, int, const QModelIndex & = QModelIndex());

  Q_PRIVATE_SLOT( d_func(), void startFirstListJob() )
//   Q_PRIVATE_SLOT( d_func(), void slotModelReset() )

  Q_PRIVATE_SLOT( d_func(), void fetchJobDone( KJob *job ) )
  Q_PRIVATE_SLOT( d_func(), void updateJobDone( KJob *job ) )
  Q_PRIVATE_SLOT( d_func(), void itemsFetched( Akonadi::Item::List list ) )
  Q_PRIVATE_SLOT( d_func(), void collectionsFetched( Akonadi::Collection::List list ) )

  Q_PRIVATE_SLOT( d_func(), void monitoredCollectionAdded( const Akonadi::Collection&, const Akonadi::Collection& ) )
  Q_PRIVATE_SLOT( d_func(), void monitoredCollectionRemoved( const Akonadi::Collection& ) )
  Q_PRIVATE_SLOT( d_func(), void monitoredCollectionChanged( const Akonadi::Collection& ) )
  Q_PRIVATE_SLOT( d_func(), void monitoredCollectionMoved( const Akonadi::Collection&, const Akonadi::Collection&, const Akonadi::Collection&) )

  Q_PRIVATE_SLOT( d_func(), void monitoredItemAdded( const Akonadi::Item&, const Akonadi::Collection& ) )
  Q_PRIVATE_SLOT( d_func(), void monitoredItemRemoved( const Akonadi::Item& ) )
  Q_PRIVATE_SLOT( d_func(), void monitoredItemChanged( const Akonadi::Item&, const QSet<QByteArray>& ) )
  Q_PRIVATE_SLOT( d_func(), void monitoredItemMoved( const Akonadi::Item&,
                  const Akonadi::Collection&, const Akonadi::Collection& ) )

  //@endcond


};

} // namespace

#endif
