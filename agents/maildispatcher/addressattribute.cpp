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

#include "addressattribute.h"

#include <QDataStream>

#include <KDebug>

using namespace Akonadi;


AddressAttribute::AddressAttribute( const QString &from, const QStringList &to,
                                    const QStringList &cc, const QStringList &bcc )
  : mFrom( from ), mTo( to ), mCc( cc ), mBcc( bcc )
{
}

AddressAttribute::~AddressAttribute()
{
}

AddressAttribute* AddressAttribute::clone() const
{
    return new AddressAttribute( mFrom, mTo, mCc, mBcc );
}

QByteArray AddressAttribute::type() const
{
    static const QByteArray sType( "AddressAttribute" );
    return sType;
}

QByteArray AddressAttribute::serialized() const
{
  QByteArray serializedData;
  QDataStream serializer( &serializedData, QIODevice::WriteOnly );
  serializer.setVersion( QDataStream::Qt_4_5 );
  serializer << mFrom << mTo << mCc << mBcc;
  return serializedData;
}

void AddressAttribute::deserialize( const QByteArray &data )
{
  QDataStream deserializer( data );
  // TODO: is this enough to make sure new versions won't trick us?
  deserializer.setVersion( QDataStream::Qt_4_5 );
  deserializer >> mFrom >> mTo >> mCc >> mBcc;
}

QString AddressAttribute::from() const
{
  return mFrom;
}

void AddressAttribute::setFrom( const QString &from )
{
  mFrom = from;
}

QStringList AddressAttribute::to() const
{
  return mTo;
}

void AddressAttribute::setTo( const QStringList &to )
{
  mTo = to;
}

QStringList AddressAttribute::cc() const
{
  return mCc;
}

void AddressAttribute::setCc( const QStringList &cc )
{
  mCc = cc;
}

QStringList AddressAttribute::bcc() const
{
  return mBcc;
}

void AddressAttribute::setBcc( const QStringList &bcc )
{
  mBcc = bcc;
}

