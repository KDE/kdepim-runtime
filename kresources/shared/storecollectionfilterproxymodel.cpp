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

#include "storecollectionfilterproxymodel.h"

#include "abstractsubresourcemodel.h"
#include "subresourcebase.h"

#include <akonadi/collectionmodel.h>

using namespace Akonadi;

StoreCollectionFilterProxyModel::StoreCollectionFilterProxyModel( QObject *parent )
  : CollectionFilterProxyModel( parent ),
    mSubResourceModel( 0 )
{
}

StoreCollectionFilterProxyModel::~StoreCollectionFilterProxyModel()
{
}

void StoreCollectionFilterProxyModel::setSubResourceModel( const AbstractSubResourceModel *subResourceModel )
{
  if ( mSubResourceModel != subResourceModel ) {
    mSubResourceModel = subResourceModel;
    invalidateFilter();
  }
}

bool StoreCollectionFilterProxyModel::filterAcceptsRow( int sourceRow,
                                                        const QModelIndex &sourceParent) const
{
  // if the base class rejects it, it doesn't match our MIME type criteria
  if ( !CollectionFilterProxyModel::filterAcceptsRow( sourceRow, sourceParent ) ) {
    return false;
  }

  QModelIndex index = sourceModel()->index( sourceRow, 0, sourceParent );
  if ( index.isValid() ) {
    QVariant data = sourceModel()->data( index, CollectionModel::CollectionRole );
    if ( data.isValid() ) {
      Collection collection = data.value<Collection>();
      if ( collection.isValid() ) {
        // only accept collections into which we can store new items
        if ( ( collection.rights() & Collection::CanCreateItem ) != 0 ) {
          // if we have a sub resource model, check the associated sub resource,
          // otherwise just accept
          if ( mSubResourceModel != 0 ) {
            SubResourceBase *subResource = mSubResourceModel->subResourceBase( collection.id() );

            // only accept sub resources which are active
            return subResource != 0 && subResource->isActive();
          }

          return true;
        }
      }
    }
  }

  return false;
}

// kate: space-indent on; indent-width 2; replace-tabs on;
