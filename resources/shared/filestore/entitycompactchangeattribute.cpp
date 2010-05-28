/*  This file is part of the KDE project
    Copyright (C) 2010 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.net
    Author: Kevin Krammer, krake@kdab.com

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "entitycompactchangeattribute.h"

#include <QDataStream>

using namespace Akonadi;

class FileStore::EntityCompactChangeAttribute::Private
{
  FileStore::EntityCompactChangeAttribute *const q;

  public:
    explicit Private( FileStore::EntityCompactChangeAttribute *parent ) : q( parent )
    {
    }

    Private& operator=( const Private &other )
    {
      if ( &other == this ) {
        return *this;
      }

      mRemoteId = other.mRemoteId;
      return *this;
    }

  public:
    QString mRemoteId;
};

FileStore::EntityCompactChangeAttribute::EntityCompactChangeAttribute()
  : Attribute(), d( new Private( this ) )
{
}

FileStore::EntityCompactChangeAttribute::~EntityCompactChangeAttribute()
{
  delete d;
}

void FileStore::EntityCompactChangeAttribute::setRemoteId( const QString &remoteId )
{
  d->mRemoteId = remoteId;
}

QString FileStore::EntityCompactChangeAttribute::remoteId() const
{
  return d->mRemoteId;
}

QByteArray FileStore::EntityCompactChangeAttribute::type() const
{
  return "ENTITYCOMPACTCHANGE";
}

FileStore::EntityCompactChangeAttribute* FileStore::EntityCompactChangeAttribute::clone() const
{
  FileStore::EntityCompactChangeAttribute *copy = new FileStore::EntityCompactChangeAttribute();
  *(copy->d) = *d;
  return copy;
}

QByteArray FileStore::EntityCompactChangeAttribute::serialized() const
{
  QByteArray data;
  QDataStream stream( &data, QIODevice::WriteOnly );

  stream << d->mRemoteId;

  return data;
}

void FileStore::EntityCompactChangeAttribute::deserialize( const QByteArray &data )
{
  QDataStream stream( data );
  stream >> d->mRemoteId;
}

// kate: space-indent on; indent-width 2; replace-tabs on;
