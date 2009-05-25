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

#include "errorattribute.h"

#include <QDataStream>

#include <KDebug>

using namespace Akonadi;
using namespace OutboxInterface;


ErrorAttribute::ErrorAttribute( const QString &msg )
  : mMessage( msg )
{
}

ErrorAttribute::~ErrorAttribute()
{
}

ErrorAttribute* ErrorAttribute::clone() const
{
    return new ErrorAttribute( mMessage );
}

QByteArray ErrorAttribute::type() const
{
    static const QByteArray sType( "ErrorAttribute" );
    return sType;
}

QByteArray ErrorAttribute::serialized() const
{
  return mMessage.toLocal8Bit();
}

void ErrorAttribute::deserialize( const QByteArray &data )
{
  mMessage = QString::fromLocal8Bit( data );
}

QString ErrorAttribute::message() const
{
  return mMessage;
}

void ErrorAttribute::setMessage( const QString &msg )
{
  mMessage = msg;
}

