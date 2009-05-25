/*
    Copyright 2009 Constantin Berzan <exit3219@gmail.com>

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

#include "sentcollectionattribute.h"

#include <QDataStream>

#include <KDebug>

using namespace Akonadi;
using namespace OutboxInterface;


SentCollectionAttribute::SentCollectionAttribute( Collection::Id id )
  : mId( id )
{
}

SentCollectionAttribute::~SentCollectionAttribute()
{
}

SentCollectionAttribute* SentCollectionAttribute::clone() const
{
    return new SentCollectionAttribute( mId );
}

QByteArray SentCollectionAttribute::type() const
{
    static const QByteArray sType( "SentCollectionAttribute" );
    return sType;
}

QByteArray SentCollectionAttribute::serialized() const
{
  return QByteArray::number( mId );
}

void SentCollectionAttribute::deserialize( const QByteArray &data )
{
  // NOTE: We secretly know Akonadi::Collection::Id is qlonglong.
  // Is there a way to make this type-agnostic?
  bool ok;
  mId = data.toLongLong( &ok );
  if( !ok ) {
    kWarning() << "Could not deserialize" << data;
    mId = -1;
  }
}

Collection::Id SentCollectionAttribute::sentCollection() const
{
  return mId;
}

void SentCollectionAttribute::setSentCollection( Collection::Id id )
{
  mId = id;
}

