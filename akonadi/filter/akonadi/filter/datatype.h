/****************************************************************************** *
 *
 *  File : datatype.h
 *  Created on Thu 07 May 2009 13:30:16 by Szymon Tomasz Stefanek
 *
 *  This file is part of the Akonadi Filtering Framework
 *
 *  Copyright 2009 Szymon Tomasz Stefanek <pragma@kvirc.net>
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

#ifndef _AKONADI_FILTER_DATATYPE_H_
#define _AKONADI_FILTER_DATATYPE_H_

#include <akonadi/filter/config-akonadi-filter.h>

#include <QtCore/QString>
#include <QtCore/QVariant>

namespace Akonadi
{
namespace Filter
{

enum DataType
{
  DataTypeNone        = 0,
  DataTypeString      = 1,
  DataTypeInteger     = 1 << 1,
  DataTypeStringList  = 1 << 2,
  DataTypeDate        = 1 << 3,
  DataTypeBoolean     = 1 << 4,
  DataTypeAddress     = 1 << 5,
  DataTypeAddressList = 1 << 6
};

typedef qint64 Integer;

inline Integer variantToInteger( const QVariant &val, bool * ok )
{
  return val.toLongLong( ok );
}

} // namespace Filter

} // namespace Akonadi

#endif //!_AKONADI_FILTER_DATATYPE_H_
