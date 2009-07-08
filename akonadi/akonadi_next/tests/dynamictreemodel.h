/*
    Copyright (c) 2009 Stephen Kelly <steveire@gmail.com>

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

#ifndef DYNAMICTREEMODEL_H
#define DYNAMICTREEMODEL_H

// #include <QAbstractItemModel>

#include "abstractitemmodel.h"

#include <QHash>
#include <QList>

#include <QDebug>

#include <kdebug.h>
#include "indexfinder.h"

template<typename T> class QList;

class DynamicTreeModel : public AbstractItemModel
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
  friend class ModelInsertWithDescendantsCommand;
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

class ModelInsertWithDescendantsCommand : public ModelInsertCommand
{
  Q_OBJECT

public:

  struct InsertFragment
  {
    int numRows;
    QHash<int, InsertFragment> subfragments;
  };

  ModelInsertWithDescendantsCommand(DynamicTreeModel *model, QObject *parent = 0);
  virtual ~ModelInsertWithDescendantsCommand() {}

  void setFragments(QList<InsertFragment> fragments);

  virtual void doCommand();

protected:
  void insertFragment(qint64 parentIdentifier, InsertFragment fragment);

  QList<InsertFragment> m_fragments;
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
