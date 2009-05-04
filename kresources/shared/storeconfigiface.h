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

#ifndef KRES_AKONADI_STORECONFIGIFACE_H
#define KRES_AKONADI_STORECONFIGIFACE_H

#include <QHash>

namespace Akonadi {
  class Collection;
}

class AbstractSubResourceModel;

class StoreConfigIface
{
  public:
    typedef QHash<QString, Akonadi::Collection> CollectionsByMimeType;

    virtual ~StoreConfigIface() {}

    virtual void setStoreCollection( const Akonadi::Collection& collection ) = 0;

    virtual Akonadi::Collection storeCollection() const = 0;

    virtual CollectionsByMimeType storeCollectionsByMimeType() const = 0;

    virtual void setStoreCollectionsByMimeType( const CollectionsByMimeType &collections ) = 0;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
