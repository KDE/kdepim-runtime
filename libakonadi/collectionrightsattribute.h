/*
    Copyright (c) 2007 Tobias Koenig <tokoe@kde.org>

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

#ifndef AKONADI_COLLECTIONRIGHTSATTRIBUTE_H
#define AKONADI_COLLECTIONRIGHTSATTRIBUTE_H

#include "libakonadi_export.h"

#include "collection.h"
#include "collectionattribute.h"

namespace Akonadi {

/**
  Stores the rights attributes of a collection.

  You shouldn't use this class directly but the convenience methods
  Collection::rights() and Collection::setRights() instead.
*/
class AKONADI_EXPORT CollectionRightsAttribute : public CollectionAttribute
{
  public:
    /**
      Creates a new collection rights attribute.
     */
    CollectionRightsAttribute();

    /**
      Destroys the collection rights attribute.
     */
    ~CollectionRightsAttribute();

    /**
      Sets the @p rights of the collection.
     */
    void setRights( Collection::Rights rights );

    /**
      Returns the rights of the collection.
     */
    Collection::Rights rights() const;

    /**
      Inherited from CollectionAttribute.
     */
    virtual QByteArray type() const;
    virtual CollectionRightsAttribute* clone() const;
    virtual QByteArray toByteArray() const;
    virtual void setData( const QByteArray& );

  private:
    QByteArray mData;
};

}

#endif
