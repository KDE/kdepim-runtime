/*
    Copyright (c) 2009 Bertjan Broeksema <b.broeksema@kdemail.net>

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

#include "deleteditemsattribute.h"

DeletedItemsAttribute::DeletedItemsAttribute()
{
}

DeletedItemsAttribute::DeletedItemsAttribute(const DeletedItemsAttribute &other)
  : Akonadi::Attribute()
{
  if (&other == this)
    return;

  mDeletedItemOffsets = other.mDeletedItemOffsets;
}

DeletedItemsAttribute::~DeletedItemsAttribute()
{
}

void DeletedItemsAttribute::addDeletedItemOffset(quint64 offset)
{
  mDeletedItemOffsets.insert( offset );
}

Akonadi::Attribute *DeletedItemsAttribute::clone() const
{
  return new DeletedItemsAttribute(*this);
}

QSet<quint64> DeletedItemsAttribute::deletedItemOffsets() const
{
  return mDeletedItemOffsets;
}

void DeletedItemsAttribute::deserialize(const QByteArray &data)
{
  QList<QByteArray> offsets = data.split(',');
  mDeletedItemOffsets.clear();

  foreach(const QByteArray& offset, offsets) {
    mDeletedItemOffsets.insert(offset.toULongLong());
  }
}

QByteArray DeletedItemsAttribute::serialized() const
{
  QByteArray serialized;

  foreach(quint64 offset, mDeletedItemOffsets) {
    serialized += QByteArray::number(offset);
    serialized += ',';
  }

  serialized.chop(1); // Remove the last ','

  return serialized;
}

QByteArray DeletedItemsAttribute::type() const
{
  return "DeletedMboxItems";
}
