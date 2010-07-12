/*  This file is part of the KDE project
    Copyright (C) 2009,2010 Kevin Krammer <kevin.krammer@gmx.at>

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

#ifndef AKONADI_FILESTORE_STOREINTERFACE_H
#define AKONADI_FILESTORE_STOREINTERFACE_H

#include "akonadi-filestore_export.h"

// TODO not nice, collection fetch type should probably be in its own header
#include "collectionfetchjob.h"

namespace Akonadi
{
  class Collection;
  class CollectionFetchScope;
  class Item;
  class ItemFetchScope;

namespace FileStore
{
  class CollectionCreateJob;
  class CollectionDeleteJob;
  class CollectionFetchJob;
  class CollectionModifyJob;
  class CollectionMoveJob;
  class ItemCreateJob;
  class ItemDeleteJob;
  class ItemFetchJob;
  class ItemModifyJob;
  class ItemMoveJob;
  class StoreCompactJob;

/**
 */
class AKONADI_FILESTORE_EXPORT StoreInterface
{
  public:
    virtual ~StoreInterface() {}

    virtual Collection topLevelCollection() const = 0;

    virtual CollectionCreateJob *createCollection( const Collection &collection, const Collection &targetParent ) = 0;

    virtual CollectionFetchJob *fetchCollections( const Collection &collection, CollectionFetchJob::Type type = CollectionFetchJob::FirstLevel ) const = 0;

    virtual CollectionDeleteJob *deleteCollection( const Collection &collection ) = 0;

    virtual CollectionModifyJob *modifyCollection( const Collection &collection ) = 0;

    virtual CollectionMoveJob *moveCollection( const Collection &collection, const Collection &targetParent ) = 0;

    virtual ItemFetchJob *fetchItems( const Collection &collection ) const = 0;

    virtual ItemFetchJob *fetchItem( const Item &item ) const = 0;

    virtual ItemCreateJob *createItem( const Item &item, const Collection &collection ) = 0;

    virtual ItemModifyJob *modifyItem( const Item &item ) = 0;

    virtual ItemDeleteJob *deleteItem( const Item &item ) = 0;

    virtual ItemMoveJob *moveItem( const Item &item, const Collection &targetParent ) = 0;

    virtual StoreCompactJob *compactStore() = 0;

  protected:
    virtual void setTopLevelCollection( const Collection &collection ) = 0;
};

}
}

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
