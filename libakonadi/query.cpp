/*
    This file is part of libakonadi.

    Copyright (c) 2006 Tobias Koenig <tokoe@kde.org>
                  2006 Marc Mutz <mutz@kde.org>

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

#include "query.h"

using namespace PIM;

class Query::QueryPrivate
{
  public:
    QString mPattern;
};


Query::Query()
  : d( new QueryPrivate )
{
}

Query::~Query()
{
  delete d;
  d = 0;
}

void Query::setQueryPattern( const QString &pattern )
{
  doSetQueryPattern( pattern );
}

QList<PIM::DataReference> Query::result() const
{
  // TODO: return real data
  return QList<PIM::DataReference>();
}

void Query::doSetQueryPattern( const QString &pattern )
{
  d->mPattern = pattern;
}

void Query::doStart()
{
  // TODO: start communication with pim storage service here
}

#include "query.moc"
