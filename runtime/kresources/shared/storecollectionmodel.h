/*
    This file is part of kdepim.
    Copyright (c) 2009 Kevin Krammer <kevin.krammer@gmx.at>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef KRES_AKONADI_STORECOLLECTIONMODEL_H
#define KRES_AKONADI_STORECOLLECTIONMODEL_H

#include <akonadi/collection.h>
#include <akonadi/collectionmodel.h>

#include <QHash>

namespace Akonadi {

class StoreCollectionModel : public CollectionModel
{
  Q_OBJECT

  public:
    typedef QHash<Collection::Id, QStringList> StoreItemsByCollection;

    enum Roles {
      ItemTypeRole = CollectionModel::UserRole + 1,
      UserRole = CollectionModel::UserRole + 8
    };

    explicit StoreCollectionModel( QObject *parent = 0 );

    virtual int columnCount( const QModelIndex & parent = QModelIndex() ) const;

    virtual QVariant data( const QModelIndex & index, int role = Qt::DisplayRole ) const;

    virtual QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;

    void setStoreMapping( const StoreItemsByCollection &storeMapping );

    StoreItemsByCollection storeMapping() const;

  protected:
    StoreItemsByCollection mStoreMapping;
};

}

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
