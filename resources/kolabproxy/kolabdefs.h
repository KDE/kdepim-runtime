/*
    Copyright (c) 2010 Volker Krause <vkrause@kde.org>

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

#ifndef KOLABDEFS_H
#define KOLABDEFS_H

#include <QtCore/QByteArray>

#define KOLAB_FOLDER_TYPE_MAIL    "mail"
#define KOLAB_FOLDER_TYPE_CONTACT "contact"
#define KOLAB_FOLDER_TYPE_EVENT   "event"
#define KOLAB_FOLDER_TYPE_TASK    "task"
#define KOLAB_FOLDER_TYPE_JOURNAL "journal"
#define KOLAB_FOLDER_TYPE_NOTE    "note"

#define KOLAB_FOLDER_TYPE_DEFAULT_SUFFIX ".default"

#define KOLAB_FOLDER_TYPE_ANNOTATION "/vendor/kolab/folder-type"

namespace Kolab
{
  enum FolderType {
    Mail = 0,
    Contact,
    Event,
    Task,
    Journal,
    Note,
    FolderTypeSize = Note + 1,
  };

  FolderType folderTypeFromString( const QByteArray &folderTypeName );
  QByteArray folderTypeToString( FolderType type, bool isDefault = false );
  FolderType guessFolderTypeFromName( const QString &name );
  QString nameForFolderType( FolderType type );
}

#endif
