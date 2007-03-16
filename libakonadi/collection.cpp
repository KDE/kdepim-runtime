/*
    Copyright (c) 2006 - 2007 Volker Krause <vkrause@kde.org>

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

#include "collection.h"

#include <QtCore/QHash>
#include <QtCore/QString>
#include <QtCore/QStringList>

using namespace Akonadi;

class Collection::Private : public QSharedData
{
  public:
    Private() :
      QSharedData(),
      id( -1 ),
      parentId( -1 ),
      type( Unknown )
    {}

    Private( const Private &other ) :
      QSharedData( other )
    {
      id = other.id;
      parentId = other.parentId;
      name = other.name;
      remoteId = other.remoteId;
      type = other.type;
      foreach ( CollectionAttribute* attr, other.attributes )
        attributes.insert( attr->type(), attr->clone() );
      resource = other.resource;
    }

    ~Private()
    {
      qDeleteAll( attributes );
    }

    int id;
    int parentId;
    QString name;
    QString remoteId;
    Type type;
    QHash<QByteArray, CollectionAttribute*> attributes;
    QString resource;
};

Collection::Collection() :
    d ( new Private )
{
}

Collection::Collection( int id ) :
    d( new Private )
{
  d->id = id;
}

Collection::Collection(const Collection & other) :
    d ( other.d )
{
}

Collection::~Collection()
{
}

int Akonadi::Collection::id() const
{
  return d->id;
}

QString Collection::name( ) const
{
  return d->name;
}

void Collection::setName( const QString & name )
{
  d->name = name;
}

Collection::Type Collection::type() const
{
  return d->type;
}

void Collection::setType( Type type )
{
  d->type = type;
}

QList<QByteArray> Collection::contentTypes() const
{
  CollectionContentTypeAttribute *attr = const_cast<Collection*>( this )->attribute<CollectionContentTypeAttribute>();
  if ( attr )
    return attr->contentTypes();
  return QList<QByteArray>();
}

void Collection::setContentTypes( const QList<QByteArray> & types )
{
  CollectionContentTypeAttribute* attr = attribute<CollectionContentTypeAttribute>( true );
  attr->setContentTypes( types );
}

int Collection::parent() const
{
  return d->parentId;
}

void Collection::setParent( int parent )
{
  d->parentId = parent;
}

QString Collection::delimiter()
{
  return QLatin1String( "/" );
}

Collection Collection::root( )
{
  Collection root( 0 );
  QList<QByteArray> types;
  types << collectionMimeType();
  root.setContentTypes( types );
  return root;
}

QByteArray Collection::collectionMimeType( )
{
  return QByteArray( "inode/directory" );
}

QList<CollectionAttribute*> Collection::attributes() const
{
  return d->attributes.values();
}

void Collection::addAttribute( CollectionAttribute * attr )
{
  if ( d->attributes.contains( attr->type() ) )
    delete d->attributes.take( attr->type() );
  d->attributes.insert( attr->type(), attr );
}

CollectionAttribute * Collection::attribute( const QByteArray & type ) const
{
  if ( d->attributes.contains( type ) )
    return d->attributes.value( type );
  return 0;
}

bool Collection::hasAttribute( const QByteArray & type ) const
{
  return d->attributes.contains( type );
}

bool Collection::isValid() const
{
  return id() >= 0;
}

bool Collection::operator ==(const Collection & other) const
{
  return d->id == other.id();
}

bool Collection::operator !=(const Collection & other) const
{
  return d->id != other.id();
}

QString Collection::remoteId() const
{
  return d->remoteId;
}

void Collection::setRemoteId(const QString & remoteId)
{
  d->remoteId = remoteId;
}

Collection& Collection::operator =(const Collection & other)
{
  d = other.d;
  return *this;
}

QString Collection::resource() const
{
  return d->resource;
}

void Collection::setResource(const QString & resource)
{
  d->resource = resource;
}
