
#include "dynamictreemodel.h"

#include <QHash>
#include <QList>
#include <QTimer>

#include <QDebug>

#include <kdebug.h>

DynamicTreeModel::DynamicTreeModel(QObject *parent)
  : QAbstractItemModel(parent),
    nextId(1)
{
}

QModelIndex DynamicTreeModel::index(int row, int column, const QModelIndex &parent) const
{
//   if (column != 0)
//     return QModelIndex();


  if ( column < 0 || row < 0 )
    return QModelIndex();

  QList<QList<qint64> > childIdColumns = m_childItems.value(parent.internalId());


  if (childIdColumns.size() == 0)
    return QModelIndex();

  if (column >= childIdColumns.size())
    return QModelIndex();

  QList<qint64> rowIds = childIdColumns.at(column);

//   kDebug() << rowIds;

  if ( row >= rowIds.size())
    return QModelIndex();

  qint64 id = rowIds.at(row);

  return createIndex(row, column, reinterpret_cast<void *>(id));

}

qint64 DynamicTreeModel::findParentId(qint64 searchId) const
{
  if (searchId <= 0)
    return -1;

  QHashIterator<qint64, QList<QList<qint64> > > i(m_childItems);
  while (i.hasNext())
  {
    i.next();
    QListIterator<QList<qint64> > j(i.value());
    while (j.hasNext())
    {
      QList<qint64> l = j.next();
      if (l.contains(searchId))
      {
        return i.key();
      }
    }
  }
  return -1;
}

QModelIndex DynamicTreeModel::parent(const QModelIndex &index) const
{
  if (!index.isValid())
    return QModelIndex();

  qint64 searchId = index.internalId();
  qint64 parentId = findParentId(searchId);
  // Will never happen for valid index, but what the hey...
  if (parentId <= 0)
    return QModelIndex();

  qint64 grandParentId = findParentId(parentId);
  if (grandParentId < 0)
    grandParentId = 0;

  int column = 0;
  QList<qint64> childList = m_childItems.value(grandParentId).at(column);

  int row = childList.indexOf(parentId);

  return createIndex(row, column, reinterpret_cast<void *>(parentId));

}

int DynamicTreeModel::rowCount(const QModelIndex &index ) const
{
  QList<QList<qint64> > cols = m_childItems.value(index.internalId());

  if (cols.size() == 0 )
    return 0;

  if (index.column() > 0)
    return 0;

  return cols.at(0).size();
}

int DynamicTreeModel::columnCount(const QModelIndex &index ) const
{
//   Q_UNUSED(index);
  return m_childItems.value(index.internalId()).size();
}

QVariant DynamicTreeModel::data(const QModelIndex &index, int role) const
{
  if (!index.isValid())
    return QVariant();

  if (Qt::DisplayRole == role)
  {
    return m_items.value(index.internalId());
  }
  return QVariant();
}
