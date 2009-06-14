/****************************************************************************** *
 *
 *  File : rulelist.cpp
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

#include <akonadi/filter/rulelist.h>

#include <QtCore/QObject>

#include <KDebug>

namespace Akonadi
{
namespace Filter 
{
namespace Action
{

RuleList::RuleList( Component * parent )
  : Base( ActionTypeRuleList, parent )
{
}

RuleList::~RuleList()
{
  qDeleteAll( mRuleList );
}

bool RuleList::isRuleList() const
{
  return true;
}


RuleList::ProcessingStatus RuleList::execute( Data * data )
{
  Q_ASSERT( data );

  setLastError( QString() );

  foreach ( Rule * rule, mRuleList )
  {
    switch ( rule->execute( data ) )
    {
      case SuccessAndContinue:
        // okie, move on
      break;
      case SuccessAndStop:
        // okie, but stop
        return SuccessAndStop;
      break;
      case Failure:
        setLastError( QObject::tr( "Rule execution failed: %1" ).arg( rule->lastError() ) );
        return Failure;
      break;
      default:
        Q_ASSERT( false ); // invalid rule execution result
        return Failure;
      break;
    }
  }

  return SuccessAndContinue;
}

void RuleList::dumpRuleList( const QString &prefix )
{
  QString localPrefix = prefix;
  localPrefix += QLatin1String("  ");
  foreach( Rule * rule, mRuleList )
    rule->dump( localPrefix );
}

void RuleList::dump( const QString &prefix )
{
  debugOutput( prefix, "Action::RuleList" );
  dumpRuleList( prefix );
}


} // namespace Action

} // namespace Filter

} // namespace Akonadi

