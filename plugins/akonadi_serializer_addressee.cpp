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

#include <QDebug>

#include <libakonadi/item.h>

using namespace Akonadi;

void SerializerPluginAddresee::deserialize( Item& item, const QString& label, QIODevice& data )
{
    if ( label != "RFC822" ) {
      item.addPart( label, data.readAll() );
      return;
    }
    if ( item.mimeType() != QString::fromLatin1("text/vcard") && item.mimeType() != QString::fromLatin1("text/directory") ) {
        //throw ItemSerializerException();
        return;
    }

    KABC::Addressee a = m_converter.parseVCard( data.readAll() );
    if ( !a.isEmpty() ) {
        item.setPayload<KABC::Addressee>( a );
    } else {
        qDebug( ) << "SerializerPluginAddresee: Empty addressee object!";
    }
}

void SerializerPluginAddresee::serialize( const Item& item, const QString& label, QIODevice& data )
{
    if ( label != "RFC822" || !item.hasPayload<KABC::Addressee>() )
      return;
    const KABC::Addressee a = item.payload<KABC::Addressee>();
    data.write( m_converter.createVCard( a ) );
}

extern "C"
KDE_EXPORT Akonadi::ItemSerializerPlugin *
libakonadi_serializer_addressee_create_item_serializer_plugin() {
  return new SerializerPluginAddresee();
}
