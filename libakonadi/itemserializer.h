/*
    Copyright (c) 2007 Till Adam <adam@kde.org>

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

#ifndef AKONADI_ITEM_SERIALIZER_H
#define AKONADI_ITEM_SERIALIZER_H

#include "libakonadi_export.h"

class QString;
class QByteArray;
class QIODevice;

namespace Akonadi {

class Item;

/**
  Base class for PIM item type serializer plugins.
*/
class ItemSerializerPlugin
{
public:
    virtual ~ItemSerializerPlugin() { };
    /**
      De-serialize item part @p label from @p data.
      If the given item part is not supported, call Item::addPart().
    */
    virtual void deserialize( Item& item, const QString& label, const QByteArray& data ) const = 0;
    virtual void deserialize( Item& item, const QString& label, const QIODevice& data ) const = 0;

    /**
      Serialize item part @p label into @p data.
      If the given part is not supported, do what???
      @todo what happens with unsupported parts.
    */
    virtual void serialize( const Item& item, const QString& label, QByteArray& data ) const = 0;
    virtual void serialize( const Item& item, const QString& label, QIODevice& data ) const = 0;
};


class AKONADI_EXPORT ItemSerializerException
{
};

/**
*/
class AKONADI_EXPORT ItemSerializer
{
  public:
      /** throws ItemSerializerException on failure */
      static void deserialize( Item& item, const QString& label, const QByteArray& data );
      /** throws ItemSerializerException on failure */
      static void deserialize( Item& item, const QString& label, const QIODevice& data );
      /** throws ItemSerializerException on failure */
      static void serialize( const Item& item, const QString& label, QByteArray& data );
      /** throws ItemSerializerException on failure */
      static void serialize( const Item& item, const QString& label, QIODevice& data );

  private:
      static const ItemSerializerPlugin& pluginForMimeType( const QString& mimetype );
};

}

#endif
