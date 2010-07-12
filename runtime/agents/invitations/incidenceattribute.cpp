/*
    Copyright (c) 2009 Sebastian Sauer <sebsauer@kdab.com>

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

#include "incidenceattribute.h"

#include <QtCore/QString>
#include <QtCore/QTextStream>

using namespace Akonadi;

class IncidenceAttribute::Private
{
  public:
    QString status;
    Akonadi::Item::Id referenceId;

    explicit Private() : referenceId( -1 ) {}
};

IncidenceAttribute::IncidenceAttribute()
  : Attribute(), d( new Private )
{
}

IncidenceAttribute::~IncidenceAttribute()
{
  delete d;
}

QByteArray IncidenceAttribute::type() const
{
  return "incidence";
}

Attribute* IncidenceAttribute::clone() const
{
  IncidenceAttribute *other = new IncidenceAttribute;
  return other;
}
 
QByteArray IncidenceAttribute::serialized() const
{
  QString data;
  QTextStream out( &data );
  out << d->status;
  out << d->referenceId;
  return data.toUtf8();
}

void IncidenceAttribute::deserialize( const QByteArray &data )
{
  QString s( QString::fromUtf8( data ) );
  QTextStream in( &s );
  in >> d->status;
  in >> d->referenceId;
}

QString IncidenceAttribute::status() const
{
  return d->status;
}

void IncidenceAttribute::setStatus( const QString &newstatus ) const
{
  d->status = newstatus;
}

Akonadi::Item::Id IncidenceAttribute::reference() const
{
  return d->referenceId;
}

void IncidenceAttribute::setReference( Akonadi::Item::Id id )
{
  d->referenceId = id;
}
