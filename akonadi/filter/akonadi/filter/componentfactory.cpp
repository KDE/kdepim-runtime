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
  // register the basic functions

  registerDataMember(
      new DataMemberDescriptor(
          QString::fromAscii( "from" ),
          i18n( "From address" ),
          DataTypeAddress
        )
    );

  registerDataMember(
      new DataMemberDescriptor(
          QString::fromAscii( "cc" ),
          i18n( "CC address list" ),
          DataTypeAddressList
        )
    );

  registerDataMember(
      new DataMemberDescriptor(
          QString::fromAscii( "anyrecipient" ),
          i18n( "recipient address list" ),
          DataTypeAddressList
        )
    );

  registerDataMember(
      new DataMemberDescriptor(
          QString::fromAscii( "bcc" ),
          i18n( "BCC address list" ),
          DataTypeAddressList
        )
    );

  registerDataMember(
      new DataMemberDescriptor(
          QString::fromAscii( "anyheader" ),
          i18n( "header list" ),
          DataTypeStringList
        )
    );

  registerDataMember(
      new DataMemberDescriptor(
          QString::fromAscii( "item" ),
          i18n( "whole item" ),
          DataTypeString
        )
    );





  registerFunction(
      new FunctionDescriptor(
          StandardFunctionSizeOf,
          QString::fromAscii( "size" ),
          i18n( "if the total size of" ),
          DataTypeInteger,
          DataTypeString | DataTypeStringList | DataTypeAddress | DataTypeAddressList
        )
    );

  registerFunction(
      new FunctionDescriptor(
          StandardFunctionCountOf,
          QString::fromAscii( "count" ),
          i18n( "if the total count of" ),
          DataTypeInteger,
          DataTypeStringList | DataTypeAddressList
        )
    );

  registerFunction(
      new FunctionDescriptor(
          StandardFunctionValueOf,
          QString::fromAscii( "header" ),
          i18n( "if the value of" ),
          DataTypeString | DataTypeStringList | DataTypeAddress | DataTypeAddressList,
          DataTypeString | DataTypeStringList | DataTypeAddress | DataTypeAddressList
        )
    );

  registerFunction(
      new FunctionDescriptor(
          StandardFunctionAnyEMailAddressIn,
          QString::fromAscii( "address" ),
          i18n( "if any address in" ),
          DataTypeAddress | DataTypeAddressList,
          DataTypeAddress | DataTypeAddressList
        )
    );

#if 0
  registerFunction(
      new FunctionDescriptor(
          StandardFunctionAnyEMailAddressIn,
          QString::fromAscii( "address:all" ),
          i18n( "if any address in" ),
          DataTypeAddress | DataTypeAddressList,
          DataTypeAddress | DataTypeAddressList
        )
    );
