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

#include <KDebug>

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

} // namespace Condition

} // namespace Filter

} // namespace Akonadi

