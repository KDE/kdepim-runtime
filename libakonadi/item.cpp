/*
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>

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

#include <QtCore/QSharedData>

#include "item.h"

using namespace Akonadi;

class Item::Private : public QSharedData
{
  public:
    Private()
      : QSharedData()
    {
    }

    Private( const Private &other )
      : QSharedData( other )
    {
      reference = other.reference;
      flags = other.flags;
      data = other.data;
      mimeType = other.mimeType;
    }

    DataReference reference;
    Item::Flags flags;
    QByteArray data;
    QString mimeType;
};

Item::Item( const DataReference & reference )
  : d( new Private ), m_payload(0)
{
  d->reference = reference;
}

Item::Item( const Item &other )
  : d( other.d ), m_payload( 0 )
{
    m_payload = other.m_payload->clone();
}

Item::~Item( )
{
    delete m_payload;
}

bool Item::isValid() const
{
  return !d->reference.isNull();
}

DataReference Item::reference() const
{
  return d->reference;
}

Item::Flags Item::flags() const
{
  return d->flags;
}

void Item::setFlag( const QByteArray & name )
{
  d->flags.insert( name );
}

void Item::unsetFlag( const QByteArray & name )
{
  d->flags.remove( name );
}

bool Item::hasFlag( const QByteArray & name ) const
{
  return d->flags.contains( name );
}

QByteArray Item::data() const
{
  return d->data;
}

void Item::setData( const QByteArray & data )
{
  d->data = data;
}

QString Item::mimeType() const
{
  return d->mimeType;
}

void Item::setMimeType( const QString & mimeType )
{
  d->mimeType = mimeType;
}

Item& Item::operator=( const Item & other )
{
  if ( this != &other ) {
    d = other.d;
    m_payload = other.m_payload->clone();
  }

  return *this;
}
