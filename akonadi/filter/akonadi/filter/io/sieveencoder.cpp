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

#include "sieveencoder.h"
#include "../functiondescriptor.h"
#include "../datamemberdescriptor.h"
#include "../operatordescriptor.h"
#include "../commanddescriptor.h"
#include "../action.h"
#include "../condition.h"
#include "../program.h"
#include "../rule.h"
#include "../rulelist.h"

#include <QtCore/QDateTime>
#include <QtCore/QStringList>
#include <QtCore/QUrl>

#include <KLocale>

namespace Akonadi
{
namespace Filter
{
namespace IO
{


static inline QString encode_property_field( const QString &field, bool encodeEqualitySign )
{
  if( encodeEqualitySign )
    return QString::fromLatin1( QUrl::toPercentEncoding( field, " =,!?(){}[]/\\", "" ) ); // value, particular exclude chars, particular include chars
  return QString::fromLatin1( QUrl::toPercentEncoding( field, " ,!?(){}[]/\\", "" ) ); // value, particular exclude chars, particular include chars
}


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

QByteArray SieveEncoder::run( Program * program )
{
  errorStack().clearErrors();

  mCurrentIndentPrefix = QString();
  mIndentLevel = 0;

  addLine( QLatin1String( "# Akonadi Filtering Program" ) );
  addLine( QString::fromAscii( "# Encoded at %1" ).arg( QDateTime::currentDateTime().toString() ) );

  addLine( QString() );

  const QHash< QString, QVariant > & props = program->allProperties();

  if( !props.isEmpty() )
  {
    for( QHash< QString, QVariant >::ConstIterator it = props.constBegin(); it != props.constEnd(); ++it )
    {
      addLine(
          QString::fromAscii( "# program::%1 = %2" )
              .arg( encode_property_field( it.key(), true ) )
              .arg( encode_property_field( it.value().toString(), false ) )
        );
    }
    addLine( QString() );
  }

  if( !encodeRuleList( program ) )
  {
    errorStack().pushError( i18n( "encoder main" ), i18n( "Could not encode program" ) );
    return QByteArray();
  }

  addLine( QString() ); // empty line at end

  Q_ASSERT( mIndentLevel == 0 );

  return mBuffer.toUtf8();
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
  Q_UNUSED( isFirst );
  Q_UNUSED( isLast );

  Condition::Base * condition = rule->condition();

  bool startedScope = false;

  const QHash< QString, QVariant > & props = rule->allProperties();

  if( !props.isEmpty() )
  {
    //addLine( QString() );

    for( QHash< QString, QVariant >::ConstIterator it = props.constBegin(); it != props.constEnd(); ++it )
    {
      QString value = it.value().toString();
      if( value.isEmpty() )
        continue;

      addLine(
          QString::fromAscii( "# rule::%1 = %2" )
              .arg( encode_property_field( it.key(), true ) )
              .arg( encode_property_field( it.value().toString(), false ) )
        );
    }
  }

  //if( !isFirst )
    //addLine( QString() );

  if( condition )
  {
    beginLine();

    addLineData( QLatin1String( "if " ) );

    if( !encodeCondition( condition ) )
      return false;

    addLineData( QLatin1String( " {" ) );

    endLine();

    startedScope = true;
    pushIndent();
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
      addLineData( QLatin1String( "\"" ) );
      addLineData( data.toString().replace( QChar( '\\' ), QLatin1String( "\\\\" ) ).replace( QChar( '"' ), QLatin1String( "\"" ) ) );
      addLineData( QLatin1String( "\"" ) );
    break;
    case DataTypeInteger:
    {
      bool ok;
      qint64 val = data.toLongLong( &ok );
      if( !ok )
      {
        errorStack().pushError(
            i18n( "encode data" ),
            i18n( "The filter is invalid: expected data type is integer, but the data instance could not be converted to a number" )
          );
        return false;
      }
      addLineData( QString::fromAscii( "%1" ).arg( val ) ); 
    }
    break;
    case DataTypeStringList:
    {
      QStringList sl = data.toStringList();
      if( sl.isEmpty() )
      {
        errorStack().pushError(
            i18n( "encode data" ),
            i18n( "The filter is invalid: expected data type is string list, but the data instance could not be converted to a string list" )
          );
        return false;
      }

      idx = 0;
      cnt = sl.count();

      addLineData( QLatin1String( "[" ) );

      foreach( QString s, sl )
      {
        addLineData( QLatin1String( "\"" ) );
        addLineData( s.replace( QChar( '\\' ), QLatin1String( "\\\\" ) ).replace( QChar( '"' ), QLatin1String( "\"" ) ) );
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
        errorStack().pushError(
            i18n( "encode condition" ),
            i18n( "The filter is invalid: and/or condition with no children" )
          );
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
      {
        errorStack().pushError(
            i18n( "encode not" ),
            i18n( "Could not encode the not inner condition" )
          );
        return false;
      }
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

      QStringList tokens;
      if( test->function() )
        tokens = test->function()->keyword().split( QChar( ':' ) );
      else
        tokens << QString::fromAscii( "header" ); // sieve compatible "valueof"

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

      if( test->operatorDescriptor() )
      {
        addLineData(
            QString( ":%1 \"%2\" " )
              .arg( test->operatorDescriptor()->keyword() )
              .arg( test->dataMember()->keyword() )
          );

        QVariant data = test->operand();

        if( !encodeData( test->operand(), test->operatorDescriptor()->rightOperandDataType() ) )
        {
          errorStack().pushError(
              i18n( "encode property test" ),
              i18n( "Could not parameter data" )
            );
          return false;
        }
      } else {
        addLineData(
            QString( "\"%2\" " )
              .arg( test->dataMember()->keyword() )
          );
      }
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
      addLine( QLatin1String( "stop;" ) );
    break;
    case Action::ActionTypeRuleList:
      if( !encodeRuleList( static_cast< Action::RuleList * >( action ) ) )
      {
        errorStack().pushError(
            i18n( "encode action" ),
            i18n( "Could not encode inner rule list" )
          );
        return false;
      }
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
        QList< QVariant >::ConstIterator dataIt = static_cast< Action::Command * >( action )->parameters()->begin();

        while( it != params->end() )
        {
          Q_ASSERT( dataIt != static_cast< Action::Command * >( action )->parameters()->end() );

          addLineData( QLatin1String( " " ) );

          if( !encodeData( *dataIt, ( *it )->dataType() ) )
          {
            errorStack().pushError(
                i18n( "encode command" ),
                i18n( "Could not encode command parameter" )
              );
            return false;
          }

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
