/*
    This file is part of oxaccess.

    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

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

#include "objectutils.h"

#include "contactutils.h"
#include "incidenceutils.h"
#include "davutils.h"
#include "oxutils.h"

#include <QtCore/QBuffer>
#include <QtXml/QDomElement>

using namespace OXA;

Object OXA::ObjectUtils::parseObject( const QDomElement &propElement, Folder::Module module )
{
  Object object;

  QDomElement element = propElement.firstChildElement();
  while ( !element.isNull() ) {
    if ( element.tagName() == QLatin1String( "last_modified" ) ) {
      object.setLastModified( OXUtils::readString( element.text() ) );
    } else if ( element.tagName() == QLatin1String( "object_id" ) ) {
      object.setObjectId( OXUtils::readNumber( element.text() ) );
    } else if ( element.tagName() == QLatin1String( "folder_id" ) ) {
      object.setFolderId( OXUtils::readNumber( element.text() ) );
    }

    element = element.nextSiblingElement();
  }

  switch ( module ) {
    case Folder::Contacts: ContactUtils::parseContact( propElement, object ); break;
    case Folder::Calendar: IncidenceUtils::parseEvent( propElement, object ); break;
    case Folder::Tasks: IncidenceUtils::parseTask( propElement, object ); break;
    case Folder::Unbound: Q_ASSERT( false ); break;
  }

  return object;
}

void OXA::ObjectUtils::addObjectElements( QDomDocument &document, QDomElement &propElement, const Object &object, void *preloadedData )
{
  if ( object.objectId() != -1 )
    DAVUtils::addOxElement( document, propElement, QLatin1String( "object_id" ), OXUtils::writeNumber( object.objectId() ) );
  if ( object.folderId() != -1 )
    DAVUtils::addOxElement( document, propElement, QLatin1String( "folder_id" ), OXUtils::writeNumber( object.folderId() ) );
  if ( !object.lastModified().isEmpty() )
    DAVUtils::addOxElement( document, propElement, QLatin1String( "last_modified" ), OXUtils::writeString( object.lastModified() ) );

  switch ( object.module() ) {
    case Folder::Contacts: ContactUtils::addContactElements( document, propElement, object, preloadedData ); break;
    case Folder::Calendar: IncidenceUtils::addEventElements( document, propElement, object ); break;
    case Folder::Tasks: IncidenceUtils::addTaskElements( document, propElement, object ); break;
    case Folder::Unbound: Q_ASSERT( false ); break;
  }
}

bool OXA::ObjectUtils::needsPreloading( const Object &object )
{
  if ( object.module() == Folder::Contacts ) {
    if ( object.contactGroup().contactReferenceCount() != 0 ) // we have to resolve these entries first
      return true;
  }

  return false;
}

KJob* OXA::ObjectUtils::preloadJob( const Object &object )
{
  if ( object.module() == Folder::Contacts ) {
    if ( object.contactGroup().contactReferenceCount() != 0 ) {
      return ContactUtils::preloadJob( object );
    }
  }

  return 0;
}

void* OXA::ObjectUtils::preloadData( const Object &object, KJob *job )
{
  if ( object.module() == Folder::Contacts ) {
    if ( object.contactGroup().contactReferenceCount() != 0 ) {
      return ContactUtils::preloadData( object, job );
    }
  }

  return 0;
}

QString OXA::ObjectUtils::davPath( Folder::Module module )
{
  switch ( module ) {
    case Folder::Contacts: return QLatin1String( "/servlet/webdav.contacts" ); break;
    case Folder::Calendar: return QLatin1String( "/servlet/webdav.calendar" ); break;
    case Folder::Tasks: return QLatin1String( "/servlet/webdav.tasks" ); break;
    case Folder::Unbound: Q_ASSERT( false ); return QString(); break;
  }

  return QString();
}
