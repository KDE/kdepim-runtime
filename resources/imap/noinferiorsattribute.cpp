/*
    Copyright (C) 2012  Christian Mollekopf <mollekopf@kolabsys.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#include "noinferiorsattribute.h"

#include <QByteArray>

#include <akonadi/attribute.h>

NoInferiorsAttribute::NoInferiorsAttribute( bool noInferiors )
: mNoInferiors( noInferiors )
{
}

void NoInferiorsAttribute::setNoInferiors( bool noInferiors )
{
    mNoInferiors = noInferiors;
}

bool NoInferiorsAttribute::noInferiors() const
{
    return mNoInferiors;
}

QByteArray NoInferiorsAttribute::type() const
{
    return "noinferiors";
}

Akonadi::Attribute* NoInferiorsAttribute::clone() const
{
    return new NoInferiorsAttribute( mNoInferiors );
}

QByteArray NoInferiorsAttribute::serialized() const
{
    return mNoInferiors ? QByteArray::number( 1 ) :  QByteArray::number( 0 );
}

void NoInferiorsAttribute::deserialize( const QByteArray &data )
{
    mNoInferiors = ( data.toInt() == 0 ) ? false : true;
}
