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

#include <QtCore/qplugin.h>

#include <kabc/addressee.h>

#include <kdebug.h>

#include <akonadi/item.h>
#include <akonadi/kabc/contactparts.h>

using namespace Akonadi;

bool SerializerPluginAddressee::deserialize( Item& item, const QByteArray& label, QIODevice& data, int version )
{
    Q_UNUSED( version );

    KABC::Addressee addr;
    if ( label == Item::FullPayload ) {
        addr = m_converter.parseVCard( data.readAll() );
    } else if ( label == Akonadi::ContactPart::Standard ) {
        addr = m_converter.parseVCard( data.readAll() );

        // remove pictures and sound
        addr.setPhoto( KABC::Picture() );
        addr.setLogo( KABC::Picture() );
        addr.setSound( KABC::Sound() );
    } else if ( label == Akonadi::ContactPart::Lookup ) {
        const KABC::Addressee temp = m_converter.parseVCard( data.readAll() );

        // copy only uid, name and email addresses
        addr.setUid( temp.uid() );
        addr.setPrefix( temp.prefix() );
        addr.setGivenName( temp.givenName() );
        addr.setAdditionalName( temp.additionalName() );
        addr.setFamilyName( temp.familyName() );
        addr.setSuffix( temp.suffix() );
        addr.setEmails( temp.emails() );
    } else {
        return false;
    }

    if ( !addr.isEmpty() ) {
        item.setPayload<KABC::Addressee>( addr );
    } else {
        kWarning( 5261 ) << "Empty addressee object!";
    }

    return true;
}

void SerializerPluginAddressee::serialize( const Item& item, const QByteArray& label, QIODevice& data, int &version )
{
    Q_UNUSED( version );

    if ( label != Item::FullPayload && label != Akonadi::ContactPart::Standard && label != Akonadi::ContactPart::Lookup )
        return;

    if ( !item.hasPayload<KABC::Addressee>() )
        return;

    KABC::Addressee addr, temp;

    temp = item.payload<KABC::Addressee>();

    if ( label == Item::FullPayload ) {
        addr = temp;
    } else if ( label == Akonadi::ContactPart::Standard ) {
        addr = temp;

        // remove pictures and sound
        addr.setPhoto( KABC::Picture() );
        addr.setLogo( KABC::Picture() );
        addr.setSound( KABC::Sound() );
    } else if ( label == Akonadi::ContactPart::Lookup ) {
        // copy only uid, name and email addresses
        addr.setUid( temp.uid() );
        addr.setPrefix( temp.prefix() );
        addr.setGivenName( temp.givenName() );
        addr.setAdditionalName( temp.additionalName() );
        addr.setFamilyName( temp.familyName() );
        addr.setSuffix( temp.suffix() );
        addr.setEmails( temp.emails() );
    }

    data.write( m_converter.createVCard( addr ) );
}

Q_EXPORT_PLUGIN2( akonadi_serializer_addressee, Akonadi::SerializerPluginAddressee )

#include "akonadi_serializer_addressee.moc"
