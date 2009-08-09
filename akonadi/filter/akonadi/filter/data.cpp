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
#include "operatordescriptor.h"

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

Data::PropertyTestResult Data::performPropertyTest(
    const FunctionDescriptor * function,
    const DataMemberDescriptor * dataMember,
    const OperatorDescriptor * op,
    const QVariant &operand
  )
{
  kDebug() << "Executing property test data member: '" << dataMember->name() << "', function: '" << function->name() << "'";

  // Generic function test implementation.
  //
  // Override this method in a derived class if you want to provide
  // an optimized/custom match (then override ComponentFactory::createPropertyTestCondition() to return your subclasses).

  QVariant val = getPropertyValue( function, dataMember );

  kDebug() << "Data member value is '" << val << "'";

  if( val.isNull() )
  {
    if( hasErrors() )
    {
      pushError( i18n( "Error retrieving property value" ) );
      return PropertyTestError;
    }
  }

  bool ok;

  switch( op->rightOperandDataType() )
  {
    case DataTypeInteger:
    {
      Integer left = variantToInteger( val, &ok );
      if( !ok )
      {
        pushError( i18n( "Could not convert the left operand '%1' to integer", val.toString() ) );
        return PropertyTestError;
      }

      Integer right = variantToInteger( operand, &ok );
      if( !ok )
      {
        pushError( i18n( "Could not convert the right operand '%1' to integer", operand.toString() ) );
        return PropertyTestError;
      } 

      Q_ASSERT( op );

      switch( op->id() )
      {
        case OperatorGreaterThan:
          return left > right ? PropertyTestVerified : PropertyTestNotVerified;
        break;
        case OperatorLowerThan:
          return left < right ? PropertyTestVerified : PropertyTestNotVerified;
        break;
        case OperatorIntegerIsEqualTo:
          return left == right ? PropertyTestVerified : PropertyTestNotVerified;
        break;
        default:
          Q_ASSERT_X( false, __FUNCTION__, "You either forgot to handle an operator or to provide your own implementation of Data::performPropertyTest()" );
          pushError( i18n( "Unhandled operator" ) );
          return PropertyTestError;
        break;
      }
    }
    break;
    case DataTypeString:
    {
      QStringList leftOperands = val.toStringList();
      QString right = operand.toString();

      Q_ASSERT( op );

      foreach( QString left, leftOperands )
      {
        switch( op->id() )
        {
          case OperatorStringIsEqualTo:
            return left == right ? PropertyTestVerified : PropertyTestNotVerified;
          break;
          case OperatorContains:
            return left.contains( right ) ? PropertyTestVerified : PropertyTestNotVerified;
          break;
          case OperatorIntegerIsEqualTo:
            return left == right ? PropertyTestVerified : PropertyTestNotVerified;
          break;
          case OperatorStringMatchesRegexp:
          {
            QRegExp exp( right );
            return left.contains( exp ) ? PropertyTestVerified : PropertyTestNotVerified;
          }
          break;
          case OperatorStringMatchesWildcard:
          {
            QRegExp exp( right, Qt::CaseSensitive, QRegExp::Wildcard );
            return left.contains( exp ) ? PropertyTestVerified : PropertyTestNotVerified;
          }
          break;
          default:
            Q_ASSERT_X( false, __FUNCTION__, "You either forgot to handle an operator or to provide your own implementation of Data::performPropertyTest()" );
            pushError( i18n( "Unhandled operator" ) );
            return PropertyTestError;
          break;
        }
      }
    }
    break;
    case DataTypeDate:
    {
      QDateTime left = val.toDateTime();
      if( !left.isValid() )
      {
        pushError( i18n( "Could not convert the left operand '%1' to date/time", val.toString() ) );
        return PropertyTestError;
      }
      QDateTime right = operand.toDateTime();
      if( !right.isValid() )
      {
        pushError( i18n( "Could not convert the right operand '%1' to date/time", operand.toString() ) );
        return PropertyTestError;
      }

      Q_ASSERT( op );

      switch( op->id() )
      {
        case OperatorDateIsEqualTo:
          return left.date() == right.date() ? PropertyTestVerified : PropertyTestNotVerified;
        break;
        case OperatorDateIsAfter:
          return left.date() > right.date() ? PropertyTestVerified : PropertyTestNotVerified;
        break;
        case OperatorDateIsBefore:
          return left.date() < right.date() ? PropertyTestVerified : PropertyTestNotVerified;
        break;
        default:
          Q_ASSERT_X( false, __FUNCTION__, "You either forgot to handle an operator or to provide your own implementation of Data::performPropertyTest()" );
          pushError( i18n( "Unhandled operator" ) );
          return PropertyTestError;
        break;
      }
    }
    break;
    case DataTypeNone:
    {
      // left operand type should be boolean!
    }
    break;
    default:
      Q_ASSERT_X( false, __FUNCTION__, "You either forgot to handle an operator or to provide your own implementation of Data::performPropertyTest()" );
      pushError( i18n( "Unhandled data type" ) );
      return PropertyTestError;
    break;
  }

  Q_ASSERT_X( false, __FUNCTION__, "This point should be never reached" );
  pushError( i18n( "Internal error" ) );
  return PropertyTestError;
}

QVariant Data::getPropertyValue( const FunctionDescriptor * function, const DataMemberDescriptor * dataMember )
{
  clearErrors();

  Q_ASSERT(
      // data member has a data type acceptable for the function
      ( function->acceptableInputDataTypeMask() & dataMember->dataType() ) && 
      // data member provides the function's required feature bits
      ( ( dataMember->featureMask() & function->requiredInputFeatureMask() ) == function->requiredInputFeatureMask() )
    );

  switch( function->id() )
  {
    case FunctionValueOf:
      return getDataMemberValue( dataMember );
    break;
    case FunctionSizeOf:
    {
      QVariant value = getDataMemberValue( dataMember );
      if( value.isNull() && hasErrors() )
        return value;
      switch( dataMember->dataType() )
      {
        case DataTypeNone:
          return integerToVariant( 0 );
        break;
        case DataTypeString:
        case DataTypeInteger:
        case DataTypeBoolean:
        case DataTypeDate:
          return integerToVariant( value.toString().length() );
        break;
        case DataTypeStringList:
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
      if( value.isNull() && hasErrors() )
        return value;
      switch( dataMember->dataType() )
      {
        case DataTypeNone:
          return integerToVariant( 0 );
        break;
        case DataTypeString:
        case DataTypeInteger:
        case DataTypeBoolean:
        case DataTypeDate:
          return integerToVariant( 1 );
        break;
        case DataTypeStringList:
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

