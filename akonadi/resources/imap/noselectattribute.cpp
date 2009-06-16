/*
    Copyright (C) 2008 Omat Holding B.V. <info@omat.nl>

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

#include "noselectattribute.h"

#include <QByteArray>

#include <akonadi/attribute.h>

NoSelectAttribute::NoSelectAttribute( bool noSelect )
        : mNoSelect( noSelect )
{
}

void NoSelectAttribute::setNoSelect( bool noSelect )
{
    mNoSelect = noSelect;
}

bool NoSelectAttribute::noSelect() const
{
    return mNoSelect;
}

QByteArray NoSelectAttribute::type() const
{
    return "noselect";
}

Akonadi::Attribute* NoSelectAttribute::clone() const
{
    return new NoSelectAttribute( mNoSelect );
}

QByteArray NoSelectAttribute::serialized() const
{
    return mNoSelect ? QByteArray::number( 1 ) :  QByteArray::number( 0 );
}

void NoSelectAttribute::deserialize( const QByteArray &data )
{
    mNoSelect = ( data.toInt() == 0 ) ? false : true;
}
