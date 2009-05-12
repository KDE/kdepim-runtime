/****************************************************************************** *
 *
 *  File : sievedecoder.cpp
 *  Created on Sun 03 May 2009 12:10:16 by Szymon Tomasz Stefanek
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

#include "sievedecoder.h"

#include "componentfactory.h"
#include "program.h"
#include "rule.h"
#include "sievereader.h"
#include "condition.h"

#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtCore/QByteArray>

namespace Akonadi
{
namespace Filter
{
namespace IO
{

SieveDecoder::SieveDecoder( ComponentFactory * componentFactory )
  : Decoder( componentFactory ), mProgram( 0 )
{
}

SieveDecoder::~SieveDecoder()
{
  if( mProgram )
    delete mProgram;
}

void SieveDecoder::onError( const QString & error )
{
  mGotError = true;
  setLastError( error );
}

void SieveDecoder::onTestStart( const QString & identifier )
{
  if( mGotError )
    return; // ignore everything

  Condition::Base * condition = 0;

  if( QString::compare( identifier, QLatin1String( "allof" ), Qt::CaseInsensitive ) )
    condition = componentFactory()->createAndCondition( mCurrentComponent );
  else if( QString::compare( identifier, QLatin1String( "anyof" ), Qt::CaseInsensitive ) )
    condition = componentFactory()->createOrCondition( mCurrentComponent );
  else {
    // unrecognized test
    mGotError = true; 
    setLastError( QObject::tr( "Unrecognized test type '%1'" ).arg( identifier ) );
    return;
  }

  Q_ASSERT( condition );

  if( mCurrentComponent->isRule() )
  {
    Rule * rule = static_cast< Rule * >( mCurrentComponent );
    if ( rule->condition() )
    {
      mGotError = true; 
      setLastError( QObject::tr( "Unexpected start of test '%1' after a previous test" ).arg( identifier ) );
      return;  
    }
    rule->setCondition( condition );
    mCurrentComponent = condition;
    return;
  }

  if( mCurrentComponent->isCondition() )
  {
    // hm
    switch( static_cast< Condition::Base *>( mCurrentComponent )->conditionType() )
    {
      case Condition::Base::ConditionTypeAnd:
      case Condition::Base::ConditionTypeOr:
      {
        Condition::Multi * multi = static_cast< Condition::Multi * >( mCurrentComponent );
        multi->addChildCondition( condition );
        mCurrentComponent = condition;
        return;
      }
      break;
      default:
        // unexpected
      break;
    }
  }

  delete condition;

  mGotError = true;
  setLastError( QObject::tr( "Unexpected start of test '%1'" ).arg( identifier ) );
}

void SieveDecoder::onTestEnd()
{
  if( mGotError )
    return; // ignore everything

  if( !mCurrentComponent->isCondition() )
  {
    mGotError = true;
    setLastError( QObject::tr( "Unexpected end of test outside of a condition" ) );
    return;
  }

  // pop
  mCurrentComponent = mCurrentComponent->parent();
}

void SieveDecoder::onTestListStart()
{
  if( mGotError )
    return; // ignore everything

  if( mCurrentComponent->isRule() )
  {
    // assume "anyof"
    Rule * rule = static_cast< Rule * >( mCurrentComponent );
    if ( rule->condition() )
    {
      mGotError = true; 
      setLastError( QObject::tr( "Unexpected start of test list after a previous test" ) );
      return;  
    }
    Condition::Or * condition = componentFactory()->createOrCondition( mCurrentComponent );
    rule->setCondition( condition );
    mCurrentComponent = condition;
    return;
  }

  if( mCurrentComponent->isCondition() )
  {
    // this is a parenthesis after either an allof or anyof
    switch( static_cast< Condition::Base *>( mCurrentComponent )->conditionType() )
    {
      case Condition::Base::ConditionTypeAnd:
      case Condition::Base::ConditionTypeOr:
        // ok
        return;
      break;
      default:
        // unexpected
      break;
    }
  }

  mGotError = true;
  setLastError( QObject::tr( "Unexpected start of test list" ) );

}

void SieveDecoder::onTestListEnd()
{
  if( mGotError )
    return; // ignore everything

  if( mCurrentComponent->isCondition() )
  {
    // this is a parenthesis after either an allof or anyof
    switch( static_cast< Condition::Base *>( mCurrentComponent )->conditionType() )
    {
      case Condition::Base::ConditionTypeAnd:
      case Condition::Base::ConditionTypeOr:
        // ok
        return;
      break;
      default:
        // unexpected
      break;
    }
  }

  mGotError = true;
  setLastError( QObject::tr( "Unexpected end of test list" ) );
}


void SieveDecoder::onCommandStart( const QString & identifier )
{
  if( mGotError )
    return; // ignore everything

  if(
      QString::compare( identifier, QLatin1String( "if" ), Qt::CaseInsensitive ) ||
      QString::compare( identifier, QLatin1String( "elsif" ), Qt::CaseInsensitive ) ||
      QString::compare( identifier, QLatin1String( "else" ), Qt::CaseInsensitive )
    )
  {
    // conditional rule start
    if( mCurrentComponent->isProgram() )
    {
      // a rule for the current program
      Program * program = static_cast< Program * >( mCurrentComponent );
      Rule * rule = componentFactory()->createRule( mCurrentComponent );
      program->addRule( rule );
      mCurrentComponent = rule;
      return;
    }

    if( mCurrentComponent->isRule() )
    {
      // must be an action 
    }

    // unrecognized command
    mGotError = true; 
    setLastError( QObject::tr( "Unexpected start of command '%1' outside of a program/rule" ).arg( identifier ) );
    return;
  }

  // unrecognized command
  mGotError = true; 
  setLastError( QObject::tr( "Unrecognized command '%1'" ).arg( identifier ) );
}

void SieveDecoder::onCommandEnd()
{
  if( mGotError )
    return; // ignore everything

  if( !( mCurrentComponent->isRule() || mCurrentComponent->isAction()) )
  {
    mGotError = true;
    setLastError( QObject::tr( "Unexpected end of command outside of a rule/action" ) );
    return;
  }

  // pop
  mCurrentComponent = mCurrentComponent->parent();
}

Program * SieveDecoder::run()
{
  QString script = QString::fromUtf8(
       "# Test script\r\n" \
       "if allof( size :over 100K, size :below 200K) {\r\n" \
          "fileinto \"huge\";\r\n" \
       "}\r\n"
    );

  QByteArray utf8Script = script.toUtf8();

  KSieve::Parser parser( utf8Script.data(), utf8Script.data() + utf8Script.length() );

  Private::SieveReader sieveReader( this );

  parser.setScriptBuilder( &sieveReader );

  if( mProgram )
    delete mProgram;

  mProgram = componentFactory()->createProgram( 0 );
  Q_ASSERT( mProgram ); // component factory shall never fail creating a Program

  mGotError = false;
  mCurrentComponent = mProgram;

  if( !parser.parse() )
  {
    // force an error
    if( mGotError )
    {
      mGotError = true;
      setLastError( QObject::tr( "Internal sieve library error" ) );
    }
  }

  if( mGotError )
  {
    qDebug( "ERROR: %s", lastError().toUtf8().data() );
    return 0;
  }

  Program * prog = mProgram;
  mProgram = 0;
  return prog;
}


} // namespace IO

} // namespace Filter

} // namespace Akonadi

