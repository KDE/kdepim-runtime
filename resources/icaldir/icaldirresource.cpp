/*
    Copyright (c) 2008 Tobias Koenig <tokoe@kde.org>
    Copyright (c) 2008 Bertjan Broeksema <broeksema@kde.org>
    Copyright (c) 2012 Sérgio Martins <iamsergio@gmail.com>
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

#include "icaldirresource.h"

#include <Akonadi/Collection>
#include <Akonadi/EntityDisplayAttribute>

#include <KCalCore/MemoryCalendar>
#include <KCalCore/FileStorage>
#include <KCalCore/ICalFormat>

#include <QtCore/QFileInfo>

using namespace Akonadi;
using namespace KCalCore;

ICalDirResource::ICalDirResource( const QString &id )
  : DirResource< KCalCore::Incidence::Ptr >( id )
{
}

ICalDirResource::~ICalDirResource()
{
}

QString ICalDirResource::payloadId( const Incidence::Ptr &payload ) const
{
    return payload->instanceIdentifier();
}

QString ICalDirResource::mimeType() const
{
    //mimetypes << KCalCore::Event::eventMimeType() << KCalCore::Todo::todoMimeType() << KCalCore::Journal::journalMimeType() << "text/calendar";
    return QLatin1String( "text/calendar" );
}

Incidence::Ptr ICalDirResource::readFromFile( const QString &filePath ) const
{
    QFileInfo fInfo( filePath );

    MemoryCalendar::Ptr calendar = MemoryCalendar::Ptr( new MemoryCalendar( QLatin1String( "UTC" ) ) );
    FileStorage::Ptr fileStorage = FileStorage::Ptr( new FileStorage( calendar, filePath, new ICalFormat() ) );

    Incidence::Ptr incidence;
    if ( fileStorage->load() ) {
        Incidence::List incidences = calendar->incidences();
        if ( incidences.count() == 1 && incidences.first()->instanceIdentifier() == fInfo.fileName() ) {
            incidence = incidences.first();
        }
    }

    return incidence;
}

bool ICalDirResource::writeToFile( const Incidence::Ptr &incidence ) const
{
    if ( !incidence ) {
        kError() << "incidence is 0!";
        return false;
    }

    const QString filePath = directoryFileName( incidence->instanceIdentifier() );
    MemoryCalendar::Ptr calendar = MemoryCalendar::Ptr( new MemoryCalendar( QLatin1String( "UTC" ) ) );
    FileStorage::Ptr fileStorage = FileStorage::Ptr( new FileStorage( calendar, filePath, new ICalFormat() ) );
    calendar->addIncidence( incidence );
    Q_ASSERT( calendar->incidences().count() == 1 );

    return fileStorage->save();
}

void ICalDirResource::retrieveCollections()
{
    Collection c = createCollection();

    QStringList mimetypes;
    mimetypes << KCalCore::Event::eventMimeType() << KCalCore::Todo::todoMimeType() << KCalCore::Journal::journalMimeType() << "text/calendar";
    c.setContentMimeTypes( mimetypes );

    EntityDisplayAttribute* attr = c.attribute<EntityDisplayAttribute>( Collection::AddIfMissing );
    attr->setDisplayName( i18n( "Calendar Folder" ) );
    attr->setIconName( "office-calendar" );

    collectionsRetrieved( Collection::List() << c );
}

AKONADI_RESOURCE_MAIN( ICalDirResource )
