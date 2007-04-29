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

#include "../libakonadi/item.h"

using namespace Akonadi;

void SerializerPluginAddresee::deserialize( Item& item, const QString& label, const QByteArray& data ) const
{
    if ( label != "RFC822" ) {
      item.addPart( label, data );
      return;
    }
    if ( item.mimeType() != QString::fromLatin1("text/vcard") ) {
        //throw ItemSerializerException();
        return;
    }

    KABC::Addressee a = const_cast<KABC::VCardConverter*>(&m_converter)->parseVCard( data );
    if ( !a.isEmpty() ) {
        item.setPayload<KABC::Addressee>( a );
    } else {
        qDebug( ) << "SerializerPluginAddresee: Empty addressee object!";
    }


}


void SerializerPluginAddresee::deserialize( Item& item, const QString& label, const QIODevice& data ) const
{
  Q_UNUSED( item );
  Q_UNUSED( label );
  Q_UNUSED( data );
}


void SerializerPluginAddresee::serialize( const Item& item, const QString& label, QByteArray& data ) const
{
    if ( label != "RFC822" || !item.hasPayload() )
      return;
    const KABC::Addressee a = item.payload<KABC::Addressee>();
    data = m_converter.createVCard( a );
}


void SerializerPluginAddresee::serialize( const Item& item, const QString& label, QIODevice& data ) const
{
  Q_UNUSED( item );
  Q_UNUSED( label );
  Q_UNUSED( data );
}

extern "C"
KDE_EXPORT Akonadi::ItemSerializerPlugin *
libakonadi_serializer_addressee_create_item_serializer_plugin() {
  return new SerializerPluginAddresee();
}