#endif

  registerFunction(
      new FunctionDescriptor(
          StandardFunctionAnyEMailAddressDomainIn,
          QString::fromAscii( "address:domain" ),
          i18n( "if any domain in" ),
          DataTypeString | DataTypeStringList,
          DataTypeAddress | DataTypeAddressList
        )
    );


  registerFunction(
      new FunctionDescriptor(
          StandardFunctionAnyEMailAddressLocalPartIn,
          QString::fromAscii( "address:local" ),
          i18n( "if any username in" ),
          DataTypeString | DataTypeStringList,
          DataTypeAddress | DataTypeAddressList
        )
    );


  registerFunction(
      new FunctionDescriptor(
          StandardFunctionDateIn,
          QString::fromAscii( "date" ),
          i18n( "if the date from" ),
          DataTypeDateTime,
          DataTypeString | DataTypeDateTime
        )
    );

  registerFunction(
      new FunctionDescriptor(
          StandardFunctionExists,
          QString::fromAscii( "exists" ),
          i18n( "if exists" ),
          DataTypeBoolean,
          DataTypeString | DataTypeStringList | DataTypeDateTime | DataTypeInteger | DataTypeBoolean | DataTypeAddress | DataTypeAddressList
        )
    );





  registerOperator(
      new OperatorDescriptor(
          StandardOperatorGreaterThan,
          QString::fromAscii( "above" ),
          i18n( "is greater than" ),
          DataTypeInteger,
          DataTypeInteger
        )
    );

  registerOperator(
      new OperatorDescriptor(
          StandardOperatorLowerThan,
          QString::fromAscii( "below" ),
          i18n( "is lower than" ),
          DataTypeInteger,
          DataTypeInteger
        )
    );

  registerOperator(
      new OperatorDescriptor(
          StandardOperatorIntegerIsEqualTo,
          QString::fromAscii( "equals" ),
          i18n( "is equal to" ),
          DataTypeInteger,
          DataTypeInteger
        )
    );

  registerOperator(
      new OperatorDescriptor(
          StandardOperatorContains,
          QString::fromAscii( "contains" ),
          i18n( "contains string" ),
          DataTypeString | DataTypeStringList | DataTypeAddress | DataTypeAddressList,
          DataTypeString
        )
    );

  registerOperator(
      new OperatorDescriptor(
          StandardOperatorStringIsEqualTo,
          QString::fromAscii( "is" ),
          i18n( "is equal to" ),
          DataTypeString | DataTypeAddress,
          DataTypeString
        )
    );

  registerOperator(
      new OperatorDescriptor(
          StandardOperatorStringMatchesRegexp,
          QString::fromAscii( "matches" ),
          i18n( "matches regular expression" ),
          DataTypeString | DataTypeStringList | DataTypeAddress | DataTypeAddressList,
          DataTypeString
        )
    );

  registerOperator(
      new OperatorDescriptor(
          StandardOperatorStringMatchesWildcard,
          QString::fromAscii( "like" ),
          i18n( "matches wildcard expression" ),
          DataTypeString | DataTypeStringList | DataTypeAddress | DataTypeAddressList,
          DataTypeString
        )
    );

  registerOperator(
      new OperatorDescriptor(
          StandardOperatorIsInAddressbook,
          QString::fromAscii( "inaddressbook" ),
          i18n( "is in addressbook" ),
          DataTypeAddress | DataTypeAddressList,
          DataTypeNone
        )
    );


  registerOperator(
      new OperatorDescriptor(
          StandardOperatorDateIsEqualTo,
          QString::fromAscii( "equals" ),
          i18n( "is equal to" ),
          DataTypeDateTime,
          DataTypeDateTime
        )
    );

  registerOperator(
      new OperatorDescriptor(
          StandardOperatorDateIsAfter,
          QString::fromAscii( "after" ),
          i18n( "is after" ),
          DataTypeDateTime,
          DataTypeDateTime
        )
    );

  registerOperator(
      new OperatorDescriptor(
          StandardOperatorDateIsBefore,
          QString::fromAscii( "before" ),
          i18n( "is before" ),
          DataTypeDateTime,
          DataTypeDateTime
        )
    );


  CommandDescriptor * cmd;
  cmd = new CommandDescriptor(
      StandardCommandLeaveMessageOnServer,
      QString::fromAscii( "leaveonserver" ),
      i18n( "leave the message on server and stop processing here" ),
      true
    );

  registerCommand( cmd );

#if 0
  cmd = new CommandDescriptor(
      StandardCommandMoveToCollection,
      QString::fromAscii( "movetocollection" ),
      i18n( "move the item to collection" ),
      false
    )
  cmd.addParameter( DataTypeInteger, i18n( "collection id" ) );
  registerCommand( cmd );
#endif
}

