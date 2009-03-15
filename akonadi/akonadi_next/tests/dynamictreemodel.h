
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

  ModelChangeCommand( DynamicTreeModel *model, QObject *parent = 0 );

  virtual ~ModelChangeCommand() {}

  void setAncestorRowNumbers(QList<int> rowNumbers) { m_rowNumbers = rowNumbers; }

  QModelIndex findIndex(QList<int> rows);

  void setStartRow(int row) { m_startRow = row; }

  void setEndRow(int row) { m_endRow = row; }

  void setNumCols(int cols) { m_numCols = cols; }

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

  ModelInsertCommand(DynamicTreeModel *model, QObject *parent = 0 );
  virtual ~ModelInsertCommand() {}

  virtual void doCommand();
};

class ModelRemoveCommand : public ModelChangeCommand
{
  Q_OBJECT
public:
  ModelRemoveCommand(DynamicTreeModel *model, QObject *parent = 0 );
  virtual ~ModelRemoveCommand() {}

  virtual void doCommand();

  void purgeItem(qint64 parent);
};

class ModelDataChangeCommand : public ModelChangeCommand
{
  Q_OBJECT
public:
  ModelDataChangeCommand(DynamicTreeModel *model, QObject *parent = 0);

  virtual ~ModelDataChangeCommand() {}

  virtual void doCommand();

  void setStartColumn(int column) { m_startColumn = column; }

protected:
  int m_startColumn;
};

class ModelMoveCommand : public ModelChangeCommand
{
  Q_OBJECT
public:
  ModelMoveCommand(DynamicTreeModel *model, QObject *parent);

  virtual ~ModelMoveCommand() {}

  virtual void doCommand();

  void setDestAncestors( QList<int> rows ) { m_destRowNumbers = rows; }

  void setDestRow(int row) { m_destRow = row; }

protected:
  QList<int> m_destRowNumbers;
  int m_destRow;
};


#endif
