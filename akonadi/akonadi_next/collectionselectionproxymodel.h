/*
  This file is part of Akonadi.

  Copyright (C) 2009 KDAB Author: Sebastian Sauer <sebsauer@kdab.net>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

  As a special exception, permission is given to link this program
  with any edition of Qt, and distribute the resulting executable,
  without including the source code for Qt in the source distribution.
*/

#ifndef AKONADI_COLLECTIONSELECTIONPROXYMODEL_H
#define AKONADI_COLLECTIONSELECTIONPROXYMODEL_H

#include "akonadi_next_export.h"

#include <QtGui/QSortFilterProxyModel>

class QItemSelectionModel;

namespace Akonadi {

/**
 * @short A proxy model that allows selecting collections with checkboxes.
 */
class AKONADI_NEXT_EXPORT CollectionSelectionProxyModel : public QSortFilterProxyModel
{
  Q_OBJECT

  public:
    /**
     * Creates a new collection selection proxy model.
     *
     * @param parent The parent object.
     */
    explicit CollectionSelectionProxyModel( QObject *parent = 0 );

    /**
     * Destroys the collection selection proxy model.
     */
    ~CollectionSelectionProxyModel();

    /**
     * Sets the selection @p model the proxy model shall work on.
     */
    void setSelectionModel( QItemSelectionModel *model );

    /**
     * Returns the selection model the proxy model shall work on.
     */
    QItemSelectionModel *selectionModel() const;

    /**
     * Sets the @p column of the model that shall be checkable.
     */
    void setCheckableColumn( int column );

    /**
     * Returns the column of the model that shall be checkable.
     */
    int checkableColumn() const;

    /* reimp */ Qt::ItemFlags flags( const QModelIndex &index ) const;

    /* reimp */ QVariant data( const QModelIndex &index, int role = Qt::DisplayRole ) const;

    /* reimp */ bool setData( const QModelIndex &index, const QVariant &value, int role = Qt::EditRole );

    /* reimp */ bool filterAcceptsColumn( int source_column, const QModelIndex& source_parent ) const;

  private:
    //@cond PRIVATE
    class Private;
    Private * const d;
    //@endcond
};

}

#endif // AKONADI_COLLECTIONSELECTIONPROXYMODEL_H
