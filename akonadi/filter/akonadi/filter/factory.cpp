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
#include "program.h"
#include "action.h"
#include "condition.h"
#include "rule.h"
#include "rulelist.h"

#include <KLocale>
#include <KDebug>

namespace Akonadi
{
namespace Filter 
{


Factory::OperatorSet::OperatorSet()
{
}

Factory::OperatorSet::~OperatorSet()
{
  qDeleteAll( mOperatorHash );
}

void Factory::OperatorSet::registerOperator( Operator * op )
{
  Q_ASSERT( op );

  Operator * existing = mOperatorHash.value( op->id(), 0 );
  if( existing )
  {
    mOperatorList.removeOne( existing );
    delete existing;
  }
   
  mOperatorHash.insert( op->id().toLower(), op ); // wil replace
  mOperatorList.append( op );
}







Factory::Factory()
{
  // register the basic properties

  registerProperty(
      new Property(
          QString::fromAscii( "size" ),
          i18n( "size of" ),
          i18n( "The size of a specific item part. If the specified item doesn't exist then the returned size is 0." ),
          DataTypeInteger
        )
    );

  registerProperty(
      new Property(
          QString::fromAscii( "header" ),
          i18n( "value of" ),
          i18n( "Returns the string value of the specified item part" ),
          DataTypeString
        )
    );

  registerProperty(
      new Property(
          QString::fromAscii( "address" ),
          i18n( "any address in" ),
          i18n( "Returns a list of addresses extracted from the specified item part" ),
          DataTypeString
        )
    );

  registerProperty(
      new Property(
          QString::fromAscii( "address:all" ),
          i18n( "any address in" ),
          i18n( "Returns a list of addresses extracted from the specified item part" ),
          DataTypeString
        )
    );

  registerProperty(
      new Property(
          QString::fromAscii( "address:domain" ),
          i18n( "any domain address part in" ),
          i18n( "Returns a list of addresses extracted from the specified item part" ),
          DataTypeString
        )
    );


  registerProperty(
      new Property(
          QString::fromAscii( "address:local" ),
          i18n( "any local address part in" ),
          i18n( "Returns a list of addresses extracted from the specified item part" ),
          DataTypeString
        )
    );


  registerProperty(
      new Property(
          QString::fromAscii( "date" ),
          i18n( "date in" ),
          i18n( "Returns the first date extracted from the specified item part" ),
          DataTypeDateTime
        )
    );

  registerProperty(
      new Property(
          QString::fromAscii( "exists" ),
          i18n( "exists" ),
          i18n( "Returns true if the specified item part exists" ),
          DataTypeBoolean
        )
    );





  registerOperator(
      new Operator(
          QString::fromAscii( "above" ),
          i18n( "Is Greater Than" ),
          i18n( "Returns true if the left operand has greater numeric value than the right operand" ),
          DataTypeInteger
        )
    );

  registerOperator(
      new Operator(
          QString::fromAscii( "below" ),
          i18n( "Is Lower Than" ),
          i18n( "Returns true if the left operand has smaller numeric value than the right operand" ),
          DataTypeInteger
        )
    );

  registerOperator(
      new Operator(
          QString::fromAscii( "matches" ),
          i18n( "Matches Regular Expression" ),
          i18n( "Returns true if the left operand matches the regular expression specified by the right operand" ),
          DataTypeString
        )
    );
}

Factory::~Factory()
{
  qDeleteAll( mPropertyHash );
  qDeleteAll( mOperatorSetHash );
}

void Factory::registerOperator( Operator * op )
{
  Q_ASSERT( op );

  OperatorSet * set = mOperatorSetHash.value( op->dataType(), 0 );
  if( !set )
  {
    set = new OperatorSet();
    mOperatorSetHash.insert( op->dataType(), set );
  }

  set->registerOperator( op );
}

const Operator * Factory::findOperator( DataType dataType, const QString &id )
{
  OperatorSet * set = mOperatorSetHash.value( dataType, 0 );
  if( !set )
    return 0;
  return set->findOperator( id );
}

const QList< const Operator * > * Factory::enumerateOperators( DataType dataType )
{
  OperatorSet * set = mOperatorSetHash.value( dataType, 0 );
  if( !set )
    return 0;
  return set->enumerateOperators();
}

void Factory::registerProperty( Property * property )
{
  Q_ASSERT( property );

  Property * existing = mPropertyHash.value( property->id(), 0 );
  if( existing )
  {
    mPropertyList.removeOne( existing );
    delete existing;
  }
   
  mPropertyHash.insert( property->id().toLower(), property ); // wil replace
  mPropertyList.append( property );
}


const Property * Factory::findProperty( const QString &id )
{
  return mPropertyHash.value( id.toLower(), 0 );
}

const QList< const Property * > * Factory::enumerateProperties()
{
  return &mPropertyList;
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

Condition::Base * Factory::createPropertyTestCondition( Component * parent, const Property * property, const QString &propertyArgument, const Operator * op, const QVariant &operand )
{
  Q_ASSERT( property->dataType() == op->dataType() );
  return new Condition::PropertyTest( parent, property, propertyArgument, op, operand );
}

Action::RuleList * Factory::createRuleList( Component * parent )
{
  return new Action::RuleList( parent );
}

} // namespace Filter

} // namespace Akonadi

