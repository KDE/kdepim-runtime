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

#include "akonadi_serializer_addressee.h"

#include <kabc/addressee.h>

#include <kdebug.h>

#include <akonadi/item.h>

using namespace Akonadi;

bool SerializerPluginAddressee::deserialize( Item& item, const QString& label, QIODevice& data )
{
    if ( label != Item::FullPayload )
      return false;

    KABC::Addressee a = m_converter.parseVCard( data.readAll() );
    if ( !a.isEmpty() ) {
        item.setPayload<KABC::Addressee>( a );
    } else {
        kWarning( 5261 ) << "Empty addressee object!";
    }
    return true;
}

void SerializerPluginAddressee::serialize( const Item& item, const QString& label, QIODevice& data )
{
    if ( label != Item::FullPayload || !item.hasPayload<KABC::Addressee>() )
      return;
    const KABC::Addressee a = item.payload<KABC::Addressee>();
    data.write( m_converter.createVCard( a ) );
}

extern "C"
KDE_EXPORT Akonadi::ItemSerializerPlugin *
akonadi_serializer_addressee_create_item_serializer_plugin() {
  return new SerializerPluginAddressee();
}
