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

#include "condition.h"
#include "data.h"
#include "datamemberdescriptor.h"
#include "functiondescriptor.h"
#include "operatordescriptor.h"

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

Not::MatchResult Not::matches( Data * data )
{
  errorStack().clearErrors();

  if( !mChildCondition )
    return ConditionDoesNotMatch;

  switch( mChildCondition->matches( data ) )
  {
    case ConditionMatches:
      return ConditionDoesNotMatch;
    break;
    case ConditionDoesNotMatch:
      return ConditionMatches;
    break;
    case ConditionMatchError:
      errorStack().pushErrorStack( mChildCondition->errorStack() );
      errorStack().pushError( i18n( "Verifying the match of the Not condition failed" ) );
      return ConditionMatchError;
    break;
    default:
      Q_ASSERT_X( false, __FUNCTION__, "Unhandled condition match result" );
    break;
  }

  Q_ASSERT_X( false, __FUNCTION__, "This point should be never reached" );

  errorStack().pushError( i18n( "Internal error" ) );
  return ConditionMatchError;
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

And::MatchResult And::matches( Data * data )
{
  errorStack().clearErrors();

  foreach( Condition::Base * child, mChildConditions )
  {
    switch( child->matches( data ) )
    {
      case ConditionMatches:
        // ok, go ahead
      break;
      case ConditionDoesNotMatch:
        return ConditionDoesNotMatch;
      break;
      case ConditionMatchError:
        errorStack().pushErrorStack( child->errorStack() );
        errorStack().pushError( i18n( "Verifying the match of the And condition failed" ) );
        return ConditionMatchError;
      break;
      default:
        Q_ASSERT_X( false, __FUNCTION__, "Unhandled condition match result" );
      break;
    }
  }

  return ConditionMatches;
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

Or::MatchResult Or::matches( Data * data )
{
  errorStack().clearErrors();

  foreach( Condition::Base * child, mChildConditions )
  {
    switch( child->matches( data ) )
    {
      case ConditionMatches:
        return ConditionMatches;
      break;
      case ConditionDoesNotMatch:
        // ok, go ahead
      break;
      case ConditionMatchError:
        errorStack().pushErrorStack( child->errorStack() );
        errorStack().pushError( i18n( "Verifying the match of the And condition failed" ) );
        return ConditionMatchError;
      break;
      default:
        Q_ASSERT_X( false, __FUNCTION__, "Unhandled condition match result" );
      break;
    }
  }

  return ConditionDoesNotMatch;
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

True::MatchResult True::matches( Data *data )
{
  Q_UNUSED( data );
  // this always matches
  return ConditionMatches;
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

False::MatchResult False::matches( Data *data )
{
  Q_UNUSED( data );
  // this never matches
  return ConditionDoesNotMatch;
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

void PropertyTest::pushError( const QString &error )
{
  errorStack().pushError( error );


  QString description;

  if( mOperatorDescriptor )
  {
    if( mOperatorDescriptor->rightOperandDataType() != DataTypeNone )
    {
      errorStack().pushError(
          i18n(
              "Evaluation of property test '%1 %2 %3 %4' failed",
              mFunctionDescriptor->name(),
              mDataMemberDescriptor->name(),
              mOperatorDescriptor->name(),
              mOperand.toString()
            )
        );      
    } else {
      errorStack().pushError(
          i18n(
              "Evaluation of property test '%1 %2 %3' failed",
              mFunctionDescriptor->name(),
              mDataMemberDescriptor->name(),
              mOperatorDescriptor->name()
            )
        );
    }
  } else {
    errorStack().pushError( i18n( "Evaluation of property test '%1 %2' failed", mFunctionDescriptor->name(), mDataMemberDescriptor->name() ) );
  }
}

PropertyTest::MatchResult PropertyTest::matches( Data * data )
{
  errorStack().clearErrors();

  kDebug() << "Executing property test data member: '" << mDataMemberDescriptor->name() << "', function: '" << mFunctionDescriptor->name() << "'";

  // Generic function test implementation.
  //
  // Override this method in a derived class if you want to provide
  // an optimized/custom match (then override ComponentFactory::createPropertyTestCondition() to return your subclasses).

  QVariant val = data->getPropertyValue( mFunctionDescriptor, mDataMemberDescriptor );

  kDebug() << "Data member value is '" << val << "'";

  if( val.isNull() )
  {
    if( data->hasErrors() )
    {
      errorStack().pushErrorStack( *data );
      pushError( i18n( "Error retrieving property value" ) );
      return ConditionMatchError;
    }
  }

  bool ok;

  switch( mOperatorDescriptor->rightOperandDataType() )
  {
    case DataTypeInteger:
    {
      Integer left = variantToInteger( val, &ok );
      if( !ok )
      {
        pushError( i18n( "Could not convert the left operand '%1' to integer", val.toString() ) );
        return ConditionMatchError;
      }

      Integer right = variantToInteger( mOperand, &ok );
      if( !ok )
      {
        pushError( i18n( "Could not convert the right operand '%1' to integer", mOperand.toString() ) );
        return ConditionMatchError;
      } 

      Q_ASSERT( mOperatorDescriptor );

      switch( mOperatorDescriptor->id() )
      {
        case OperatorGreaterThan:
          return left > right ? ConditionMatches : ConditionDoesNotMatch;
        break;
        case OperatorLowerThan:
          return left < right ? ConditionMatches : ConditionDoesNotMatch;
        break;
        case OperatorIntegerIsEqualTo:
          return left == right ? ConditionMatches : ConditionDoesNotMatch;
        break;
        default:
          Q_ASSERT_X( false, __FUNCTION__, "You either forgot to handle an operator or to provide your own implementation of the PropertyTest class" );
          pushError( i18n( "Internal error" ) );
          return ConditionMatchError;
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
        case OperatorStringIsEqualTo:
          return left == right ? ConditionMatches : ConditionDoesNotMatch;
        break;
        case OperatorContains:
          return left.contains( right ) ? ConditionMatches : ConditionDoesNotMatch;
        break;
        case OperatorIntegerIsEqualTo:
          return left == right ? ConditionMatches : ConditionDoesNotMatch;
        break;
        case OperatorStringMatchesRegexp:
        {
          QRegExp exp( right );
          return left.contains( exp ) ? ConditionMatches : ConditionDoesNotMatch;
        }
        break;
        case OperatorStringMatchesWildcard:
        {
          QRegExp exp( right, Qt::CaseSensitive, QRegExp::Wildcard );
          return left.contains( exp ) ? ConditionMatches : ConditionDoesNotMatch;
        }
        break;
        default:
          Q_ASSERT_X( false, __FUNCTION__, "You either forgot to handle an operator or to provide your own implementation of the PropertyTest class" );
          pushError( i18n( "Internal error" ) );
          return ConditionMatchError;
        break;
      }
    }
    break;
    case DataTypeDate:
    {
      QDateTime left = val.toDateTime();
      if( !left.isValid() )
      {
        pushError( i18n( "Could not convert the left operand '%1' to date/time", val.toString() ) );
        return ConditionMatchError;
      }
      QDateTime right = mOperand.toDateTime();
      if( !right.isValid() )
      {
        pushError( i18n( "Could not convert the right operand '%1' to date/time", mOperand.toString() ) );
        return ConditionMatchError;
      }

      Q_ASSERT( mOperatorDescriptor );

      switch( mOperatorDescriptor->id() )
      {
        case OperatorDateIsEqualTo:
          return left.date() == right.date() ? ConditionMatches : ConditionDoesNotMatch;
        break;
        case OperatorDateIsAfter:
          return left.date() > right.date() ? ConditionMatches : ConditionDoesNotMatch;
        break;
        case OperatorDateIsBefore:
          return left.date() < right.date() ? ConditionMatches : ConditionDoesNotMatch;
        break;
        default:
          Q_ASSERT_X( false, __FUNCTION__, "You either forgot to handle an operator or to provide your own implementation of the PropertyTest class" );
          pushError( i18n( "Internal error" ) );
          return ConditionMatchError;
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
      pushError( i18n( "Internal error" ) );
      return ConditionMatchError;
    break;
  }

  Q_ASSERT_X( false, __FUNCTION__, "This point should be never reached" );
  pushError( i18n( "Internal error" ) );
  return ConditionMatchError;
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

