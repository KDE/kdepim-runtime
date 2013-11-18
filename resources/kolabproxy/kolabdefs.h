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

#ifndef KOLABPROXY_KOLABDEFS_H
#define KOLABPROXY_KOLABDEFS_H

#include <QByteArray>
#include <QMap>

#include <kolabdefinitions.h> //libkolab
#include <formathelpers.h> //libkolab

namespace Kolab {
  FolderType folderTypeFromString( const QByteArray &folderTypeName );
  QByteArray getFolderTypeAnnotation( const QMap<QByteArray, QByteArray> &annotations);
  void setFolderTypeAnnotation( QMap<QByteArray, QByteArray> &annotations, const QByteArray &value);
}

namespace KolabV2 {

#define PRODUCT_ID QLatin1String("Akonadi-KolabResource")

  enum FolderType {
    Mail = 0,
    Contact,
    Event,
    Task,
    Journal,
    Note,
    FolderTypeSize = Note + 1
  };

  FolderType folderTypeFromString( const QByteArray &folderTypeName );
  QByteArray folderTypeToString( FolderType type, bool isDefault = false );
  FolderType guessFolderTypeFromName( const QString &name );
  QString nameForFolderType( FolderType type );
}

#endif
