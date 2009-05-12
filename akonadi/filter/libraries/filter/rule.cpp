/****************************************************************************** *
 *
 *  File : rule.cpp
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

#include "rule.h"
#include "data.h"
#include "condition.h"
#include "action.h"

#include <QObject>

namespace Akonadi
{
namespace Filter 
{

Rule::Rule( Component * parent )
  : Component( ComponentTypeRule, parent ), mCondition( 0 )
{
  Q_ASSERT( parent );
}

Rule::~Rule()
{
  if( mCondition )
    delete mCondition;

  qDeleteAll( mActionList );
}

void Rule::setCondition( Condition::Base * condition )
{
  if( mCondition )
    delete mCondition;
  mCondition = condition;
}


Rule::ProcessingStatus Rule::execute( Data * data )
{
  if( mCondition )
  {
    if( !mCondition->matches( data ) )
      return SuccessAndContinue; 
  }

  foreach( Action::Base * action, mActionList )
  {
    switch( action->execute( data ) )
    {
      case SuccessAndContinue:
        // go on
      break;
      case SuccessAndStop:
        // propagate the stop up
        return SuccessAndStop;
      break;
      case Failure:
        setLastError( QObject::tr("Action execution failed: %1").arg( action->lastError() ) );
        return Failure;
      break;
      default:
        Q_ASSERT( false ); // invalid action result
        return Failure;
      break;
    }
  }

  return SuccessAndContinue;
}


} // namespace Filter

} // namespace Akonadi