ComponentFactory::~ComponentFactory()
{
  qDeleteAll( mCommandDescriptorHash );
  qDeleteAll( mFunctionDescriptorHash );
  qDeleteAll( mDataMemberDescriptorHash );
  qDeleteAll( mOperatorDescriptorList );
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

QList< const DataMemberDescriptor * > ComponentFactory::enumerateDataMembers( int acceptableDataTypeMask )
{
  QList< const DataMemberDescriptor * > lReturn;
  foreach( DataMemberDescriptor * dataMember, mDataMemberDescriptorHash )
  {
    if( dataMember->dataType() & acceptableDataTypeMask )
      lReturn.append( dataMember );
  }
  return lReturn;
}

void ComponentFactory::registerOperator( OperatorDescriptor * op )
{
  Q_ASSERT( op );

  QString lowerKeyword = op->keyword().toLower();

  // Get the list of operators with the same keyword
  QList< OperatorDescriptor * > existingOperatorDescriptors = mOperatorDescriptorMultiHash.values( lowerKeyword );
  if( !existingOperatorDescriptors.isEmpty() )
  {
    QList< OperatorDescriptor * > toRemove;
    OperatorDescriptor * existing;

    foreach( existing, existingOperatorDescriptors )
    {
      if( existing->id() == op->id() )
      {
        // the same operator registered twice!
        toRemove.append( existing );       
      } else {
        // warn about input data type collision
        if( existing->acceptableLeftOperandDataTypeMask() & op->acceptableLeftOperandDataTypeMask() )
        {
          kWarning() << "Multiple definitions of operator " << existing->keyword() << "collide in the input operand data types: the wrong operator may be choosen by the encoding/decoding engines";
        }
      }
    }

    foreach( existing, toRemove )
    {
      mOperatorDescriptorMultiHash.remove( lowerKeyword, existing );
      mOperatorDescriptorList.removeOne( existing );
      delete existing;
    }
  }

  mOperatorDescriptorMultiHash.insertMulti( lowerKeyword, op );
  mOperatorDescriptorList.append( op );
}

const OperatorDescriptor * ComponentFactory::findOperator( const QString &keyword, int leftOperandDataTypeMask )
{
  // Get the list of operators with the same keyword
  QList< OperatorDescriptor * > existingOperatorDescriptors = mOperatorDescriptorMultiHash.values( keyword.toLower() );
  foreach( OperatorDescriptor * op, existingOperatorDescriptors )
  {
    // the operator must cover ALL the left operand data types
    if( ( leftOperandDataTypeMask & op->acceptableLeftOperandDataTypeMask() ) == leftOperandDataTypeMask )
      return op;
  }
  return 0;
}

QList< const OperatorDescriptor * > ComponentFactory::enumerateOperators( int leftOperandDataTypeMask )
{
  QList< const OperatorDescriptor * > ret;
  foreach( OperatorDescriptor * op, mOperatorDescriptorList )
  {
    // the operator must cover ALL the left operand data types
    if( ( leftOperandDataTypeMask & op->acceptableLeftOperandDataTypeMask() ) == leftOperandDataTypeMask )
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
  return new Program();
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

Action::Base * ComponentFactory::createCommandAction( Component * parent, const CommandDescriptor * command, const QList< QVariant > &params )
{
  Q_ASSERT( parent );
  Q_ASSERT( command );
  setLastError( i18n( "Command %1 is not supported", command->keyword() ) );
  return 0;
}

Condition::Base * ComponentFactory::createPropertyTestCondition(
    Component * parent,
    const FunctionDescriptor * function,
    const DataMemberDescriptor * dataMember,
    const OperatorDescriptor * op,
    const QVariant &operand
  )
{
  Q_ASSERT( ( function->outputDataTypeMask() & op->acceptableLeftOperandDataTypeMask() ) == function->outputDataTypeMask() );
  return new Condition::PropertyTest( parent, function, dataMember, op, operand );
}

Action::RuleList * ComponentFactory::createRuleList( Component * parent )
{
  return new Action::RuleList( parent );
}

} // namespace Filter

} // namespace Akonadi

