/****************************************************************************** *
 *
 *  File : condition.cpp
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

#include <akonadi/filter/condition.h>
#include <akonadi/filter/data.h>
#include <akonadi/filter/datamemberdescriptor.h>
#include <akonadi/filter/functiondescriptor.h>
#include <akonadi/filter/operatordescriptor.h>

#include <KDebug>
#include <KLocale>

#include <QtCore/QRegExp>
#include <QtCore/QDateTime>

namespace Akonadi
{
namespace Filter 
{
namespace Condition
{

Base::Base( ConditionType type, Component * parent )
  : Component( parent ), mConditionType( type )
{
  Q_ASSERT( parent );
}

Base::~Base()
{
}

bool Base::isCondition() const
{
  return true;
}


bool Base::matches( Data * data )
{
  return false;
}

void Base::dump( const QString &prefix )
{
  debugOutput( prefix, "Condition::Base" );
}

Multi::Multi( ConditionType type, Component * parent )
  : Base( type, parent )
{
}

Multi::~Multi()
{
  qDeleteAll( mChildConditions );
}

void Multi::dumpChildConditions( const QString &prefix )
{
  QString localPrefix = prefix;
  localPrefix += QLatin1String("  ");
  foreach( Condition::Base * condition, mChildConditions )
    condition->dump( localPrefix );
}

void Multi::dump( const QString &prefix )
{
  debugOutput( prefix, "Condition::Multi" );
  dumpChildConditions( prefix );
}

Not::Not( Component * parent )
  : Base( ConditionTypeNot, parent ), mChildCondition( 0 )
{
}

Not::~Not()
{
}

bool Not::matches( Data * data )
{
  if( !mChildCondition )
    return false;
  return !mChildCondition->matches( data );
}

void Not::dump( const QString &prefix )
{
  debugOutput( prefix, "Condition::Not" );
  if( mChildCondition )
  {
    QString localPrefix = prefix;
    localPrefix += QLatin1String( "  " );
    mChildCondition->dump( localPrefix );
  } else {
    debugOutput( prefix, "  No Child Condition (So The Not is Always False)" );
  }
}


And::And( Component * parent )
  : Multi( ConditionTypeAnd, parent )
{
}

And::~And()
{
}

bool And::matches( Data * data )
{
  return false;
}

void And::dump( const QString &prefix )
{
  debugOutput( prefix, "Condition::And" );
  dumpChildConditions( prefix );
}


Or::Or( Component * parent )
  : Multi( ConditionTypeOr, parent )
{
}

Or::~Or()
{
}

bool Or::matches( Data * data )
{
  return false;
}

void Or::dump( const QString &prefix )
{
  debugOutput( prefix, "Condition::Or" );
  dumpChildConditions( prefix );
}


True::True( Component * parent )
  : Base( ConditionTypeTrue, parent )
{
}

True::~True()
{
}

bool True::matches( Data * )
{
  // this always matches
  return true;
}

void True::dump( const QString &prefix )
{
  debugOutput( prefix, "Condition::True" );
}


False::False( Component * parent )
  : Base( ConditionTypeFalse, parent )
{
}

False::~False()
{
}

bool False::matches( Data * )
{
  // this never matches
  return false;
}

void False::dump( const QString &prefix )
{
  debugOutput( prefix, "Condition::False" );
}

PropertyTest::PropertyTest( Component * parent, const FunctionDescriptor * function, const DataMemberDescriptor * dataMember, const OperatorDescriptor * op, const QVariant &operand )
  : Base( ConditionTypePropertyTest, parent ), mFunctionDescriptor( function ), mDataMemberDescriptor( dataMember ), mOperatorDescriptor( op ), mOperand( operand )
{
  Q_ASSERT( mFunctionDescriptor );
  Q_ASSERT( mDataMemberDescriptor );

  if( mOperatorDescriptor )
  {
    Q_ASSERT( ( mFunctionDescriptor->outputDataTypeMask() & mOperatorDescriptor->acceptableLeftOperandDataTypeMask() ) == mFunctionDescriptor->outputDataTypeMask() );
  }
}

PropertyTest::~PropertyTest()
{
}

bool PropertyTest::matches( Data * data )
{
  // Generic function test implementation.
  //
  // Override this method in a derived class if you want to provide
  // an optimized/custom match (then override ComponentFactory::createPropertyTestCondition() to return your subclasses).

  QVariant val = data->getPropertyValue( mFunctionDescriptor, mDataMemberDescriptor );

  bool ok;

  switch( mOperatorDescriptor->rightOperandDataType() )
  {
    case DataTypeInteger:
    {
      Integer left = variantToInteger( val, &ok );
      if( !ok )
      {
        setLastError( i18n( "Could not convert the left operand to integer" ) );
        return false;
      }

      Integer right = variantToInteger( mOperand, &ok );
      if( !ok )
      {
        setLastError( i18n( "Could not convert the right operand to integer" ) );
        return false;
      } 

      Q_ASSERT( mOperatorDescriptor );

      switch( mOperatorDescriptor->id() )
      {
        case StandardOperatorGreaterThan:
          return left > right;
        break;
        case StandardOperatorLowerThan:
          return left < right;
        break;
        case StandardOperatorIntegerIsEqualTo:
          return left == right;
        break;
        default:
          Q_ASSERT_X( false, __FUNCTION__, "You either forgot to handle an operator or to provide your own implementation of the PropertyTest class" );
        break;
      }
    }
    break;
    case DataTypeString:
    {
      QString left = val.toString();
      QString right = mOperand.toString();

      Q_ASSERT( mOperatorDescriptor );

      switch( mOperatorDescriptor->id() )
      {
        case StandardOperatorStringIsEqualTo:
          return left == right;
        break;
        case StandardOperatorContains:
          return left.contains( right );
        break;
        case StandardOperatorIntegerIsEqualTo:
          return left == right;
        break;
        case StandardOperatorIsInAddressbook:
          // FIXME: Implementation missing
          Q_ASSERT_X( false, __FUNCTION__, "Not implemented yet" );
        break;
        case StandardOperatorStringMatchesRegexp:
        {
          QRegExp exp( right );
          return left.contains( exp );
        }
        break;
        case StandardOperatorStringMatchesWildcard:
        {
          QRegExp exp( right, Qt::CaseSensitive, QRegExp::Wildcard );
          return left.contains( exp );
        }
        break;
        default:
          Q_ASSERT_X( false, __FUNCTION__, "You either forgot to handle an operator or to provide your own implementation of the PropertyTest class" );
        break;
      }
    }
    break;
    case DataTypeDate:
    {
      QDateTime left = val.toDateTime();
      if( !left.isValid() )
      {
        setLastError( i18n( "Could not convert the left operand to date/time" ) );
        return false;
      }
      QDateTime right = mOperand.toDateTime();
      if( !right.isValid() )
      {
        setLastError( i18n( "Could not convert the right operand to date/time" ) );
        return false;
      }

      Q_ASSERT( mOperatorDescriptor );

      switch( mOperatorDescriptor->id() )
      {
        case StandardOperatorDateIsEqualTo:
          return left.date() == right.date();
        break;
        case StandardOperatorDateIsAfter:
          return left.date() > right.date();
        break;
        case StandardOperatorDateIsBefore:
          return left.date() < right.date();
        break;
        default:
          Q_ASSERT_X( false, __FUNCTION__, "You either forgot to handle an operator or to provide your own implementation of the PropertyTest class" );
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
      Q_ASSERT_X( false, __FUNCTION__, "You either forgot to handle an operator or to provide your own implementation of the PropertyTest class" );
    break;
  }

  return false;
}

void PropertyTest::dump( const QString &prefix )
{
  debugOutput( prefix, QString::fromAscii( "Condition::PropertyTest(%1,%2,%3,%4)" )
      .arg( mFunctionDescriptor->name() )
      .arg( mDataMemberDescriptor->name() )
      .arg( mOperatorDescriptor ? mOperatorDescriptor->name() : QString::fromAscii( "Boolean" ) )
      .arg( mOperand.toString() )
    );
}

} // namespace Condition

} // namespace Filter

} // namespace Akonadi

