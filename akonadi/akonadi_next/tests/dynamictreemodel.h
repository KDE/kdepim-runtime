
#ifndef DYNAMICTREEMODEL_H
#define DYNAMICTREEMODEL_H

#include <QAbstractItemModel>

//#include "../abstractitemmodel.h"

#include <QHash>
#include <QList>
#include <QUuid>

#include <QDebug>

#include <kdebug.h>

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
  QHash<qint64, QList<QList<qint64> > > m_childItems;
  qint64 nextId;
  qint64 newId() { return nextId++; };

  QModelIndex m_nextParentIndex;
  int m_nextRow;

  int m_depth;
  int maxDepth;

  friend class ModelInsertCommand;
  friend class ModelRemoveCommand;
  friend class ModelDataChangeCommand;
  friend class ModelMoveCommand;
  friend class ModelMoveLayoutChangeCommand;
//   friend class ModelSortIndexCommand;
  friend class ModelSortIndexLayoutChangeCommand;

};


class ModelChangeCommand : public QObject
{
  Q_OBJECT
public:

  ModelChangeCommand( DynamicTreeModel *model, QObject *parent = 0 )
    : QObject(parent), m_model(model), m_numCols(1) {}

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

  void setNumCols(int cols)
  {
    m_numCols = cols;
  }

  virtual void doCommand() = 0;

protected:
  DynamicTreeModel* m_model;
  QList<int> m_rowNumbers;
  int m_startRow;
  int m_endRow;
  int m_numCols;

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
      for(int col = 0; col < m_numCols; col++ )
      {
        if (m_model->m_childItems[parentId].size() <= col)
        {
          m_model->m_childItems[parentId].append(QList<qint64>());
        }
  //       QString name = QUuid::createUuid().toString();
        qint64 id = m_model->newId();
        QString name = QString::number(id);

        m_model->m_items.insert(id, name);
        m_model->m_childItems[parentId][col].insert(row, id);

      }
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
    for(int col = 0; col < m_numCols; col++ )
    {
      QList<qint64> childItems = m_model->m_childItems.value(parentId).value(col);
      for (int row = m_startRow; row <= m_endRow; row++)
      {
        qint64 item = childItems[row];
        purgeItem(item);
        m_model->m_childItems[parentId][col].removeOne(item);
      }
    }
    m_model->endRemoveRows();
  }

  void purgeItem(qint64 parent)
  {
    QList<QList<qint64> > childItemRows = m_model->m_childItems.value(parent);

    if (childItemRows.size() > 0)
    {
      for (int col = 0; col < m_numCols; col++)
      {
        QList<qint64> childItems = childItemRows[col];
        foreach(qint64 item, childItems)
        {
          purgeItem(item);
          m_model->m_childItems[parent][col].removeOne(item);
        }
      }
    }
    m_model->m_items.remove(parent);

  }

};

class ModelDataChangeCommand : public ModelChangeCommand
{
  Q_OBJECT
public:
  ModelDataChangeCommand(DynamicTreeModel *model, QObject *parent = 0)
    : ModelChangeCommand(model, parent), m_startColumn(0)
  { }

  virtual ~ModelDataChangeCommand() {}

  virtual void doCommand()
  {
    QModelIndex parent = findIndex(m_rowNumbers);
    QModelIndex topLeft = m_model->index(m_startRow, m_startColumn, parent);
    QModelIndex bottomRight = m_model->index(m_endRow, m_numCols - 1, parent);

    QList<QList<qint64> > childItems = m_model->m_childItems[parent.internalId()];


    for (int col = m_startColumn; col < m_startColumn + m_numCols; col++)
    {
      for (int row = m_startRow; row < m_endRow; row++ )
      {
        kDebug() << col << row;
        QString name = QUuid::createUuid().toString();
        m_model->m_items[childItems[col][row]] = name;
      }
    }
    m_model->dataChanged(topLeft, bottomRight);
  }

protected:
  void setStartColumn(int column)
  {
    m_startColumn = column;
  }
  int m_startColumn;
};

class ModelMoveCommand : public ModelChangeCommand
{
  Q_OBJECT
public:
  ModelMoveCommand(DynamicTreeModel *model, QObject *parent)
    : ModelChangeCommand(model, parent)
  {}

  virtual ~ModelMoveCommand() {}

  virtual void doCommand()
  {
    QModelIndex srcParent = findIndex(m_rowNumbers);
    QModelIndex destParent = findIndex(m_destRowNumbers);

    m_model->beginMoveRows(srcParent, m_startRow, m_endRow, destParent, m_destRow);

    QList<qint64> l = m_model->m_childItems.value(srcParent.internalId())[0].mid(m_startRow, m_endRow - m_startRow + 1 );

//     kDebug() << "before remove";
//     kDebug() << "src" << m_model->m_childItems.value(srcParent.internalId())[0];
//     kDebug() << "dest" << m_model->m_childItems.value(destParent.internalId())[0];
    for (int i = m_startRow; i <= m_endRow ; i++)
    {
      m_model->m_childItems[srcParent.internalId()][0].removeAt(m_startRow);
//       m_model->m_childItems[srcParent.internalId()].move(m_startRow, d);
    }
//     kDebug() << "after remove" << m_model->m_childItems.value(srcParent.internalId())[0] << "dest" << m_model->m_childItems.value(destParent.internalId())[0];;

    int d;
    if (m_destRow < m_startRow)
      d = m_destRow;
    else
    {
      if (srcParent == destParent)
        d = m_destRow - (m_endRow - m_startRow + 1);
      else
        d = m_destRow - (m_endRow - m_startRow) + 1;
    }

//     kDebug() << "to insert" << l;
    foreach(const qint64 id, l)
    {
      m_model->m_childItems[destParent.internalId()][0].insert(d++, id);
    }
//     kDebug() << "after insert" << m_model->m_childItems.value(srcParent.internalId())[0] << "dest" << m_model->m_childItems.value(destParent.internalId())[0];;

    m_model->endMoveRows();
  }

  void setDestAncestors( QList<int> rows )
  {
    m_destRowNumbers = rows;
  }

  void setDestRow(int row)
  {
    m_destRow = row;
  }

protected:
  QList<int> m_destRowNumbers;
  int m_destRow;
};


#endif
