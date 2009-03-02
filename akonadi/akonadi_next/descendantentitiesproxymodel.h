
#ifndef DESCENDANTENTITIESPROXYMODEL_H
#define DESCENDANTENTITIESPROXYMODEL_H

#include <QAbstractProxyModel>
// #include <QSortFilterProxyModel>

#include <akonadi/collection.h>


namespace Akonadi
{
class DescendantEntitiesProxyModelPrivate;

// Make this a proxy model? That way I have to write maptosource, but I can make the proxy model
// take a ClientSideEntityStorage so that it can directly get the same data as the source model.

class DescendantEntitiesProxyModel : public QAbstractProxyModel
// class DescendantEntitiesProxyModel : public QSortFilterProxyModel
{
  Q_OBJECT
public:

//   DescendantEntitiesProxyModel( EntityUpdateAdapter *entityUpdateAdapter,
//                                 ClientSideEntityStorage *clientSideEntityStorage,
//                                 Collection rootDescendCollection = Collection(),
//                                 QObject *parent = 0);
    DescendantEntitiesProxyModel( //ClientSideEntityStorage *clientSideEntityStorage,
//                                   Collection rootDescendCollection = Collection(),
//                                   QModelIndex rootDescendIndex,
                                  QObject *parent = 0 );

    ~DescendantEntitiesProxyModel();


    virtual void setSourceModel ( QAbstractItemModel * sourceModel );

    /**
    Sets the root index to @p index. This is the root of the proxy model.
    @param index The root index in the *source* model which will be shown in this model.
    if the index is invalid, the model is empty.
    */
    void setRootIndex( const QModelIndex &index);

    /**
    Set whether to show ancestor data in the model. If @p display is true, then
    a source model which is displayed as

    @code
    -> "Item 0-0" (this is row-depth)
    -> -> "Item 0-1"
    -> -> "Item 1-1"
    -> -> -> "Item 0-2"
    -> -> -> "Item 1-2"
    -> "Item 1-0"
    @endcode

    will be displayed as

    @code
    -> *Item 0-0"
    -> "Item 0-0 / Item 0-1"
    -> "Item 0-0 / Item 1-1"
    -> "Item 0-0 / Item 1-1 / Item 0-2"
    -> "Item 0-0 / Item 1-1 / Item 1-2"
    -> "Item 1-0"
    @endcode

    If @p display is false, the proxy will show

    @code
    -> *Item 0-0"
    -> "Item 0-1"
    -> "Item 1-1"
    -> "Item 0-2"
    -> "Item 1-2"
    -> "Item 1-0"
    @endcode

    Default is false.

    */
    void setDisplayAncestorData(bool display, const QString &sep = QString(" / "));

    /**
    Whether ancestor data will be displayed.
    */
    bool displayAncestorData() const;

    /**
    Separator used between data of ancestors.
    Returns a null QString() if displayAncestorData is false.
    */
    QString ancestorSeparator() const;

    QModelIndex mapFromSource ( const QModelIndex & sourceIndex ) const;
    QModelIndex mapToSource ( const QModelIndex & proxyIndex ) const;
    int descendantCount(const QModelIndex &index);

    QVariant data( const QModelIndex & index, int role = Qt::DisplayRole ) const;
    virtual int rowCount( const QModelIndex & parent = QModelIndex() ) const;

    virtual bool hasChildren ( const QModelIndex & parent = QModelIndex() ) const;
    virtual QModelIndex index(int, int, const QModelIndex&) const;
    virtual QModelIndex parent(const QModelIndex&) const;
    virtual int columnCount(const QModelIndex&) const;

//   virtual Qt::DropActions supportedDropActions() const;


private:
  Q_DECLARE_PRIVATE( DescendantEntitiesProxyModel )
  //@cond PRIVATE
  DescendantEntitiesProxyModelPrivate *d_ptr;

  Q_PRIVATE_SLOT(d_func(), void sourceRowsAboutToBeInserted(const QModelIndex &, int, int))
  Q_PRIVATE_SLOT(d_func(), void sourceRowsInserted(const QModelIndex &, int, int))
  Q_PRIVATE_SLOT(d_func(), void sourceRowsAboutToBeRemoved(const QModelIndex &, int, int))
  Q_PRIVATE_SLOT(d_func(), void sourceRowsRemoved(const QModelIndex &, int, int))
  Q_PRIVATE_SLOT(d_func(), void sourceModelAboutToBeReset())
  Q_PRIVATE_SLOT(d_func(), void sourceModelReset())
  Q_PRIVATE_SLOT(d_func(), void sourceLayoutAboutToBeChanged())
  Q_PRIVATE_SLOT(d_func(), void sourceLayoutChanged())
  Q_PRIVATE_SLOT(d_func(), void sourceDataChanged(const QModelIndex &, const QModelIndex &))

  // Make these private, they shouldn't be called by applications
//   virtual bool insertRows(int , int, const QModelIndex & = QModelIndex());
//   virtual bool insertColumns(int, int, const QModelIndex & = QModelIndex());
//   virtual bool removeRows(int, int, const QModelIndex & = QModelIndex());
//   virtual bool removeColumns(int, int, const QModelIndex & = QModelIndex());


  //@endcond
};

}

#endif
