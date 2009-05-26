/****************************************************************************** *
 *
 *  File : factory.cpp
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

#include "factory.h"

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



Factory::Factory()
{
  // register the basic functions

  registerDataMember(
      new DataMember(
          QString::fromAscii( "from" ),
          i18n( "From address" ),
          DataTypeAddress
        )
    );

  registerDataMember(
      new DataMember(
          QString::fromAscii( "cc" ),
          i18n( "CC address list" ),
          DataTypeAddressList
        )
    );

  registerDataMember(
      new DataMember(
          QString::fromAscii( "anyrecipient" ),
          i18n( "recipient address list" ),
          DataTypeAddressList
        )
    );

  registerDataMember(
      new DataMember(
          QString::fromAscii( "bcc" ),
          i18n( "BCC address list" ),
          DataTypeAddressList
        )
    );

  registerDataMember(
      new DataMember(
          QString::fromAscii( "anyheader" ),
          i18n( "header list" ),
          DataTypeStringList
        )
    );

  registerDataMember(
      new DataMember(
          QString::fromAscii( "item" ),
          i18n( "whole item" ),
          DataTypeString
        )
    );





  registerFunction(
      new Function(
          StandardFunctionSizeOf,
          QString::fromAscii( "size" ),
          i18n( "if the total size of" ),
          DataTypeInteger,
          DataTypeString | DataTypeStringList | DataTypeAddress | DataTypeAddressList
        )
    );

  registerFunction(
      new Function(
          StandardFunctionCountOf,
          QString::fromAscii( "count" ),
          i18n( "if the total count of" ),
          DataTypeInteger,
          DataTypeStringList | DataTypeAddressList
        )
    );

  registerFunction(
      new Function(
          StandardFunctionValueOf,
          QString::fromAscii( "header" ),
          i18n( "if the value of" ),
          DataTypeString | DataTypeStringList | DataTypeAddress | DataTypeAddressList,
          DataTypeString | DataTypeStringList | DataTypeAddress | DataTypeAddressList
        )
    );

  registerFunction(
      new Function(
          StandardFunctionAnyEMailAddressIn,
          QString::fromAscii( "address" ),
          i18n( "if any address extracted from" ),
          DataTypeAddress | DataTypeAddressList,
          DataTypeAddress | DataTypeAddressList
        )
    );

#if 0
  registerFunction(
      new Function(
          StandardFunctionAnyEMailAddressIn,
          QString::fromAscii( "address:all" ),
          i18n( "if any address extracted from" ),
          DataTypeAddress | DataTypeAddressList,
          DataTypeAddress | DataTypeAddressList
        )
    );
#endif

  registerFunction(
      new Function(
          StandardFunctionAnyEMailAddressDomainIn,
          QString::fromAscii( "address:domain" ),
          i18n( "if any domain address part extracted from" ),
          DataTypeString | DataTypeStringList,
          DataTypeAddress | DataTypeAddressList
        )
    );


  registerFunction(
      new Function(
          StandardFunctionAnyEMailAddressLocalPartIn,
          QString::fromAscii( "address:local" ),
          i18n( "if any username address part extracted from" ),
          DataTypeString | DataTypeStringList,
          DataTypeAddress | DataTypeAddressList
        )
    );


  registerFunction(
      new Function(
          StandardFunctionDateIn,
          QString::fromAscii( "date" ),
          i18n( "if the date from" ),
          DataTypeDateTime,
          DataTypeString | DataTypeDateTime
        )
    );

  registerFunction(
      new Function(
          StandardFunctionExists,
          QString::fromAscii( "exists" ),
          i18n( "if exists" ),
          DataTypeBoolean,
          DataTypeString | DataTypeStringList | DataTypeDateTime | DataTypeInteger | DataTypeBoolean | DataTypeAddress | DataTypeAddressList
        )
    );





  registerOperator(
      new Operator(
          StandardOperatorGreaterThan,
          QString::fromAscii( "above" ),
          i18n( "is greater than" ),
          DataTypeInteger,
          DataTypeInteger
        )
    );

  registerOperator(
      new Operator(
          StandardOperatorLowerThan,
          QString::fromAscii( "below" ),
          i18n( "is lower than" ),
          DataTypeInteger,
          DataTypeInteger
        )
    );

  registerOperator(
      new Operator(
          StandardOperatorIntegerIsEqualTo,
          QString::fromAscii( "equals" ),
          i18n( "is equal to" ),
          DataTypeInteger,
          DataTypeInteger
        )
    );

  registerOperator(
      new Operator(
          StandardOperatorContains,
          QString::fromAscii( "contains" ),
          i18n( "contains string" ),
          DataTypeString | DataTypeStringList | DataTypeAddress | DataTypeAddressList,
          DataTypeString
        )
    );

  registerOperator(
      new Operator(
          StandardOperatorStringIsEqualTo,
          QString::fromAscii( "is" ),
          i18n( "is equal to" ),
          DataTypeString | DataTypeAddress,
          DataTypeString
        )
    );

  registerOperator(
      new Operator(
          StandardOperatorStringMatchesRegexp,
          QString::fromAscii( "matches" ),
          i18n( "matches regular expression" ),
          DataTypeString | DataTypeStringList | DataTypeAddress | DataTypeAddressList,
          DataTypeString
        )
    );

  registerOperator(
      new Operator(
          StandardOperatorStringMatchesWildcard,
          QString::fromAscii( "like" ),
          i18n( "matches wildcard expression" ),
          DataTypeString | DataTypeStringList | DataTypeAddress | DataTypeAddressList,
          DataTypeString
        )
    );

  registerOperator(
      new Operator(
          StandardOperatorIsInAddressbook,
          QString::fromAscii( "inaddressbook" ),
          i18n( "is in addressbook" ),
          DataTypeAddress | DataTypeAddressList,
          DataTypeNone
        )
    );


  registerOperator(
      new Operator(
          StandardOperatorDateIsEqualTo,
          QString::fromAscii( "equals" ),
          i18n( "is equal to" ),
          DataTypeDateTime,
          DataTypeDateTime
        )
    );

  registerOperator(
      new Operator(
          StandardOperatorDateIsAfter,
          QString::fromAscii( "after" ),
          i18n( "is after" ),
          DataTypeDateTime,
          DataTypeDateTime
        )
    );

  registerOperator(
      new Operator(
          StandardOperatorDateIsBefore,
          QString::fromAscii( "before" ),
          i18n( "is before" ),
          DataTypeDateTime,
          DataTypeDateTime
        )
    );
}

Factory::~Factory()
{
  qDeleteAll( mFunctionHash );
  qDeleteAll( mDataMemberHash );
  qDeleteAll( mOperatorList );
}

void Factory::registerDataMember( DataMember * dataMember )
{
  Q_ASSERT( dataMember );

  mDataMemberHash.insert( dataMember->keyword().toLower(), dataMember ); // will replace
}

const DataMember * Factory::findDataMember( const QString &keyword )
{
  return mDataMemberHash.value( keyword.toLower(), 0 );
}

QList< const DataMember * > Factory::enumerateDataMembers( int acceptableDataTypeMask )
{
  QList< const DataMember * > lReturn;
  foreach( DataMember * dataMember, mDataMemberHash )
  {
    if( dataMember->dataType() & acceptableDataTypeMask )
      lReturn.append( dataMember );
  }
  return lReturn;
}

void Factory::registerOperator( Operator * op )
{
  Q_ASSERT( op );

  QString lowerKeyword = op->keyword().toLower();

  // Get the list of operators with the same keyword
  QList< Operator * > existingOperators = mOperatorMultiHash.values( lowerKeyword );
  if( !existingOperators.isEmpty() )
  {
    QList< Operator * > toRemove;
    Operator * existing;

    foreach( existing, existingOperators )
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
      mOperatorMultiHash.remove( lowerKeyword, existing );
      mOperatorList.removeOne( existing );
      delete existing;
    }
  }

  mOperatorMultiHash.insertMulti( lowerKeyword, op );
  mOperatorList.append( op );
}

const Operator * Factory::findOperator( const QString &keyword, int leftOperandDataTypeMask )
{
  // Get the list of operators with the same keyword
  QList< Operator * > existingOperators = mOperatorMultiHash.values( keyword.toLower() );
  foreach( Operator * op, existingOperators )
  {
    // the operator must cover ALL the left operand data types
    if( ( leftOperandDataTypeMask & op->acceptableLeftOperandDataTypeMask() ) == leftOperandDataTypeMask )
      return op;
  }
  return 0;
}

QList< const Operator * > Factory::enumerateOperators( int leftOperandDataTypeMask )
{
  QList< const Operator * > ret;
  foreach( Operator * op, mOperatorList )
  {
    // the operator must cover ALL the left operand data types
    if( ( leftOperandDataTypeMask & op->acceptableLeftOperandDataTypeMask() ) == leftOperandDataTypeMask )
      ret.append( op );
  }

  return ret;
}

void Factory::registerFunction( Function * function )
{
  Q_ASSERT( function );

  Function * existing = mFunctionHash.value( function->keyword(), 0 );
  if( existing )
  {
    mFunctionList.removeOne( existing );
    delete existing;
  }
   
  mFunctionHash.insert( function->keyword().toLower(), function ); // wil replace
  mFunctionList.append( function );
}


const Function * Factory::findFunction( const QString &keyword )
{
  return mFunctionHash.value( keyword.toLower(), 0 );
}

const QList< const Function * > * Factory::enumerateFunctions()
{
  return &mFunctionList;
}

Program * Factory::createProgram( Component * parent )
{
  return new Program( parent );
}

Rule * Factory::createRule( Component * parent )
{
  return new Rule( parent );
}

Condition::And * Factory::createAndCondition( Component * parent )
{
  return new Condition::And( parent );
}

Condition::Or * Factory::createOrCondition( Component * parent )
{
  return new Condition::Or( parent );
}

Condition::Not * Factory::createNotCondition( Component * parent )
{
  return new Condition::Not( parent );
}

Action::Stop * Factory::createStopAction( Component * parent )
{
  return new Action::Stop( parent );
}

Action::Base * Factory::createGenericAction( Component * parent, const QString &name )
{
  return 0; // by default we have no generic actions
}

Condition::Base * Factory::createPropertyTestCondition(
    Component * parent,
    const Function * function,
    const DataMember * dataMember,
    const Operator * op,
    const QVariant &operand
  )
{
  Q_ASSERT( ( function->outputDataTypeMask() & op->acceptableLeftOperandDataTypeMask() ) == function->outputDataTypeMask() );
  return new Condition::PropertyTest( parent, function, dataMember, op, operand );
}

Action::RuleList * Factory::createRuleList( Component * parent )
{
  return new Action::RuleList( parent );
}

} // namespace Filter

} // namespace Akonadi

