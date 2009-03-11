
#include "dynamictreemodel.h"

#include <QHash>
#include <QList>
#include <QTimer>

#include <QDebug>

DynamicTreeModel::DynamicTreeModel(QObject *parent)
  : QAbstractItemModel(parent),
    nextId(1)
{
}

QModelIndex DynamicTreeModel::index(int row, int column, const QModelIndex &parent) const
{
  if (column != 0)
    return QModelIndex();

  QList<qint64> childIds = m_childItems.value(parent.internalId());

  if (row < 0 || row >= childIds.size())
    return QModelIndex();

  qint64 id = childIds.at(row);

  return createIndex(row, column, reinterpret_cast<void *>(id));
}

qint64 DynamicTreeModel::findParentId(qint64 searchId) const
{
  if (searchId <= 0)
    return -1;

  QHashIterator<qint64, QList<qint64> > i(m_childItems);
  while (i.hasNext())
  {
    i.next();
    if (i.value().contains(searchId))
    {
      return i.key();
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

  QList<qint64> childList = m_childItems.value(grandParentId);

  int row = childList.indexOf(parentId);
  int column = 0;

  return createIndex(row, column, reinterpret_cast<void *>(parentId));

}

int DynamicTreeModel::rowCount(const QModelIndex &index ) const
{
  return m_childItems.value(index.internalId()).size();
}

int DynamicTreeModel::columnCount(const QModelIndex &index ) const
{
  Q_UNUSED(index);
  return 1;
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
