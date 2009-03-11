
#ifndef DYNAMICTREEMODEL_H
#define DYNAMICTREEMODEL_H

#include <QAbstractItemModel>

// #include "abstractitemmodel.h"

#include <QHash>
#include <QUuid>

#include <QDebug>

template class QList<qint64>;
// template class QHash<qint64, QList<qint64> >;

class DynamicTreeModel : public QAbstractItemModel
{
  Q_OBJECT

public:
  DynamicTreeModel(QObject *parent = 0);

  QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
  QModelIndex parent(const QModelIndex &index) const;
  int rowCount(const QModelIndex &index = QModelIndex()) const;
  int columnCount(const QModelIndex &index = QModelIndex()) const;

  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

protected slots:

  /**
  Finds the parent id of the string with id @p searchId.

  Returns -1 if not found.
  */
  qint64 findParentId(qint64 searchId) const;

private:
  QHash<qint64, QString> m_items;
  QHash<qint64, QList<qint64> > m_childItems;
  qint64 nextId;
  qint64 newId() { return nextId++; };

  QModelIndex m_nextParentIndex;
  int m_nextRow;

  int m_depth;
  int maxDepth;

  friend class ModelInsertCommand;
  friend class ModelRemoveCommand;
  friend class ModelDataChangeCommand;
//   friend class ModelMoveCommand;
  friend class ModelMoveLayoutChangeCommand;
//   friend class ModelSortIndexCommand;
  friend class ModelSortIndexLayoutChangeCommand;

};


class ModelChangeCommand : public QObject
{
  Q_OBJECT
public:

  ModelChangeCommand( DynamicTreeModel *model, QObject *parent = 0 )
    : QObject(parent), m_model(model) {}

  virtual ~ModelChangeCommand() {}

  void setAncestorRowNumbers(QList<int> rowNumbers) { m_rowNumbers = rowNumbers; }

  QModelIndex findIndex(QList<int> rows)
  {
    const int col = 0;
    QModelIndex parent = QModelIndex();
    QListIterator<int> i(rows);
    while (i.hasNext())
    {
      parent = m_model->index(i.next(), col, parent);
      Q_ASSERT(parent.isValid());
    }
    return parent;
  }

  void setStartRow(int row)
  {
    m_startRow = row;
  }

  void setEndRow(int row)
  {
    m_endRow = row;
  }

  virtual void doCommand() = 0;

protected:
  DynamicTreeModel* m_model;
  QList<int> m_rowNumbers;
  int m_startRow;
  int m_endRow;

};

class ModelInsertCommand : public ModelChangeCommand
{
  Q_OBJECT

public:

  ModelInsertCommand(DynamicTreeModel *model, QObject *parent = 0 )
    : ModelChangeCommand(model, parent) {}
  virtual ~ModelInsertCommand() {}

  virtual void doCommand()
  {
    QModelIndex parent = findIndex(m_rowNumbers);
    m_model->beginInsertRows(parent, m_startRow, m_endRow);
    qint64 parentId = parent.internalId();
    for (int row = m_startRow; row <= m_endRow; row++)
    {
//       QString name = QUuid::createUuid().toString();
      qint64 id = m_model->newId();
      QString name = QString::number(id);

      m_model->m_items.insert(id, name);
      m_model->m_childItems[parentId].insert(row, id);
    }
    m_model->endInsertRows();
  }
};

class ModelRemoveCommand : public ModelChangeCommand
{
  Q_OBJECT
public:
  ModelRemoveCommand(DynamicTreeModel *model, QObject *parent = 0 )
    : ModelChangeCommand(model, parent) {}
  virtual ~ModelRemoveCommand() {}

  virtual void doCommand()
  {
    QModelIndex parent = findIndex(m_rowNumbers);
    m_model->beginRemoveRows(parent, m_startRow, m_endRow);
    qint64 parentId = parent.internalId();
    QList<qint64> childItems = m_model->m_childItems.value(parentId);
    for (int row = m_startRow; row <= m_endRow; row++)
    {
        qint64 item = childItems[row];
        purgeItem(item);
        m_model->m_childItems[parentId].removeOne(item);
    }
    m_model->endRemoveRows();
  }

  void purgeItem(qint64 parent)
  {
    QList<qint64> childItems = m_model->m_childItems.value(parent);

    foreach(qint64 item, childItems)
    {
      purgeItem(item);
      m_model->m_childItems[parent].removeOne(item);
    }
    m_model->m_items.remove(parent);

  }

};

class ModelDataChangeCommand : public ModelChangeCommand
{
  Q_OBJECT
public:
  ModelDataChangeCommand(DynamicTreeModel *model, QObject *parent = 0)
    : ModelChangeCommand(model, parent)
  { }

  virtual ~ModelDataChangeCommand() {}

  virtual void doCommand()
  {
    QModelIndex parent = findIndex(m_rowNumbers);
    QModelIndex topLeft = m_model->index(m_startRow, 0, parent);
    QModelIndex bottomRight = m_model->index(m_endRow, 0, parent);

    QModelIndex idx = topLeft;
    while (idx.isValid())
    {
      QString name = QUuid::createUuid().toString();
      m_model->m_items[idx.internalId()] = name;
      idx = idx.sibling(idx.row() + 1, 0);
    }
    m_model->dataChanged(topLeft, bottomRight);
  }
};

#endif
