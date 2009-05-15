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

#include "transportattribute.h"

#include <KDebug>

#include "mailtransport/transportmanager.h"

using namespace Akonadi;
using namespace MailTransport;
using namespace OutboxInterface;


TransportAttribute::TransportAttribute( int id )
  : mId( id )
{
}

TransportAttribute::~TransportAttribute()
{
}

TransportAttribute* TransportAttribute::clone() const
{
    return new TransportAttribute( mId );
}

QByteArray TransportAttribute::type() const
{
    static const QByteArray sType( "TransportAttribute" );
    return sType;
}

QByteArray TransportAttribute::serialized() const
{
  return QByteArray::number( mId );
}

void TransportAttribute::deserialize( const QByteArray &data )
{
  mId = data.toInt();
}

int TransportAttribute::transportId() const
{
  return mId;
}

Transport* TransportAttribute::transport() const
{
  return TransportManager::self()->transportById( mId );
}

void TransportAttribute::setTransportId( int id )
{
  mId = id;
}

