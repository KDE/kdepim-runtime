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
#include "program.h"
#include "action.h"
#include "condition.h"
#include "rule.h"
#include "rulelist.h"

#include <KLocale>

namespace Akonadi
{
namespace Filter 
{

ComponentFactory::ComponentFactory()
{
  // register the basic attributes

  registerAttribute(
      new Attribute(
          QString::fromAscii( "size" ),
          i18n( "Size" ),
          i18n( "The size of the whole item" ),
          Attribute::DataTypeInteger
        )
    );

}

ComponentFactory::~ComponentFactory()
{
  qDeleteAll( mAttributeHash );
}

void ComponentFactory::registerAttribute( Attribute * attribute )
{
  Q_ASSERT( attribute );

  Attribute * existing = mAttributeHash.value( attribute->id(), 0 );
  if( existing )
  {
    mAttributeList.removeOne( existing );
    delete existing;
  }
   
  mAttributeHash.insert( attribute->id(), attribute ); // wil replace
  mAttributeList.append( attribute );
}


const Attribute * ComponentFactory::findAttribute( const QString &id )
{
  return mAttributeHash.value( id, 0 );
}

const QList< const Attribute * > & ComponentFactory::enumerateAttributes()
{
  return mAttributeList;
}

Program * ComponentFactory::createProgram( Component * parent )
{
  return new Program( parent );
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

Action::Stop * ComponentFactory::createStopAction( Component * parent )
{
  return new Action::Stop( parent );
}

Action::Base * ComponentFactory::createGenericAction( Component * parent, const QString &name )
{
  return 0; // by default we have no generic actions
}

Condition::Base * ComponentFactory::createAttributeTestCondition( Component * parent, const Attribute * attribute )
{
  return 0; // by default we have no generic conditions
}


Action::RuleList * ComponentFactory::createRuleList( Component * parent )
{
  return new Action::RuleList( parent );
}

} // namespace Filter

} // namespace Akonadi

