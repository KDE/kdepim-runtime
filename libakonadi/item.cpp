/*
    Copyright (c) 2006 Volker Krause <volker.krause@rwth-aachen.de>

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

#include "item.h"

using namespace PIM;

class PIM::ItemPrivate {
  public:
    DataReference ref;
    Item::Flags flags;
};

PIM::Item::Item( const DataReference & ref ) :
    d( new ItemPrivate )
{
  d->ref = ref;
}

PIM::Item::~ Item( )
{
  delete d;
}

DataReference PIM::Item::reference( ) const
{
  return d->ref;
}

Item::Flags PIM::Item::flags( ) const
{
  return d->flags;
}

void PIM::Item::setFlag( const QByteArray & name )
{
  d->flags.insert( name );
}

void PIM::Item::unsetFlag( const QByteArray & name )
{
  d->flags.remove( name );
}

bool PIM::Item::hasFlag( const QByteArray & name ) const
{
  return d->flags.contains( name );
}


