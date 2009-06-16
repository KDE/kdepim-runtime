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

#ifndef SELECTIONPROXYMODEL_H
#define SELECTIONPROXYMODEL_H

#include <QItemSelectionModel>

#include "abstractproxymodel.h"
#include "akonadi_next_export.h"

namespace Akonadi
{

class SelectionProxyModelPrivate;

class AKONADI_NEXT_EXPORT SelectionProxyModel : public AbstractProxyModel
{
  Q_OBJECT
public:
  SelectionProxyModel(QItemSelectionModel *selectionModel, QObject *parent = 0 );

  virtual ~SelectionProxyModel();

  virtual void setSourceModel ( QAbstractItemModel * sourceModel );

  /**
    @code
    A
    B
    - C
    - - D
    - - - E
    - F
    G
    @endcode


    If B is selected,
      If omit is true: the proxy shows two levels of items.

      @code
      B
      - C
      @endcode

      else it shows all available levels:

      @code
      B
      - C
      - - D
      - - - E
      @endcode

  */
  void setOmitDescendants(bool omit);

  /**
    @code
    A
    B
    - C
    - - D
    - - - E
    - F
    G
    @endcode

    If startWithChildTrees is true,
      If B is selected, shows:

      @code
      C
      - D
      - - E
      F
      @endcode

    else show:
    @code
      B
      - C
      - - D
      - - - E
      - F
      @endcode
  */
  void setStartWithChildTrees(bool startWithChildTrees);

  /**
    @code
    A
    B
    - C
    - - D
    - - - E
    - F
    G
    @endcode

    Normally, if B and C are selected, only B and descendants appear in the proxy model because C (as a descendant)
    is already part of the model.

    However, if startWithChildTrees is true and omitDescendants is true, @p includeAllSelected can be set to true
    to process all selected indexes.

    If B and D are selected, it shows:

    @code
    C
    E
    F
    @endcode

    This method has no effect if either startWithChildTrees or omitDescendants are false.
  */
  void setIncludeAllSelected(bool includeAllSelected);

  QModelIndex mapFromSource ( const QModelIndex & sourceIndex ) const;
  QModelIndex mapToSource ( const QModelIndex & proxyIndex ) const;

  virtual Qt::ItemFlags flags( const QModelIndex &index ) const;
  QVariant data( const QModelIndex & index, int role = Qt::DisplayRole ) const;
  virtual int rowCount( const QModelIndex & parent = QModelIndex() ) const;
  virtual QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;

  virtual bool hasChildren ( const QModelIndex & parent = QModelIndex() ) const;
  virtual QModelIndex index(int, int, const QModelIndex&) const;
  virtual QModelIndex parent(const QModelIndex&) const;
  virtual int columnCount(const QModelIndex&) const;

private:
  Q_DECLARE_PRIVATE(SelectionProxyModel)
  //@cond PRIVATE
  SelectionProxyModelPrivate *d_ptr;

  Q_PRIVATE_SLOT(d_func(), void sourceRowsAboutToBeInserted(const QModelIndex &, int, int))
  Q_PRIVATE_SLOT(d_func(), void sourceRowsInserted(const QModelIndex &, int, int))
  Q_PRIVATE_SLOT(d_func(), void sourceRowsAboutToBeRemoved(const QModelIndex &, int, int))
  Q_PRIVATE_SLOT(d_func(), void sourceRowsRemoved(const QModelIndex &, int, int))
//   Q_PRIVATE_SLOT(d_func(), void sourceRowsAboutToBeMoved(const QModelIndex &, int, int, const QModelIndex &, int))
//   Q_PRIVATE_SLOT(d_func(), void sourceRowsMoved(const QModelIndex &, int, int, const QModelIndex &, int))
  Q_PRIVATE_SLOT(d_func(), void sourceModelAboutToBeReset())
  Q_PRIVATE_SLOT(d_func(), void sourceModelReset())
  Q_PRIVATE_SLOT(d_func(), void sourceLayoutAboutToBeChanged())
  Q_PRIVATE_SLOT(d_func(), void sourceLayoutChanged())
  Q_PRIVATE_SLOT(d_func(), void sourceDataChanged(const QModelIndex &, const QModelIndex &))
//
  Q_PRIVATE_SLOT(d_func(), void selectionChanged( const QItemSelection & selected, const QItemSelection & deselected ) )

  //@endcond

};

}

#endif
