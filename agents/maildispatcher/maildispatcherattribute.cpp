/*
    Copyright 2008 Ingo Klöcker <kloecker@kde.org>

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

#include "maildispatcherattribute.h"

class MailDispatcherAttribute::Private
{
  public:
    int transport;
    Akonadi::Entity::Id sentMailCollection;
    QDateTime dueDate;
};

MailDispatcherAttribute::MailDispatcherAttribute()
: d( new Private )
{
}

MailDispatcherAttribute::~MailDispatcherAttribute()
{
    delete d;
}

MailDispatcherAttribute::MailDispatcherAttribute( const MailDispatcherAttribute & other )
: Attribute( other )
, d( new Private( *(other.d) ) )
{
}

MailDispatcherAttribute * MailDispatcherAttribute::clone() const
{
    return new MailDispatcherAttribute( *this );
}

QByteArray MailDispatcherAttribute::type() const
{
    static const QByteArray sType( "MailDispatcherAttribute" );
    return sType;
}

QByteArray MailDispatcherAttribute::serialized() const
{
    QByteArray serializedData;
    QDataStream serializer( &serializedData, QIODevice::WriteOnly );
    serializer << d->transport;
    serializer << d->sentMailCollection;
    serializer << d->dueDate;
    return serializedData;
}

void MailDispatcherAttribute::deserialize( const QByteArray &data )
{
    QDataStream deserializer( data );
    deserializer >> d->transport;
    deserializer >> d->sentMailCollection;
    deserializer >> d->dueDate;
}

int MailDispatcherAttribute::transport() const
{
    return d->transport;
}

void MailDispatcherAttribute::setTransport( int id )
{
    d->transport = id;
}

Akonadi::Entity::Id MailDispatcherAttribute::sentMailCollection() const
{
    return d->sentMailCollection;
}

void MailDispatcherAttribute::setSentMailCollection( Akonadi::Entity::Id id )
{
    d->sentMailCollection = id;
}

QDateTime MailDispatcherAttribute::dueDate() const
{
    return d->dueDate;
}

void MailDispatcherAttribute::setDueDate( const QDateTime &date )
{
    d->dueDate = date;
}
