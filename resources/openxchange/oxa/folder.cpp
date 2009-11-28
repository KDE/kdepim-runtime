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

#include "folder.h"

using namespace OXA;

Folder::Permissions::Permissions()
  : mFolderPermission( NoPermission ),
    mObjectReadPermission( NoReadPermission ),
    mObjectWritePermission( NoWritePermission ),
    mObjectDeletePermission( NoDeletePermission ),
    mAdminFlag( false )
{
}

void Folder::Permissions::setFolderPermission( FolderPermission permission )
{
  mFolderPermission = permission;
}

Folder::Permissions::FolderPermission Folder::Permissions::folderPermission() const
{
  return mFolderPermission;
}

void Folder::Permissions::setObjectReadPermission( ObjectReadPermission permission )
{
  mObjectReadPermission = permission;
}

Folder::Permissions::ObjectReadPermission Folder::Permissions::objectReadPermission() const
{
  return mObjectReadPermission;
}

void Folder::Permissions::setObjectWritePermission( ObjectWritePermission permission )
{
  mObjectWritePermission = permission;
}

Folder::Permissions::ObjectWritePermission Folder::Permissions::objectWritePermission() const
{
  return mObjectWritePermission;
}

void Folder::Permissions::setObjectDeletePermission( ObjectDeletePermission permission )
{
  mObjectDeletePermission = permission;
}

Folder::Permissions::ObjectDeletePermission Folder::Permissions::objectDeletePermission() const
{
  return mObjectDeletePermission;
}

void Folder::Permissions::setAdminFlag( bool value )
{
  mAdminFlag = value;
}

bool Folder::Permissions::adminFlag() const
{
  return mAdminFlag;
}


Folder::Folder()
{
}

void Folder::setObjectStatus( ObjectStatus status )
{
  mObjectStatus = status;
}

Folder::ObjectStatus Folder::objectStatus() const
{
  return mObjectStatus;
}

void Folder::setTitle( const QString &title )
{
  mTitle = title;
}

QString Folder::title() const
{
  return mTitle;
}

void Folder::setType( Type type )
{
  mType = type;
}

Folder::Type Folder::type() const
{
  return mType;
}

void Folder::setModule( Module module )
{
  mModule = module;
}

Folder::Module Folder::module() const
{
  return mModule;
}

void Folder::setObjectId( qlonglong id )
{
  mObjectId = id;
}

qlonglong Folder::objectId() const
{
  return mObjectId;
}

void Folder::setFolderId( qlonglong id )
{
  mFolderId = id;
}

qlonglong Folder::folderId() const
{
  return mFolderId;
}

void Folder::setIsDefaultFolder( bool value )
{
  mIsDefaultFolder = value;
}

bool Folder::isDefaultFolder() const
{
  return mIsDefaultFolder;
}

void Folder::setOwner( qlonglong id )
{
  mOwner = id;
}

qlonglong Folder::owner() const
{
  return mOwner;
}

void Folder::setLastModified( const QString &timeStamp )
{
  mLastModified = timeStamp;
}

QString Folder::lastModified() const
{
  return mLastModified;
}

void Folder::setUserPermissions( const UserPermissions &permissions )
{
  mUserPermissions = permissions;
}

Folder::UserPermissions Folder::userPermissions() const
{
  return mUserPermissions;
}

void Folder::setGroupPermissions( const GroupPermissions &permissions )
{
  mGroupPermissions = permissions;
}

Folder::GroupPermissions Folder::groupPermissions() const
{
  return mGroupPermissions;
}
