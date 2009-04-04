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


#ifndef AKONADI_ABSTRACTITEMMODEL_H
#define AKONADI_ABSTRACTITEMMODEL_H

// WARNING: Changes to this cless will probably also have to be made to AbstractProxyModel

#include <QAbstractItemModel>

class AbstractItemModelPrivate;

/**
This class adds some api considered missing from QAbstractItemModel.

It can notify observers explicitly about rows moving in the model, instead of emitting only layout{,AboutToBe}Changed.

You must call beginMoveRows before moving items in the model, and endMoveRows
after the operation has been completed.

Additionally, it can notify about the children of an index being rearranged, also , instead of emitting only layout{,AboutToBe}Changed.

This means that observers don't have to take persistent indexes of all items in the model, and can take indexes only of the children of that index.

You must call beginChangeChildOrder before moving items in the model, and endChangeChildOrder after the operation has been completed.

Additionally, methods are provided to emit separate signals before and after reseting the model.

/sa AbstractProxyModel.
*/
class AbstractItemModel : public QAbstractItemModel
{
  Q_OBJECT
public:
  AbstractItemModel(QObject *parent = 0 );
  virtual ~AbstractItemModel();

  protected:

  /**
  In the case of moving rows within a parent, the destinationRow should be the row <b>above</b> the move operation.

  eg, if the model can be represented as:
  @code
  (root)
  -> Item 0-0
  -> Item 1-0
  -> Item 2-0
  -> Item 3-0
  -> Item 4-0
  @endcode

  and Item 1-0 is to be moved to beneath Item 4-0, it should be put above the (non-existant) item at index 5.

  @code
  beginMoveRows(QModelIndex(), 1,1, QModelIndex(), 5);
  @endcode

  would be called.

  Moving a set of rows to a destination that overlaps the source has no effect.
  eg
  @code
  beginMoveRows(QModelIndex(), 1, 2, QModelIndex(), 1);
  beginMoveRows(QModelIndex(), 1, 2, QModelIndex(), 2);
  beginMoveRows(QModelIndex(), 1, 2, QModelIndex(), 3);
  @endcode
  all have no effect.
  */
  void beginMoveRows(const QModelIndex &srcParent, int start, int end, const QModelIndex &destinationParent, int destinationStart);

  /**
  Called when the row move operation is complete.
  */
  void endMoveRows();

  void beginMoveColumns(const QModelIndex &srcParent, int start, int end, const QModelIndex &destinationParent, int destinationStart);

  /**
  Called when the row move operation is complete.
  */
  void endMoveColumns();

  void beginChangeChildOrder(const QModelIndex &index);
  void endChangeChildOrder();

  void beginResetModel();
  void endResetModel();

signals:

#if !defined(Q_MOC_RUN) && !defined(qdoc)
private: // can only be emitted by AbstractItemModel
#endif

  void rowsAboutToBeMoved(const QModelIndex &parent, int start, int end, const QModelIndex &destination, int row);
  void rowsMoved(const QModelIndex &parent, int start, int end, const QModelIndex &destination, int row);

  void columnsAboutToBeMoved(const QModelIndex &parent, int start, int end, const QModelIndex &destination, int column);
  void columnsMoved(const QModelIndex &parent, int start, int end, const QModelIndex &destination, int column);

  void childOrderAboutToBeChanged(const QModelIndex &index);
  void childOrderChanged(const QModelIndex &index);

private:
  Q_DECLARE_PRIVATE(AbstractItemModel)

  AbstractItemModelPrivate *d_ptr;

};

#endif
