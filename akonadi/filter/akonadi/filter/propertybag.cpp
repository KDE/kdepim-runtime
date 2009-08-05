/****************************************************************************** *
 *
 *  File : propertybag.cpp
 *  Created on Wed 05 Aug 2009 01:13:34 by Szymon Stefanek
 *
 *  This file is part of the Akonadi Filtering Framework
 *
 *  Copyright 2009 Szymon Tomasz Stefanek <s.stefanek at gmail dot com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *******************************************************************************/

#include "propertybag.h"

PropertyBag::PropertyBag()
{
}

PropertyBag::~PropertyBag()
{
}

QVariant PropertyBag::property( const QString &name ) const
{
  return mProperties.value( name, QVariant() );
}

void PropertyBag::setProperty( const QString &name, const QVariant &value )
{
  mProperties.insert( name, value );
}

void PropertyBag::setAllProperties( const QHash< QString, QVariant > &all )
{
  mProperties = all;
}
