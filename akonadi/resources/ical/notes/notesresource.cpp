/*
    Copyright (c) 2009 David Jarvie <djarvie@kde.org>

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

#include "notesresource.h"
#include "settingsadaptor.h"

#include <kcal/incidence.h>

#include <kglobal.h>
#include <klocale.h>
#include <kconfigskeleton.h>
#include <kdebug.h>

using namespace Akonadi;
using namespace KCal;

static QLatin1String sNotesType( "application/x-vnd.kde.notes" );

NotesResource::NotesResource( const QString &id )
    : ICalResource( id, allMimeTypes(), "knotes" )
{
  KConfigSkeleton::ItemPath *item = static_cast<KConfigSkeleton::ItemPath*>( Settings::self()->findItem( "Path" ) );
  if ( item ) {
    item->setDefaultValue( KGlobal::dirs()->saveLocation( "data", "knotes/" ) + "notes.ics" );
  }
}

NotesResource::~NotesResource()
{
}

QStringList NotesResource::allMimeTypes() const
{
    return QStringList() << sNotesType;
}

QString NotesResource::mimeType( KCal::IncidenceBase * )
{
  return sNotesType;
}

AKONADI_RESOURCE_MAIN( NotesResource )

#include "notesresource.moc"
