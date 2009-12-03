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

#include "folderutils.h"

#include "davutils.h"
#include "folder.h"
#include "oxutils.h"

#include <QtXml/QDomElement>

using namespace OXA;

static void createFolderPermissions( const Folder &folder, QDomDocument &document, QDomElement &permissions )
{
  {
    const Folder::UserPermissions userPermissions = folder.userPermissions();
    Folder::UserPermissions::ConstIterator it = userPermissions.constBegin();
    while ( it != userPermissions.constEnd() ) {
      QDomElement user = DAVUtils::addOxElement( document, permissions, QLatin1String( "user" ),
                                                 OXUtils::writeNumber( it.key() ) );
      DAVUtils::setOxAttribute( user, QLatin1String( "folderpermission" ),
                                OXUtils::writeNumber( it.value().folderPermission() ) );
      DAVUtils::setOxAttribute( user, QLatin1String( "objectreadpermission" ),
                                OXUtils::writeNumber( it.value().objectReadPermission() ) );
      DAVUtils::setOxAttribute( user, QLatin1String( "objectwritepermission" ),
                                OXUtils::writeNumber( it.value().objectWritePermission() ) );
      DAVUtils::setOxAttribute( user, QLatin1String( "objectdeletepermission" ),
                                OXUtils::writeNumber( it.value().objectDeletePermission() ) );
      DAVUtils::setOxAttribute( user, QLatin1String( "admin_flag" ),
                                OXUtils::writeBoolean( it.value().adminFlag() ) );

      ++it;
    }
  }

  {
    const Folder::GroupPermissions groupPermissions = folder.groupPermissions();
    Folder::GroupPermissions::ConstIterator it = groupPermissions.constBegin();
    while ( it != groupPermissions.constEnd() ) {
      QDomElement group = DAVUtils::addOxElement( document, permissions, QLatin1String( "group" ),
                                                  OXUtils::writeNumber( it.key() ) );
      DAVUtils::setOxAttribute( group, QLatin1String( "folderpermission" ),
                                OXUtils::writeNumber( it.value().folderPermission() ) );
      DAVUtils::setOxAttribute( group, QLatin1String( "objectreadpermission" ),
                                OXUtils::writeNumber( it.value().objectReadPermission() ) );
      DAVUtils::setOxAttribute( group, QLatin1String( "objectwritepermission" ),
                                OXUtils::writeNumber( it.value().objectWritePermission() ) );
      DAVUtils::setOxAttribute( group, QLatin1String( "objectdeletepermission" ),
                                OXUtils::writeNumber( it.value().objectDeletePermission() ) );
      DAVUtils::setOxAttribute( group, QLatin1String( "admin_flag" ),
                                OXUtils::writeBoolean( it.value().adminFlag() ) );

      ++it;
    }
  }
}

static void parseFolderPermissions( const QDomElement &permissions, Folder &folder )
{
  Folder::UserPermissions userPermissions;
  Folder::GroupPermissions groupPermissions;

  QDomElement element = permissions.firstChildElement();
  while ( !element.isNull() ) {
    if ( element.tagName() == QLatin1String( "user" ) ) {
      Folder::Permissions permissions;
      permissions.setFolderPermission( (Folder::Permissions::FolderPermission)OXUtils::readNumber( element.attribute( QLatin1String( "folderpermission" ), QLatin1String( "0" ) ) ) );
      permissions.setObjectReadPermission( (Folder::Permissions::ObjectReadPermission)OXUtils::readNumber( element.attribute( QLatin1String( "objectreadpermission" ), QLatin1String( "0" ) ) ) );
      permissions.setObjectWritePermission( (Folder::Permissions::ObjectWritePermission)OXUtils::readNumber( element.attribute( QLatin1String( "objectwritepermission" ), QLatin1String( "0" ) ) ) );
      permissions.setObjectDeletePermission( (Folder::Permissions::ObjectDeletePermission)OXUtils::readNumber( element.attribute( QLatin1String( "objectdeletepermission" ), QLatin1String( "0" ) ) ) );
      permissions.setAdminFlag( OXUtils::readBoolean( element.attribute( QLatin1String( "admin_flag" ), QLatin1String( "false" ) ) ) );

      userPermissions.insert( OXUtils::readNumber( element.text() ), permissions );
    } else if ( element.tagName() == QLatin1String( "group" ) ) {
      Folder::Permissions permissions;
      permissions.setFolderPermission( (Folder::Permissions::FolderPermission)OXUtils::readNumber( element.attribute( QLatin1String( "folderpermission" ), QLatin1String( "0" ) ) ) );
      permissions.setObjectReadPermission( (Folder::Permissions::ObjectReadPermission)OXUtils::readNumber( element.attribute( QLatin1String( "objectreadpermission" ), QLatin1String( "0" ) ) ) );
      permissions.setObjectWritePermission( (Folder::Permissions::ObjectWritePermission)OXUtils::readNumber( element.attribute( QLatin1String( "objectwritepermission" ), QLatin1String( "0" ) ) ) );
      permissions.setObjectDeletePermission( (Folder::Permissions::ObjectDeletePermission)OXUtils::readNumber( element.attribute( QLatin1String( "objectdeletepermission" ), QLatin1String( "0" ) ) ) );
      permissions.setAdminFlag( OXUtils::readBoolean( element.attribute( QLatin1String( "admin_flag" ), QLatin1String( "false" ) ) ) );

      groupPermissions.insert( OXUtils::readNumber( element.text() ), permissions );
    }

    element = element.nextSiblingElement();
  }

  folder.setUserPermissions( userPermissions );
  folder.setGroupPermissions( groupPermissions );
}

