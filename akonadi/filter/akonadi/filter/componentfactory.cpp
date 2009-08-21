/****************************************************************************** *
 *
 *  File : componentfactory.cpp
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

#include "componentfactory.h"

#include "action.h"
#include "condition.h"
#include "program.h"
#include "rule.h"
#include "rulelist.h"

#include <KLocale>
#include <KDebug>

namespace Akonadi
{
namespace Filter 
{

ComponentFactory::ComponentFactory()
{
  // register the basic functions (everyone needs this)

#if 0
  // handled by "header", automagically
  registerFunction(
      new FunctionDescriptor(
          FunctionValueOf,
          QString::fromAscii( "header" ),
          QString::fromAscii( "" ), // the value of
          DataTypeString,
          0, // provides no special features
          DataTypeString
        )
    );
#endif

  registerFunction(
      new FunctionDescriptor(
          FunctionSizeOf,
          QString::fromAscii( "size" ),
          i18n( "the total size of" ),
          DataTypeInteger,
          0, // provides no special features
          DataTypeString | DataTypeStringList
        )
    );

  registerFunction(
      new FunctionDescriptor(
          FunctionCountOf,
          QString::fromAscii( "count" ),
          i18n( "the total count of" ),
          DataTypeInteger,
          0, // provides no special features
          DataTypeStringList
        )
    );

  registerFunction(
      new FunctionDescriptor(
          FunctionDateIn,
          QString::fromAscii( "date" ),
          i18n( "the date from" ),
          DataTypeDate,
          0, // provides no special features
          DataTypeString,
          FeatureContainsDate // requires feature "contains date"
        )
    );

#if 0
  // This is basically "sizeof() > 0"
  registerFunction(
      new FunctionDescriptor(
          FunctionExists,
          QString::fromAscii( "exists" ),
          i18n( "exists" ),
          DataTypeBoolean,
          0,
          DataTypeString | DataTypeStringList | DataTypeDate | DataTypeInteger | DataTypeBoolean
        )
    );
#endif


  registerOperator(
      new OperatorDescriptor(
          OperatorGreaterThan,
          QString::fromAscii( "above" ),
          i18n( "is greater than" ),
          DataTypeInteger, // left operand must be integer
          0, // and no special features are requested from it
          DataTypeInteger
        )
    );

  registerOperator(
      new OperatorDescriptor(
          OperatorLowerThan,
          QString::fromAscii( "below" ),
          i18n( "is lower than" ),
          DataTypeInteger, // left operand must be integer
          0, // and no special features are requested from it
          DataTypeInteger
        )
    );

  registerOperator(
      new OperatorDescriptor(
          OperatorIntegerIsEqualTo,
          QString::fromAscii( "equals" ),
          i18n( "is equal to" ),
          DataTypeInteger, // left operand must be integer
          0, // and no special features are requested from it
          DataTypeInteger
        )
    );

  registerOperator(
      new OperatorDescriptor(
          OperatorContains,
          QString::fromAscii( "contains" ),
          i18n( "contains string" ),
          DataTypeString | DataTypeStringList, // left operand must be string or string list
          0, // and no special features are requested from it
          DataTypeString
        )
    );

  registerOperator(
      new OperatorDescriptor(
          OperatorStringIsEqualTo,
          QString::fromAscii( "is" ),
          i18n( "is equal to" ),
          DataTypeString | DataTypeStringList, // left operand must be string or string list
          0, // and no special features are requested from it
          DataTypeString
        )
    );

  registerOperator(
      new OperatorDescriptor(
          OperatorStringMatchesRegexp,
          QString::fromAscii( "matches" ),
          i18n( "matches regular expression" ),
          DataTypeString | DataTypeStringList, // left operand must be string or string list
          0, // and no special features are requested from it
          DataTypeString
        )
    );

  registerOperator(
      new OperatorDescriptor(
          OperatorStringMatchesWildcard,
          QString::fromAscii( "like" ),
          i18n( "matches wildcard expression" ),
          DataTypeString | DataTypeStringList, // left operand must be string or string list
          0, // and no special features are requested from it
          DataTypeString
        )
    );

  registerOperator(
      new OperatorDescriptor(
          OperatorDateIsEqualTo,
          QString::fromAscii( "on" ),
          i18n( "is equal to" ),
          DataTypeDate, // left operand must be date
          0, // and no special features are requested from it
          DataTypeDate
        )
    );

  registerOperator(
      new OperatorDescriptor(
          OperatorDateIsAfter,
          QString::fromAscii( "after" ),
          i18n( "is after" ),
          DataTypeDate, // left operand must be date
          0, // and no special features are requested from it
          DataTypeDate
        )
    );

  registerOperator(
      new OperatorDescriptor(
          OperatorDateIsBefore,
          QString::fromAscii( "before" ),
          i18n( "is before" ),
          DataTypeDate, // left operand must be date
          0, // and no special features are requested from it
          DataTypeDate
        )
    );
}

ComponentFactory::~ComponentFactory()
{
  qDeleteAll( mCommandDescriptorHash );
  qDeleteAll( mFunctionDescriptorHash );
  qDeleteAll( mDataMemberDescriptorHash );
  qDeleteAll( mOperatorDescriptorHash );
}



void ComponentFactory::registerDataMember( DataMemberDescriptor * dataMember )
{
  Q_ASSERT( dataMember );

  mDataMemberDescriptorHash.insert( dataMember->keyword().toLower(), dataMember ); // will replace
}

const DataMemberDescriptor * ComponentFactory::findDataMember( const QString &keyword )
{
  return mDataMemberDescriptorHash.value( keyword.toLower(), 0 );
}

QList< const DataMemberDescriptor * > ComponentFactory::enumerateDataMembers()
{
  QList< const DataMemberDescriptor * > lReturn;
  foreach( DataMemberDescriptor * dataMember, mDataMemberDescriptorHash )
    lReturn.append( dataMember );
  return lReturn;
}


QList< const DataMemberDescriptor * > ComponentFactory::enumerateDataMembers( int acceptableDataTypeMask, int requiredFeatureBits )
{
  QList< const DataMemberDescriptor * > lReturn;
  foreach( DataMemberDescriptor * dataMember, mDataMemberDescriptorHash )
  {
    if( !( dataMember->dataType() & acceptableDataTypeMask ) )
      continue;
    if( ( dataMember->featureMask() & requiredFeatureBits ) != requiredFeatureBits )
      continue;

    lReturn.append( dataMember );
  }
  return lReturn;
}

void ComponentFactory::registerOperator( OperatorDescriptor * op )
{
  Q_ASSERT( op );

  QString lowerKeyword = op->keyword().toLower();

  mOperatorDescriptorHash.insert( lowerKeyword, op );
}

const OperatorDescriptor * ComponentFactory::findOperator( const QString &keyword )
{
  return mOperatorDescriptorHash.value( keyword.toLower(), 0 );
}

QList< const OperatorDescriptor * > ComponentFactory::enumerateOperators( DataType leftOperandDataType, int featureBits )
{
  QList< const OperatorDescriptor * > ret;
  foreach( OperatorDescriptor * op, mOperatorDescriptorHash )
  {
    if( !( leftOperandDataType & op->acceptableLeftOperandDataTypeMask() ) )
      continue;
    if( ( featureBits & op->requiredLeftOperandFeatureMask() ) != op->requiredLeftOperandFeatureMask() )
      continue;
    ret.append( op );
  }

  return ret;
}

void ComponentFactory::registerFunction( FunctionDescriptor * function )
{
  Q_ASSERT( function );

  FunctionDescriptor * existing = mFunctionDescriptorHash.value( function->keyword(), 0 );
  if( existing )
  {
    mFunctionDescriptorList.removeOne( existing );
    delete existing;
  }
   
  mFunctionDescriptorHash.insert( function->keyword().toLower(), function ); // wil replace
  mFunctionDescriptorList.append( function );
}


const FunctionDescriptor * ComponentFactory::findFunction( const QString &keyword )
{
  return mFunctionDescriptorHash.value( keyword.toLower(), 0 );
}

const QList< const FunctionDescriptor * > * ComponentFactory::enumerateFunctions()
{
  return &mFunctionDescriptorList;
}

void ComponentFactory::registerCommand( CommandDescriptor * command )
{
  Q_ASSERT( command );

  CommandDescriptor * existing = mCommandDescriptorHash.value( command->keyword(), 0 );
  if( existing )
  {
    mCommandDescriptorList.removeOne( existing );
    delete existing;
  }
   
  mCommandDescriptorHash.insert( command->keyword().toLower(), command ); // wil replace
  mCommandDescriptorList.append( command );
}


const CommandDescriptor * ComponentFactory::findCommand( const QString &keyword )
{
  return mCommandDescriptorHash.value( keyword.toLower(), 0 );
}

const QList< const CommandDescriptor * > * ComponentFactory::enumerateCommands()
{
  return &mCommandDescriptorList;
}

Program * ComponentFactory::createProgram()
{
  return new Program( this );
}

Rule * ComponentFactory::createRule( Component * parent )
{
  return new Rule( parent );
}

Condition::And * ComponentFactory::createAndCondition( Component * parent )
{
  return new Condition::And( parent );
}

Condition::Or * ComponentFactory::createOrCondition( Component * parent )
{
  return new Condition::Or( parent );
}

Condition::Not * ComponentFactory::createNotCondition( Component * parent )
{
  return new Condition::Not( parent );
}

Condition::True * ComponentFactory::createTrueCondition( Component * parent )
{
  return new Condition::True( parent );
}

Condition::False * ComponentFactory::createFalseCondition( Component * parent )
{
  return new Condition::False( parent );
}

Action::Stop * ComponentFactory::createStopAction( Component * parent )
{
  return new Action::Stop( parent );
}

Action::Command * ComponentFactory::createCommand( Component * parent, const CommandDescriptor * command, const QList< QVariant > &params )
{
  Q_ASSERT( parent );
  Q_ASSERT( command );

  // FIXME: Should check that the actual parameters match formal parameters ?

  return new Action::Command( parent, command, params );
}

Condition::Base * ComponentFactory::createPropertyTestCondition(
    Component * parent,
    const FunctionDescriptor * function,
    const DataMemberDescriptor * dataMember,
    const OperatorDescriptor * op,
    const QVariant &operand
  )
{
  if( function )
  {
    Q_ASSERT( function->outputDataType() & op->acceptableLeftOperandDataTypeMask() );
  }
  return new Condition::PropertyTest( parent, function, dataMember, op, operand );
}

Action::RuleList * ComponentFactory::createRuleList( Component * parent )
{
  return new Action::RuleList( parent );
}

} // namespace Filter

} // namespace Akonadi

