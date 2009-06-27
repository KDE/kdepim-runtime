/****************************************************************************** *
 *
 *  File : sieveencoder.cpp
 *  Created on Sun 15 Jun 2009 00:14:16 by Szymon Tomasz Stefanek
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

#include <akonadi/filter/io/sieveencoder.h>
#include <akonadi/filter/functiondescriptor.h>
#include <akonadi/filter/datamemberdescriptor.h>
#include <akonadi/filter/operatordescriptor.h>
#include <akonadi/filter/commanddescriptor.h>
#include <akonadi/filter/action.h>
#include <akonadi/filter/condition.h>
#include <akonadi/filter/program.h>
#include <akonadi/filter/rule.h>
#include <akonadi/filter/rulelist.h>

#include <QtCore/QDateTime>
#include <QtCore/QStringList>

#include <KLocale>

namespace Akonadi
{
namespace Filter
{
namespace IO
{

SieveEncoder::SieveEncoder()
  : Encoder()
{
  mIndentText = QLatin1String( "	" );
  mLineSeparator = QLatin1String( "\n" );
}

SieveEncoder::~SieveEncoder()
{
}

void SieveEncoder::beginLine()
{
  mBuffer += mCurrentIndentPrefix;
}

void SieveEncoder::addLineData( const QString &data )
{
  mBuffer += data;
}

void SieveEncoder::endLine()
{
  mBuffer += mLineSeparator;
}

void SieveEncoder::addLine( const QString &line )
{
  beginLine();
  addLineData( line );
  endLine();
}

void SieveEncoder::pushIndent()
{
  mIndentLevel++;
  mCurrentIndentPrefix = QString();
  for( int i=0; i < mIndentLevel; i++ )
    mCurrentIndentPrefix += mIndentText;
}

void SieveEncoder::popIndent()
{
  Q_ASSERT( mIndentLevel > 0 );
  mIndentLevel--;
  mCurrentIndentPrefix = QString();
  for( int i=0; i < mIndentLevel; i++ )
    mCurrentIndentPrefix += mIndentText;
}

QString SieveEncoder::run( Program * program )
{
  mCurrentIndentPrefix = QString();
  mIndentLevel = 0;

  addLine( QLatin1String( "# Akonadi Filter Program" ) );
  addLine( QString( "# Encoded at %1" ).arg( QDateTime::currentDateTime().toString() ) );

  addLine( QString() );

  if( !encodeRuleList( program ) )
    return QString();

  addLine( QString() ); // empty line at end

  Q_ASSERT( mIndentLevel == 0 );

  return mBuffer;
}

bool SieveEncoder::encodeRuleList( Action::RuleList * ruleList )
{
  const QList< Rule * > * rules = ruleList->ruleList();

  Q_ASSERT( rules );

  int idx = 0;
  int cnt = rules->count();

  foreach( Rule * rule, *rules )
  {
    if( !encodeRule( rule, idx == 0, idx == ( cnt - 1 ) ) )
      return false;

    idx++;
  }

  return true;
}

bool SieveEncoder::encodeRule( Rule * rule, bool isFirst, bool isLast )
{
  Condition::Base * condition = rule->condition();

  bool startedScope = false;

  if( condition )
  {
    beginLine();

    if( isFirst )
      addLineData( QLatin1String( "if " ) );
    else
      addLineData( QLatin1String( "elsif " ) );

    if( !encodeCondition( condition ) )
      return false;

    addLineData( QLatin1String( " {" ) );
    endLine();

    startedScope = true;
    pushIndent();
  } else {
    Q_ASSERT( isFirst || isLast );

    if ( !isFirst )
    {
      addLine( QLatin1String( "else {" ) );
      startedScope = true;
      pushIndent();
    }
  }

  QList< Action::Base * > * actions = rule->actionList();

  Q_ASSERT( actions );

  foreach( Action::Base * act, *actions )
  {
    if( !encodeAction( act ) )
      return false;
  }

  if( startedScope )
  {
    popIndent();
    addLine( QLatin1String( "}" ) );
  }

  return true;
}

bool SieveEncoder::encodeData( const QVariant &data, DataType dataType )
{
  int idx,cnt;

  switch( dataType )
  {
    case DataTypeNone:
      // nuthin
    break;
    case DataTypeString:
    case DataTypeAddress:
      addLineData( QLatin1String( "\"" ) );
      addLineData( data.toString() );
      addLineData( QLatin1String( "\"" ) );
    break;
    case DataTypeInteger:
    {
      bool ok;
      qint64 val = data.toLongLong( &ok );
      if( !ok )
      {
        setLastError( i18n( "The filter is invalid: expected data type is integer, but the data instance could not be converted to a number" ) );
        return false;
      }
      addLineData( QString( "%1" ).arg( val ) ); 
    }
    break;
    case DataTypeStringList:
    case DataTypeAddressList:
    {
      QStringList sl = data.toStringList();
      if( sl.isEmpty() )
      {
        setLastError( i18n( "The filter is invalid: expected data type is string list, but the data instance could not be converted to a string list" ) );
        return false;
      }

      idx = 0;
      cnt = sl.count();

      addLineData( QLatin1String( "[" ) );

      foreach( QString s, sl )
      {
        addLineData( QLatin1String( "\"" ) );
        addLineData( s );
        addLineData( QLatin1String( "\"" ) );
        if( idx < ( cnt - 1 ) )
          addLineData( QLatin1String( ", " ) );
        idx++;
      }

      addLineData( QLatin1String( "]" ) );

    }
    break;
    case DataTypeBoolean:
      addLineData( data.toBool() ? QLatin1String( "true" ) : QLatin1String( "false" ) ); 
    break;
    case DataTypeDate:
      Q_ASSERT_X( false, "SieveEncoder::encodeData()", "Date encoding not implemented yet!" );
    break;
    default:
      Q_ASSERT_X( false, "SieveEncoder::encodeData()", "Unhandled data type" );
    break;
  }

  return true;
}

bool SieveEncoder::encodeCondition( Condition::Base * condition )
{
  int idx;
  int cnt;

  switch( condition->conditionType() )
  {
    case Condition::ConditionTypeAnd:
    case Condition::ConditionTypeOr:
    {
      addLineData( condition->conditionType() == Condition::ConditionTypeAnd ? QLatin1String( "allof (" ) : QLatin1String( "anyof (" ) );
      endLine();

      const QList< Condition::Base * > * childConditions = static_cast< Condition::Multi * >( condition )->childConditions();
      Q_ASSERT( childConditions );
      if( childConditions->isEmpty() )
      {
        setLastError( i18n( "The filter is invalid: and/or condition with no children" ) );
        return false;
      }

      pushIndent();
      pushIndent();

      idx = 0;
      cnt = childConditions->count();

      foreach( Condition::Base * child, *childConditions )
      {
        beginLine();
        if( !encodeCondition( child ) )
          return false;
        if( idx < ( cnt - 1 ) )
          addLineData( QLatin1String( "," ) );
        endLine();
        idx++;
      }
      
      popIndent();
      beginLine();
      addLineData( QLatin1String( ")" ) );
      popIndent();
    } 
    break;

    break;
    case Condition::ConditionTypeNot:
      addLineData( QLatin1String( "not " ) );
      if( !encodeCondition( static_cast< Condition::Not * >( condition )->childCondition() ) )
        return false;
    break;
    case Condition::ConditionTypeTrue:
      addLineData( QLatin1String( "true" ) );
    break;
    case Condition::ConditionTypeFalse:
      addLineData( QLatin1String( "false" ) );
    break;
    case Condition::ConditionTypePropertyTest:
    {
      Condition::PropertyTest * test = static_cast< Condition::PropertyTest * >( condition );

      QStringList tokens = test->function()->keyword().split( QChar( ':' ) );
      idx = 0;
      foreach( QString tok, tokens )
      {
        if( tok.isEmpty() )
          continue;
        if( idx > 0 )
          addLineData( QLatin1String( ":" ) );
        addLineData( tok );
        addLineData( QLatin1String( " " ) );
        idx++;
      }

      addLineData(
          QString( ":%1 \"%2\" " )
            .arg( test->functionOperatorDescriptor()->keyword() )
            .arg( test->dataMember()->keyword() )
        );


      QVariant data = test->operand();

      if( !encodeData( test->operand(), test->functionOperatorDescriptor()->rightOperandDataType() ) )
        return false;
    }
    break;
    default:
      Q_ASSERT_X( false, "SieveEncoder::encodeCondition()", "Unhandled condition type" );
    break;
  }

  return true;
}

bool SieveEncoder::encodeAction( Action::Base * action )
{
  switch( action->actionType() )
  {
    case Action::ActionTypeStop:
      addLine( QLatin1String( "keep;" ) );
    break;
    case Action::ActionTypeRuleList:
      if( !encodeRuleList( static_cast< Action::RuleList * >( action ) ) )
        return false;
    break;
    case Action::ActionTypeCommand:
    {
      const CommandDescriptor * command = static_cast< Action::Command * >( action )->commandDescriptor();

      const QList< CommandDescriptor::ParameterDescriptor * > * params = command->parameters();
      Q_ASSERT( params );

      beginLine();
      addLineData( command->keyword() );

      if( params->count() > 0 )
      {
        QList< CommandDescriptor::ParameterDescriptor * >::ConstIterator it = params->begin();
        QList< QVariant >::ConstIterator dataIt = static_cast< Action::Command * >( action )->params()->begin();

        while( it != params->end() )
        {
          Q_ASSERT( dataIt != static_cast< Action::Command * >( action )->params()->end() );

          addLineData( QLatin1String( " " ) );

          if( !encodeData( *dataIt, ( *it )->dataType() ) )
            return false;

          ++it;
          ++dataIt;
        }
      }

      addLineData( QLatin1String( ";" ) );
      endLine();
    }
    break;
    default:
      Q_ASSERT_X( false, "SieveEncoder::encodeAction()", "Unhandled action type" );
    break;
  }

  return true;
}

} // namespace IO

} // namespace Filter

} // namespace Akonadi
