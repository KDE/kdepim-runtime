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

#include "storecollectionmodel.h"

#include <akonadi/collection.h>

#include <KDebug>
#include <KLocale>

#include <QtGui/QFont>

using namespace Akonadi;

StoreCollectionModel::StoreCollectionModel( QObject *parent )
  : CollectionModel( parent )
{
}

int StoreCollectionModel::columnCount( const QModelIndex &parent ) const
{
  if ( parent.isValid() && parent.column() != 0 ) {
    return 0;
  }

  return 2;
}

QVariant StoreCollectionModel::data( const QModelIndex &index, int role ) const
{
  if ( !index.isValid() ) {
    return QVariant();
  }

  Collection collection = collectionForId( CollectionModel::data( index, CollectionIdRole ).toLongLong() );
  if ( !collection.isValid() ) {
    return QVariant();
  }

  if ( index.column() == 1 && (role == ItemTypeRole || role == Qt::DisplayRole ) ) {
    QStringList itemTypes = mStoreMapping.value( collection.id(), QStringList() );
    itemTypes.sort();
    return itemTypes.join( QLatin1String( ", " ) );
  }

  return CollectionModel::data( index, role );
}

QVariant StoreCollectionModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
  if ( orientation == Qt::Horizontal && role == Qt::DisplayRole )
    if ( section == 1 ) {
      return i18nc( "@title:column, data types which should be stored here by default",
                    "Defaults" );
    }

  return CollectionModel::headerData( section, orientation, role );
}

void StoreCollectionModel::setStoreMapping( const StoreCollectionModel::StoreItemsByCollection &storeMapping )
{
  if ( mStoreMapping != storeMapping ) {
    mStoreMapping = storeMapping;
    reset();
  }
}

StoreCollectionModel::StoreItemsByCollection StoreCollectionModel::storeMapping() const
{
  return mStoreMapping;
}

#include "storecollectionmodel.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
