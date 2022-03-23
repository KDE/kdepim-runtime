/*
    This file is part of oxaccess.

    SPDX-FileCopyrightText: 2009 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QMap>
#include <QString>
#include <QVector>

namespace OXA
{
/**
 * @short A class that contains information about folders on the OX server.
 *
 * @author Tobias Koenig <tokoe@kde.org>
 */
class Folder
{
public:
    /**
     * Describes a list of folders.
     */
    using List = QVector<Folder>;

    /**
     * Describes the status of the folder.
     */
    enum ObjectStatus {
        Created, ///< The folder has been created or modified.
        Deleted ///< The folder has been deleted.
    };

    /**
     * Describes the visibility type of the folder.
     */
    enum Type {
        Public, ///< The folder is visible for all users.
        Private ///< The folder is only visible for the owner.
    };

    /**
     * Describes the module the folder belongs to.
     */
    enum Module {
        Unbound, ///< The folder is only a structural folder.
        Calendar, ///< The folder contains events.
        Contacts, ///< The folder contains contacts.
        Tasks ///< The folder contains tasks.
    };

    /**
     * Describes the permissions a user or group can have on
     * a folder.
     */
    class Permissions
    {
    public:
        /**
         * Describes the permissions on folder objects.
         */
        enum FolderPermission {
            NoPermission = 0, ///< No permissions.
            FolderIsVisible = 2, ///< The folder can be read.
            CreateObjects = 4, ///< Objects can be created in the folder.
            CreateSubfolders = 8, ///< Subfolders can be created in the folder.
            AdminPermission = 128 ///< Permissions can be changed.
        };

        /**
         * Describes the read permissions on other objects.
         */
        enum ObjectReadPermission {
            NoReadPermission = 0, ///< The objects can not be read.
            ReadOwnObjects = 2, ///< Only own objects can be read.
            ReadAllObjects = 4, ///< All objects can be read.
            AdminReadPermission = 128
        };

        /**
         * Describes the write permissions on other objects.
         */
        enum ObjectWritePermission {
            NoWritePermission = 0, ///< The objects can not be written.
            WriteOwnObjects = 2, ///< Only own objects can be written.
            WriteAllObjects = 4, ///< All objects can be written.
            AdminWritePermission = 128
        };

        /**
         * Describes the delete permissions on other objects.
         */
        enum ObjectDeletePermission {
            NoDeletePermission = 0, ///< The objects can not be deleted.
            DeleteOwnObjects = 2, ///< Only own objects can be deleted.
            DeleteAllObjects = 4, ///< All objects can be deleted.
            AdminDeletePermission = 128
        };

        Permissions();

        void setFolderPermission(FolderPermission permission);
        FolderPermission folderPermission() const;

        void setObjectReadPermission(ObjectReadPermission permission);
        ObjectReadPermission objectReadPermission() const;

        void setObjectWritePermission(ObjectWritePermission permission);
        ObjectWritePermission objectWritePermission() const;

        void setObjectDeletePermission(ObjectDeletePermission permission);
        ObjectDeletePermission objectDeletePermission() const;

        void setAdminFlag(bool value);
        bool adminFlag() const;

    private:
        FolderPermission mFolderPermission;
        ObjectReadPermission mObjectReadPermission;
        ObjectWritePermission mObjectWritePermission;
        ObjectDeletePermission mObjectDeletePermission;
        bool mAdminFlag = false;
    };

    using UserPermissions = QMap<qlonglong, Permissions>;
    using GroupPermissions = QMap<qlonglong, Permissions>;

    Folder();

    void setObjectStatus(ObjectStatus status);
    ObjectStatus objectStatus() const;

    void setTitle(const QString &title);
    QString title() const;

    void setType(Type type);
    Type type() const;

    void setModule(Module module);
    Module module() const;

    void setObjectId(qlonglong id);
    qlonglong objectId() const;

    void setFolderId(qlonglong id);
    qlonglong folderId() const;

    void setIsDefaultFolder(bool value);
    bool isDefaultFolder() const;

    void setOwner(qlonglong id);
    qlonglong owner() const;

    void setLastModified(const QString &timeStamp);
    QString lastModified() const;

    void setUserPermissions(const UserPermissions &permissions);
    UserPermissions userPermissions() const;

    void setGroupPermissions(const GroupPermissions &permissions);
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
Q_DECLARE_TYPEINFO(OXA::Folder, Q_MOVABLE_TYPE);
