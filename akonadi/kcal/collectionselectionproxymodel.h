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

#include "akonadi-kcal_export.h"

#include <QSortFilterProxyModel>

class QItemSelectionModel;

namespace Akonadi {

    class AKONADI_KCAL_EXPORT CollectionSelectionProxyModel : public QSortFilterProxyModel
    {
    public:
        explicit CollectionSelectionProxyModel( QObject *parent=0 );
        ~CollectionSelectionProxyModel();

        QItemSelectionModel *selectionModel() const;
        void setSelectionModel( QItemSelectionModel *selectionModel );
        
        /* reimp */ Qt::ItemFlags flags( const QModelIndex &index ) const;

        /* reimp */ QVariant data( const QModelIndex &index, int role = Qt::DisplayRole ) const;

        /* reimp */ bool setData( const QModelIndex &index, const QVariant &value, int role = Qt::EditRole );

        /* reimp */ bool filterAcceptsColumn( int source_column, const QModelIndex& source_parent ) const;
        
    private:
        class Private;
        Private * const d;
    };
}

#endif // AKONADI_COLLECTIONSELECTIONPROXYMODEL_H
