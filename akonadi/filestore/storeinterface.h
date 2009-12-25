/*  This file is part of the KDE project
    Copyright (C) 2009 Kevin Krammer <kevin.krammer@gmx.at>

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
#include <akonadi/filestore/collectionfetchjob.h>

namespace Akonadi
{
class Collection;
class CollectionFetchScope;
class Item;
class ItemFetchScope;

namespace FileStore
{

class ItemCreateJob;
class ItemDeleteJob;
class ItemFetchJob;
class ItemModifyJob;
class StoreCompactJob;

/**
 */
class AKONADI_FILESTORE_EXPORT StoreInterface
{
  public:
    virtual ~StoreInterface() {}

    virtual Collection topLevelCollection() const = 0;

    virtual CollectionFetchJob *fetchCollections( const Collection &collection, CollectionFetchJob::Type type = CollectionFetchJob::FirstLevel, const CollectionFetchScope *fetchScope = 0 ) const = 0;

    virtual ItemFetchJob *fetchItems( const Collection &collection, const ItemFetchScope *fetchScope = 0 ) const = 0;

    virtual ItemFetchJob *fetchItem( const Item &item, const ItemFetchScope *fetchScope = 0 ) const = 0;

    virtual ItemCreateJob *createItem( const Item &item, const Collection &collection ) = 0;

    virtual ItemModifyJob *modifyItem( const Item &item, bool ignorePayload = false ) = 0;

    virtual ItemDeleteJob *deleteItem( const Item &item ) = 0;

    virtual StoreCompactJob *compactStore() = 0;

  protected:
    virtual void setTopLevelCollection( const Collection &collection ) = 0;
};

}
}

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
