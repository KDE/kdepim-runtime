/****************************************************************************** *
 *
 *  File : data.cpp
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

#include "data.h"

#include "function.h"
#include "datamember.h"

namespace Akonadi
{
namespace Filter 
{

Data::Data()
{
}

Data::~Data()
{
}

QVariant Data::getPropertyValue( const Function * function, const DataMember * dataMember )
{
#if 0
  Q_ASSERT( function->outputDataType() == DataTypeString );
  Q_ASSERT( function->acceptableInputDataTypeMask() & dataMember->dataType() );

  switch( function->id() )
  {
    case StandardFunctionValueOf:
      Q_ASSERT( dataMember->dataType() == DataTypeString );
      if( !getDataMemberValue( dataMember, buffer ) )
      {
        // the value of a member that doesn't exist is an empty string
        buffer = QString();
      }
    break;
    case StandardFunctionSizeOf:
    case StandardFunctionCountOf:
    case StandardFunctionExists:
    case StandardFunctionDateIn:
      Q_ASSERT( false ); // function data type mismatch
    break;
    case StandardFunctionAnyAddressIn:
    break;
    case StandardFunctionAnyAddressDomainIn:
    break;
    case StandardFunctionAnyAddressLocalPartIn:
    break;
    default:
      // unrecognized function: you should provide a handler for it by overriding getPropertyValue()
      kDebug() <<
          "Unrecognized function with id " << function->id() <<
          ", keyword '" << function->keyword() <<
          "' and name '" << function->name() << "'. You should provide a handler by overriding Data::getPropertyValue()";

      Q_ASSERT( false );
    break;
  }
#endif
  return QVariant();
}

QVariant Data::getDataMemberValue( const DataMember * dataMember )
{
  return QVariant();
}


} // namespace Filter

} // namespace Akonadi

