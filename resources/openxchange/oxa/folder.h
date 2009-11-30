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

#ifndef OXA_FOLDER_H
#define OXA_FOLDER_H

#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QString>

namespace OXA {

class Folder
{
  public:
    typedef QList<Folder> List;

    enum ObjectStatus
    {
      Created,
      Deleted
    };

    enum Type
    {
      Public,
      Private
    };

    enum Module
    {
      Unbound,
      Calendar,
      Contacts,
      Tasks
    };

    class Permissions
    {
      public:
        enum FolderPermission
        {
          NoPermission = 0,
          FolderIsVisible = 2,
          CreateObjects = 4,
          CreateSubfolders = 8,
          AdminPermission = 128
        };

        enum ObjectReadPermission
        {
          NoReadPermission = 0,
          ReadOwnObjects = 2,
          ReadAllObjects = 4,
          AdminReadPermission = 128
        };

        enum ObjectWritePermission
        {
          NoWritePermission = 0,
          WriteOwnObjects = 2,
          WriteAllObjects = 4,
          AdminWritePermission = 128
        };

        enum ObjectDeletePermission
        {
          NoDeletePermission = 0,
          DeleteOwnObjects = 2,
          DeleteAllObjects = 4,
          AdminDeletePermission = 128
        };

        Permissions();

        void setFolderPermission( FolderPermission permission );
        FolderPermission folderPermission() const;

        void setObjectReadPermission( ObjectReadPermission permission );
        ObjectReadPermission objectReadPermission() const;

        void setObjectWritePermission( ObjectWritePermission permission );
        ObjectWritePermission objectWritePermission() const;

        void setObjectDeletePermission( ObjectDeletePermission permission );
        ObjectDeletePermission objectDeletePermission() const;

        void setAdminFlag( bool value );
        bool adminFlag() const;

      private:
        FolderPermission mFolderPermission;
        ObjectReadPermission mObjectReadPermission;
        ObjectWritePermission mObjectWritePermission;
        ObjectDeletePermission mObjectDeletePermission;
        bool mAdminFlag;
    };

    typedef QMap<qlonglong, Permissions> UserPermissions;
    typedef QMap<qlonglong, Permissions> GroupPermissions;

    Folder();

    void setObjectStatus( ObjectStatus status );
    ObjectStatus objectStatus() const;

    void setTitle( const QString &title );
    QString title() const;

    void setType( Type type );
    Type type() const;

    void setModule( Module module );
    Module module() const;

    void setObjectId( qlonglong id );
    qlonglong objectId() const;

    void setFolderId( qlonglong id );
    qlonglong folderId() const;

    void setIsDefaultFolder( bool value );
    bool isDefaultFolder() const;

    void setOwner( qlonglong id );
    qlonglong owner() const;

    void setLastModified( const QString &timeStamp );
    QString lastModified() const;

    void setUserPermissions( const UserPermissions &permissions );
    UserPermissions userPermissions() const;

    void setGroupPermissions( const GroupPermissions &permissions );
    GroupPermissions groupPermissions() const;

  private:
    ObjectStatus mObjectStatus;
    QString mTitle;
    Type mType;
    Module mModule;
    qlonglong mObjectId;
    qlonglong mFolderId;
    bool mIsDefaultFolder;
    qlonglong mOwner;
    QString mLastModified;
    UserPermissions mUserPermissions;
    GroupPermissions mGroupPermissions;
};

}

#endif
