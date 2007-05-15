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

#include <QtCore/QMap>
#include <QtCore/QSharedData>

#include "item.h"

#include <kurl.h>

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
      mimeType = other.mimeType;
      mParts = other.mParts;
    }

    DataReference reference;
    Item::Flags flags;
    QString mimeType;
    QMap<QString, QByteArray> mParts;
};

Item::Item( const DataReference & reference )
  : d( new Private ), m_payload( 0 )
{
  d->reference = reference;
}

Item::Item(const QString & mimeType) :
    d ( new Private ), m_payload( 0 )
{
  d->mimeType = mimeType;
}

Item::Item( const Item &other )
  : d( other.d ), m_payload( 0 )
{
  if ( other.m_payload )
    m_payload = other.m_payload->clone();
  else
    m_payload = 0;
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

void Item::setReference(const DataReference & ref)
{
  d->reference = ref;
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

void Item::addPart( const QString &identifier, const QByteArray &data )
{
  d->mParts.insert( identifier, data );
}

QByteArray Item::part( const QString &identifier ) const
{
  return d->mParts.value( identifier );
}

QStringList Item::availableParts() const
{
  return d->mParts.keys();
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
    if ( other.m_payload )
      m_payload = other.m_payload->clone();
    else
      m_payload = 0;
  }

  return *this;
}

bool Item::hasPayload() const
{
  return m_payload != 0;
}

KUrl Item::url() const
{
  KUrl url;
  url.setProtocol( QString::fromLatin1("akonadi") );
  url.setEncodedPathAndQuery( QString::fromLatin1("?item=")
                                + reference().id()
                                + QString::fromLatin1( "&type=" )
                                + mimeType() );
  return url;
}

DataReference Item::fromUrl( const KUrl &url )
{
  return DataReference( url.queryItems()[ QString::fromLatin1("item") ].toInt(), QString() );
}

bool Item::urlIsValid( const KUrl &url )
{
  return url.protocol() == QString::fromLatin1("akonadi") && url.queryItems().contains( QString::fromLatin1("item") );
}
