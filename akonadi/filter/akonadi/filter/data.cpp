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

#include "functiondescriptor.h"
#include "datamemberdescriptor.h"
#include "commanddescriptor.h"

#include <QtCore/QStringList>
#include <QtCore/QDateTime>

#include <KDateTime>
#include <KDebug>
#include <KLocale>

namespace Akonadi
{
namespace Filter 
{

Data::Data()
  : ErrorStack()
{
}

Data::~Data()
{
}

QVariant Data::getPropertyValue( const FunctionDescriptor * function, const DataMemberDescriptor * dataMember )
{
  clearErrors();

  Q_ASSERT( function->acceptableInputDataTypeMask() & dataMember->dataType() );

  switch( function->id() )
  {
    case FunctionValueOf:
      return getDataMemberValue( dataMember );
    break;
    case FunctionSizeOf:
    {
      QVariant value = getDataMemberValue( dataMember );
      switch( dataMember->dataType() )
      {
        case DataTypeNone:
          return integerToVariant( 0 );
        break;
        case DataTypeString:
        case DataTypeInteger:
        case DataTypeBoolean:
        case DataTypeAddress:
        case DataTypeDate:
          return integerToVariant( value.toString().length() );
        break;
        case DataTypeStringList:
        case DataTypeAddressList:
        {
          Integer sum = 0;
          QStringList sl = value.toStringList();
          foreach( QString s, sl )
            sum += s.length();
          return integerToVariant( sum );
        }
        break;
        default:
          Q_ASSERT_X( false, "Data::getPropertyValue", "Unhandled DataType" );
        break;
      }
    }
    break;
    case FunctionCountOf:
    {
      QVariant value = getDataMemberValue( dataMember );
      switch( dataMember->dataType() )
      {
        case DataTypeNone:
          return integerToVariant( 0 );
        break;
        case DataTypeString:
        case DataTypeInteger:
        case DataTypeBoolean:
        case DataTypeAddress:
        case DataTypeDate:
          return integerToVariant( 1 );
        break;
        case DataTypeStringList:
        case DataTypeAddressList:
          return integerToVariant( value.toStringList().count() );
        break;
        default:
          Q_ASSERT_X( false, "Data::getPropertyValue", "Unhandled DataType" );
        break;
      }
    }
    break;
    case FunctionDateIn:
    {
      QVariant value = getDataMemberValue( dataMember );
      if( value.isNull() )
        return QVariant();
      if( value.type() == QVariant::DateTime )
        return value;
      // FIXME: Implement better parsing of dates from a QString
      return QVariant( KDateTime::fromString( value.toString(), KDateTime::RFCDate ).dateTime() );
    }
    break;
    default:
      // unrecognized function: you should provide a handler for it by overriding getPropertyValue()
      kDebug() <<
          "Unrecognized function with id " << function->id() <<
          ", keyword '" << function->keyword() <<
          "' and name '" << function->name() << "'. You should provide a handler by overriding Data::getPropertyValue()";

      Q_ASSERT_X( false, "Data::getPropertyValue", "Unrecognized function: you should provide a handler for it" );
    break;
  }

  pushError( i18n( "Unrecognized function requested" ) );
  return QVariant();
}

} // namespace Filter

} // namespace Akonadi

