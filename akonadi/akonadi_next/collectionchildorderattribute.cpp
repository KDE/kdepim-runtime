/*
Copyright (c) 2008 Stephen Kelly <steveire@gmail.com>

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

#include "collectionchildorderattribute.h"

#include <akonadi/private/imapparser_p.h>
#include <QString>
#include <QStringList>

#include <kdebug.h>

using namespace Akonadi;

class CollectionChildOrderAttribute::Private
{
public:
  QStringList orderStringList;
};

CollectionChildOrderAttribute::CollectionChildOrderAttribute() :
    d( new Private )
{
}

CollectionChildOrderAttribute::~ CollectionChildOrderAttribute()
{
  delete d;
}

QByteArray Akonadi::CollectionChildOrderAttribute::type() const
{
  return "COLLECTIONCHILDORDER";
}

CollectionChildOrderAttribute * CollectionChildOrderAttribute::clone() const
{
  CollectionChildOrderAttribute *attr = new CollectionChildOrderAttribute();
  attr->d->orderStringList = d->orderStringList;
  return attr;
}

QByteArray CollectionChildOrderAttribute::serialized() const
{
  QList<QByteArray> l;
  foreach( const QString &s, d->orderStringList ) {
    l << ImapParser::quote( s.toUtf8() );
  }

  return '(' + ImapParser::join( l, " " ) + ')';
}

void CollectionChildOrderAttribute::deserialize( const QByteArray &data )
{
  QList<QByteArray> l;
  ImapParser::parseParenthesizedList( data, l );
  QListIterator<QByteArray> i( l );
  while ( i.hasNext() ) {
    d->orderStringList << QString::fromUtf8( i.next() );
  }
}

void CollectionChildOrderAttribute::setOrderList( QStringList l )
{
  d->orderStringList = l;
}

QStringList CollectionChildOrderAttribute::orderList()
{
// kDebug() << d->orderStringList;
  return d->orderStringList;
}