Folder OXA::FolderUtils::parseFolder( const QDomElement &propElement )
{
  Folder folder;

  QDomElement element = propElement.firstChildElement();
  while ( !element.isNull() ) {
    if ( element.tagName() == QLatin1String( "object_status" ) ) {
      const QString content = OXUtils::readString( element.text() );
      if ( content == QLatin1String( "CREATE" ) )
        folder.setObjectStatus( Folder::Created );
      else if ( content == QLatin1String( "DELETE" ) )
        folder.setObjectStatus( Folder::Deleted );
      else
        Q_ASSERT( false );
    } else if ( element.tagName() == QLatin1String( "title" ) ) {
      folder.setTitle( OXUtils::readString( element.text() ) );
    } else if ( element.tagName() == QLatin1String( "owner" ) ) {
      folder.setOwner( OXUtils::readNumber( element.text() ) );
    } else if ( element.tagName() == QLatin1String( "module" ) ) {
      const QString content = OXUtils::readString( element.text() );
      if ( content == QLatin1String( "calendar" ) )
        folder.setModule( Folder::Calendar );
      else if ( content == QLatin1String( "contact" ) )
        folder.setModule( Folder::Contacts );
      else if ( content == QLatin1String( "task" ) )
        folder.setModule( Folder::Tasks );
      else
        folder.setModule( Folder::Unbound );
    } else if ( element.tagName() == QLatin1String( "type" ) ) {
      const QString content = OXUtils::readString( element.text() );
      if ( content == QLatin1String( "public" ) )
        folder.setType( Folder::Public );
      else if ( content == QLatin1String( "private" ) )
        folder.setType( Folder::Private );
      else
        Q_ASSERT( false );
    } else if ( element.tagName() == QLatin1String( "defaultfolder" ) ) {
      folder.setIsDefaultFolder( OXUtils::readBoolean( element.text() ) );
    } else if ( element.tagName() == QLatin1String( "last_modified" ) ) {
      folder.setLastModified( OXUtils::readString( element.text() ) );
    } else if ( element.tagName() == QLatin1String( "object_id" ) ) {
      folder.setObjectId( OXUtils::readNumber( element.text() ) );
    } else if ( element.tagName() == QLatin1String( "folder_id" ) ) {
      folder.setFolderId( OXUtils::readNumber( element.text() ) );
    } else if ( element.tagName() == QLatin1String( "permissions" ) ) {
      parseFolderPermissions( element, folder );
    }

    element = element.nextSiblingElement();
  }

  return folder;
}

void OXA::FolderUtils::addFolderElements( QDomDocument &document, QDomElement &propElement, const Folder &folder )
{
  DAVUtils::addOxElement( document, propElement, QLatin1String( "title" ), OXUtils::writeString( folder.title() ) );
  DAVUtils::addOxElement( document, propElement, QLatin1String( "folder_id" ), OXUtils::writeNumber( folder.folderId() ) );

  const QString type = (folder.type() == Folder::Public ? QLatin1String( "public" ) : QLatin1String( "private" ));
  DAVUtils::addOxElement( document, propElement, QLatin1String( "type" ), OXUtils::writeString( type ) );

  QString module;
  switch ( folder.module() ) {
    case Folder::Calendar: module = QLatin1String( "calendar" ); break;
    case Folder::Contacts: module = QLatin1String( "contact" ); break;
    case Folder::Tasks: module = QLatin1String( "task" ); break;
    default: break;
  }
  DAVUtils::addOxElement( document, propElement, QLatin1String( "module" ), OXUtils::writeString( module ) );

  QDomElement permissions = DAVUtils::addOxElement( document, propElement, QLatin1String( "permissions" ) );
  createFolderPermissions( folder, document, permissions );
}

