/*
    Copyright (c) 2006 Volker Krause <volker.krause@rwth-aachen.de>

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

#ifndef PIM_COLLECTIONATTRIBUTE_H
#define PIM_COLLECTIONATTRIBUTE_H

#include <QtCore/QByteArray>
#include <QtCore/QList>

namespace PIM {

/**
  Stores specific collection attributes (ACLs, unread counts, quotas, etc.).
  Every collection can have zero ore one attribute of every type.
*/
class CollectionAttribute
{
  public:
    typedef QList<CollectionAttribute*> List;

    /**
      Returns the attribute name of this collection.
    */
    virtual QByteArray type() const = 0;

    /**
      Destroys this attribute.
    */
    virtual ~CollectionAttribute();
};


/**
  Content mimetypes collection attribute. Contains a list of allowed content
  types of a collection.
*/
class CollectionContentTypeAttribute : public CollectionAttribute
{
  public:
    /**
      Creates a new content mimetype attribute.
      @param contentTypes Allowed content types.
    */
    CollectionContentTypeAttribute( const QList<QByteArray> &contentTypes = QList<QByteArray>() );

    virtual  QByteArray type() const;

    /**
      Returns the allowed content mimetypes.
    */
    QList<QByteArray> contentTypes() const;

    /**
      Sets the allowed content mimetypes.
      @param contentTypes Allowed content types.
    */
    void setContentTypes( const QList<QByteArray> &contentTypes );

  private:
    QList<QByteArray> mContentTypes;
};

}

#endif
