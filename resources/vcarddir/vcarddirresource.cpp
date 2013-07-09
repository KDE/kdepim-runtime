/*
    Copyright (c) 2008 Tobias Koenig <tokoe@kde.org>
    Copyright (c) 2008 Bertjan Broeksema <broeksema@kde.org>
    Copyright (c) 2013 Daniel Vrátil <dvratil@redhat.com>

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

#include "vcarddirresource.h"
#include "../shared/dirsettingsdialog.h"

#include <Akonadi/Collection>
#include <Akonadi/EntityDisplayAttribute>

using namespace Akonadi;

VCardDirResource::VCardDirResource( const QString &id )
  : DirResource< KABC::Addressee >( id )
{
}

VCardDirResource::~VCardDirResource()
{
}

QString VCardDirResource::payloadId( const KABC::Addressee &payload ) const
{
    return payload.uid();
}

QString VCardDirResource::mimeType() const
{
    return KABC::Addressee::mimeType();
}

KABC::Addressee VCardDirResource::readFromFile( const QString &filePath ) const
{
    QFile file( filePath );
    if ( file.open( QIODevice::ReadOnly ) ) {
        const QByteArray data = file.readAll();
        file.close();

        return mConverter.parseVCard( data );
    }

    return KABC::Addressee();
}

bool VCardDirResource::writeToFile( const KABC::Addressee &payload ) const
{
    const QByteArray data = mConverter.createVCard( payload );

    QFile file( directoryFileName( payload.uid() ) );
    if ( file.open( QIODevice::WriteOnly ) ) {
        file.write( data );
        file.close();

        return true;
    }

    return false;
}

void VCardDirResource::retrieveCollections()
{
    Collection c = createCollection();

    EntityDisplayAttribute* attr = c.attribute<EntityDisplayAttribute>( Collection::AddIfMissing );
    attr->setDisplayName( i18n( "Contacts Folder" ) );
    attr->setIconName( "x-office-address-book" );

    collectionsRetrieved( Collection::List() << c );
}

AKONADI_RESOURCE_MAIN( VCardDirResource )
