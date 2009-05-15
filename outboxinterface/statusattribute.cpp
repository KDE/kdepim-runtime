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

#include "statusattribute.h"

#include <QDataStream>

#include <KDebug>

using namespace Akonadi;
using namespace OutboxInterface;


StatusAttribute::StatusAttribute( Status status, const QString &msg )
  : mStatus( status )
  , mMessage( msg )
{
}

StatusAttribute::~StatusAttribute()
{
}

StatusAttribute* StatusAttribute::clone() const
{
    return new StatusAttribute( mStatus, mMessage );
}

QByteArray StatusAttribute::type() const
{
    static const QByteArray sType( "StatusAttribute" );
    return sType;
}

QByteArray StatusAttribute::serialized() const
{
  QByteArray serializedData;
  QDataStream serializer( &serializedData, QIODevice::WriteOnly );
  serializer.setVersion( QDataStream::Qt_4_5 );
  serializer << int( mStatus );
  serializer << mMessage;
  return serializedData;
}

void StatusAttribute::deserialize( const QByteArray &data )
{
  int i;
  QDataStream deserializer( data );
  // TODO: is this enough to make sure new versions won't trick us?
  deserializer.setVersion( QDataStream::Qt_4_5 );
  deserializer >> i;
  mStatus = Status( i ); // HACK
  deserializer >> mMessage;
}

StatusAttribute::Status StatusAttribute::status() const
{
  return mStatus;
}

void StatusAttribute::setStatus( Status status )
{
  mStatus = status;
}
  
QString StatusAttribute::message() const
{
  return mMessage;
}

void StatusAttribute::setMessage( const QString &msg )
{
  mMessage = msg;
}

