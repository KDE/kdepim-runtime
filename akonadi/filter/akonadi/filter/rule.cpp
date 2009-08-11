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

#include <QtCore/QObject>

#include <KDebug>
#include <KLocale>

namespace Akonadi
{
namespace Filter 
{

Rule::Rule( Component * parent )
  : Component( parent ), PropertyBag(), mCondition( 0 )
{
  Q_ASSERT( parent );
}

Rule::~Rule()
{
  if( mCondition )
    delete mCondition;

  qDeleteAll( mActionList );
}

void Rule::clearActionList()
{
  qDeleteAll( mActionList );
  mActionList.clear();
}


QString Rule::description() const
{
  return property( QString::fromAscii( "description" ) ).toString();
}

bool Rule::isEmpty() const
{
  if( mCondition )
    return false;
  if( !mActionList.isEmpty() )
    return false;

  return true;
}


void Rule::setDescription( const QString &description )
{
  setProperty( QString::fromAscii( "description" ), QVariant( description ) );
}

void Rule::setCondition( Condition::Base * condition )
{
  if( mCondition )
    delete mCondition;
  mCondition = condition;
}

bool Rule::isRule() const
{
  return true;
}


Rule::ProcessingStatus Rule::execute( Data * data )
{
  errorStack().clearErrors();

  kDebug() << "Executing rule " << this;

  if( mCondition )
  {
    switch( mCondition->matches( data ) )
    {
      case Condition::Base::ConditionMatches:
        // ok, condition matched
      break;
      case Condition::Base::ConditionDoesNotMatch:
        kDebug() << "Condition didn't match: skipping actions";
        return SuccessAndContinue; 
      break;
      case Condition::Base::ConditionMatchError:
        errorStack().pushErrorStack( mCondition->errorStack() );
        errorStack().pushError( i18n( "Evaluation of the condition failed" ) );
        return Failure;
      break;
    }
  }

  kDebug() << "Condition matched: executing actions!";


  int idx = 0;

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
        errorStack().pushErrorStack( action->errorStack() );
        errorStack().pushError( i18n( "Execution of action #%1 failed", idx + 1 ) );
        return Failure;
      break;
      default:
        Q_ASSERT( false ); // invalid action result
        return Failure;
      break;
    }

    idx++;
  }

  return SuccessAndContinue;
}

void Rule::dump( const QString &prefix )
{
  debugOutput( prefix, "Rule" );
  QString localPrefix = prefix;
  localPrefix += QLatin1String("  ");
  if ( mCondition )
    mCondition->dump( localPrefix );  
  else
    debugOutput( localPrefix, "Null Condition (Always True)" );
  foreach( Action::Base * action, mActionList )
    action->dump( localPrefix );
}

} // namespace Filter

} // namespace Akonadi

