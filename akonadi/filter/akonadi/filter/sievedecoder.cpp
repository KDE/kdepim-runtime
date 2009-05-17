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

#include "factory.h"
#include "program.h"
#include "rule.h"
#include "sievereader.h"
#include "condition.h"
#include "rulelist.h"
#include "property.h"

#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtCore/QByteArray>

#include <KLocale>

#define DEBUG_SIEVE_DECODER 1

// Sieve is somewhat an ugly language. It's hard to parse, has some ambiguities
// and a human has trouble understanding its constructs. We actually support
// a subset of the whole language and we extend it in other directions to support
// constructs which the original specification is unable to encode.

namespace Akonadi
{
namespace Filter
{
namespace IO
{

SieveDecoder::SieveDecoder( Factory * componentFactory )
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

bool SieveDecoder::addConditionToCurrentComponent( Condition::Base * condition, const QString &identifier )
{
  if( mCurrentComponent->isRule() )
  {
    Rule * rule = static_cast< Rule * >( mCurrentComponent );
    if ( rule->condition() )
    {
      delete condition;
      mGotError = true; 
      setLastError( i18n( "Unexpected start of test '%1' after a previous test", identifier ) );
      return false;  
    }
    rule->setCondition( condition );
    mCurrentComponent = condition; // always becomes current
    return true;
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
        return true;
      }
      break;
      case Condition::Base::ConditionTypeNot:
      {
        Condition::Not * whoTheHeckDefinedNotToTheExclamationMark = static_cast< Condition::Not * >( mCurrentComponent );
        Condition::Base * currentNotChild = whoTheHeckDefinedNotToTheExclamationMark->childCondition();
        if( currentNotChild )
        {
          // ugly.. the not alreay has a child condition: it should have.
          // we fix this by creating an inner "or"
          whoTheHeckDefinedNotToTheExclamationMark->releaseChildCondition();
          Condition::Or * orCondition = factory()->createOrCondition( mCurrentComponent );
          Q_ASSERT( orCondition );
          whoTheHeckDefinedNotToTheExclamationMark->setChildCondition( orCondition );
          orCondition->addChildCondition( currentNotChild );
          orCondition->addChildCondition( condition );
        } else {
          whoTheHeckDefinedNotToTheExclamationMark->setChildCondition( condition );
        }
        mCurrentComponent = condition;
        return true;
      }
      break;
      default:
        // unexpected
      break;
    }
  }

  delete condition;

  mGotError = true;
  setLastError( i18n( "Unexpected start of test '%1' outside of a rule or a condition", identifier ) );
  return false;
}

void SieveDecoder::onTestStart( const QString & identifier )
{
  if( mGotError )
    return; // ignore everything

  if( !mCurrentSimpleTestName.isEmpty() )
  {
    // a test can't start inside a simple test
    mGotError = true; 
    setLastError( i18n( "Unexpected start of test '%1' inside a simple test", identifier ) );
    return;  
  }

  Condition::Base * condition = 0;

  if( QString::compare( identifier, QLatin1String( "allof" ), Qt::CaseInsensitive ) == 0 )
    condition = factory()->createAndCondition( mCurrentComponent );
  else if( QString::compare( identifier, QLatin1String( "anyof" ), Qt::CaseInsensitive ) == 0 )
    condition = factory()->createOrCondition( mCurrentComponent );
  else if( QString::compare( identifier, QLatin1String( "not" ), Qt::CaseInsensitive ) == 0 )
    condition = factory()->createNotCondition( mCurrentComponent );
  else {
    // we delay the start of the simple tests to "onTestEnd", as they can't be structured.
    mCurrentSimpleTestName = identifier;
    mCurrentSimpleTestArguments.clear();
    return;
  }

  mCurrentSimpleTestName = QString(); // this is not a simple test

  Q_ASSERT( condition );

  addConditionToCurrentComponent( condition, identifier );
}

void SieveDecoder::onTestEnd()
{
  if( mGotError )
    return; // ignore everything

  if( !mCurrentSimpleTestName.isEmpty() )
  {
    // this is the end of a simple test

    // here you have most of the weirdness of the sieve syntax

    // standard sieve syntax
    //
    //   testname {:<modifier> {<modifierArgument>}} [:<operator>] {:<modifier> {<modifierArgument>}} [ header_name ] [ values ]
    //

    const Property * property = factory()->findProperty( mCurrentSimpleTestName );
    if ( !property )
    {
      // unrecognized test
      mGotError = true; 
      setLastError( i18n( "Unrecognized property '%1'", mCurrentSimpleTestName ) );
      return;
    }

    // Must have at least 1 non optional argument (the value)
    if( mCurrentSimpleTestArguments.count() < 1 )
    {
      mGotError = true;
      setLastError( i18n( "The 'header' test requires at one non-optional argument" ) );
      return;
    }

    QList< QVariant >::Iterator it = mCurrentSimpleTestArguments.end();

    --it;
    QVariant values = *it;

    // do we have field names around ?
    QVariant fieldNames;

    if ( it != mCurrentSimpleTestArguments.begin() )
    {
      --it;
      fieldNames = *it;
      // check if it is not a tagged argument
      if( fieldNames.type() == QVariant::String )
      {
        QString maybeTaggedArgument = fieldNames.toString();
        if( maybeTaggedArgument.length() > 1 )
        {
          if ( maybeTaggedArgument.at( 0 ) == QChar( ':' ) )
            fieldNames = QVariant(); // this is a tagged argument
        }
      }
    }

    QStringList fieldList;

    mCurrentSimpleTestArguments.removeLast();
    if ( !fieldNames.isNull() )
    {
      mCurrentSimpleTestArguments.removeLast();

      if( !qVariantCanConvert< QStringList >( fieldNames ) )
      {
        mGotError = true;
        setLastError( i18n( "The 'header' field name argument must be convertible to a string list" ) );
        return;
      }

      fieldList = fieldNames.toStringList();
    } else {
      // use a string list with a single null value
      fieldList.append( QString() );
    }
    
    const Operator * op = 0;

    foreach( QVariant argVariant, mCurrentSimpleTestArguments )
    {
      if( argVariant.type() != QVariant::String )
        continue;
      QString arg = argVariant.toString();
      Q_ASSERT( arg.length() > 0 );
      QString argNoTag = arg.at( 0 ) == QChar(':') ? arg.mid(1) : arg;
      op = factory()->findOperator( property->dataType(), argNoTag );
      if( op )
      {
        mCurrentSimpleTestArguments.removeOne( arg );
        break;
      }
    }

    if( !op )
    {
      // unrecognized test
      mGotError = true;
      switch( mCurrentSimpleTestArguments.count() )
      {
        case 0:
          setLastError( i18n( "Property '%1' expects an operator", mCurrentSimpleTestName ) );
        break;
        case 1:
          setLastError( i18n( "Invalid operator '%1' for property '%2'", mCurrentSimpleTestArguments.at( 0 ).toString() , mCurrentSimpleTestName ) );
        break;
        default:
          setLastError( i18n( "No such operator for property '%1'", mCurrentSimpleTestName ) );
        break;
      }
      return;
    }

    QList< QVariant > valueList;

    switch( values.type() )
    {
      case QVariant::ULongLong:
        valueList.append( values );
      break;
      case QVariant::String:
        valueList.append( values );
      break;
      case QVariant::StringList:
      {
        QStringList sl = values.toStringList();
        foreach( QString stringValue, sl )
          valueList.append( QVariant( stringValue ) );
      }
      break;
      default:
        Q_ASSERT( false ); // shouldn't happen
      break;
    }

    bool multiTest = false;

    // if there is more than one field or more than one value then we use an "or" test as parent
    if( ( valueList.count() > 1 ) || ( fieldList.count() > 1 ) )
    {
      Condition::Or * orCondition = factory()->createOrCondition( mCurrentComponent );
      Q_ASSERT( orCondition );

      if( !addConditionToCurrentComponent( orCondition, QLatin1String( "anyof" ) ) )
        return; // error already signaled

      multiTest = true;
    }

    int first = true;
    foreach( QString field, fieldList )
    {
      foreach( QVariant value, valueList )
      {
        Condition::Base * condition = factory()->createPropertyTestCondition( mCurrentComponent, property, field, op, value );
        if ( !condition )
        {
          // unrecognized test
          mGotError = true; 
          setLastError( i18n( "Test on property '%1' isn't supported", field ) );
          return;
        }

        if( !addConditionToCurrentComponent( condition, mCurrentSimpleTestName ) )
          return; // error already set

        if( multiTest )
        {
          // pop it (so the current condition becomes the or added above)

          mCurrentComponent = mCurrentComponent->parent();
          Q_ASSERT( mCurrentComponent ); // a condition always has a parent
          Q_ASSERT( mCurrentComponent->isCondition() );
        }
      }
    }

    mCurrentSimpleTestName = QString();
  }

  if( !mCurrentComponent->isCondition() )
  {
    mGotError = true;
    setLastError( i18n( "Unexpected end of test outside of a condition" ) );
    return;
  }

  // pop
  mCurrentComponent = mCurrentComponent->parent();
  Q_ASSERT( mCurrentComponent ); // a condition always has a parent
  Q_ASSERT( mCurrentComponent->isCondition() || mCurrentComponent->isRule() );
}

void SieveDecoder::onTaggedArgument( const QString & tag )
{
  if( mGotError )
    return; // ignore everything

  if( !mCurrentSimpleCommandName.isEmpty() )
  {
    Q_ASSERT( mCurrentSimpleTestName.isEmpty() );
    // argument to a simple command
    mCurrentSimpleCommandArguments.append( QVariant( tag ) );
    return;
  }

  if( !mCurrentSimpleTestName.isEmpty() )
  {
    // argument to a simple test
    QString bleah = QChar( ':' );
    bleah += tag;
    mCurrentSimpleTestArguments.append( QVariant( bleah ) );
    return;
  }

  mGotError = true;
  setLastError( i18n( "Unexpected tagged argument outside of a simple test or simple command" ) );
}

void SieveDecoder::onStringArgument( const QString & string, bool multiLine, const QString & embeddedHashComment )
{
  if( mGotError )
    return; // ignore everything

  if( !mCurrentSimpleCommandName.isEmpty() )
  {
    Q_ASSERT( mCurrentSimpleTestName.isEmpty() );
    // argument to a simple command
    mCurrentSimpleCommandArguments.append( QVariant( string ) );
    return;
  }

  if( !mCurrentSimpleTestName.isEmpty() )
  {
    // argument to a simple test
    mCurrentSimpleTestArguments.append( QVariant( string ) );
    return;
  }

  mGotError = true;
  setLastError( i18n( "Unexpected string argument outside of a simple test or simple command" ) );
}

void SieveDecoder::onNumberArgument( unsigned long number, char quantifier )
{
  if( mGotError )
    return; // ignore everything

  if( !mCurrentSimpleCommandName.isEmpty() )
  {
    Q_ASSERT( mCurrentSimpleTestName.isEmpty() );
    // argument to a simple command
    mCurrentSimpleCommandArguments.append( QVariant( (qulonglong)number ) );
    return;
  }

  if( !mCurrentSimpleTestName.isEmpty() )
  {
    // argument to a simple test
    mCurrentSimpleTestArguments.append( QVariant( (qulonglong)number ) );
    return;
  }

  mGotError = true;
  setLastError( i18n( "Unexpected number argument outside of a simple test or simple command" ) );
}

void SieveDecoder::onStringListArgumentStart()
{
  if( mGotError )
    return; // ignore everything

  if( !mCurrentStringList.isEmpty() )
  {
    mGotError = true;
    setLastError( i18n( "Unexpected start of string list inside a string list" ) ); 
  }
}

void SieveDecoder::onStringListEntry( const QString & string, bool multiLine, const QString & embeddedHashComment )
{
  if( mGotError )
    return; // ignore everything

  mCurrentStringList.append( string );
}

void SieveDecoder::onStringListArgumentEnd()
{
  if( mGotError )
    return; // ignore everything

  if( !mCurrentSimpleCommandName.isEmpty() )
  {
    Q_ASSERT( mCurrentSimpleTestName.isEmpty() );
    // argument to a simple command
    mCurrentSimpleCommandArguments.append( QVariant( mCurrentStringList ) );
    mCurrentStringList.clear();
    return;
  }

  if( !mCurrentSimpleTestName.isEmpty() )
  {
    // argument to a simple test
    mCurrentSimpleTestArguments.append( QVariant( mCurrentStringList ) );
    mCurrentStringList.clear();
    return;
  }

  mGotError = true;
  setLastError( i18n( "Unexpected string list argument outside of a simple test or simple command" ) );
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
      setLastError( i18n( "Unexpected start of test list after a previous test" ) );
      return;  
    }
    Condition::Or * condition = factory()->createOrCondition( mCurrentComponent );
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
      case Condition::Base::ConditionTypeNot:
        // ok
        return;
      break;
      default:
        // unexpected
      break;
    }
  }

  mGotError = true;
  setLastError( i18n( "Unexpected start of test list" ) );

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
      case Condition::Base::ConditionTypeNot:
        // ok
        return;
      break;
      default:
        // unexpected
      break;
    }
  }

  mGotError = true;
  setLastError( i18n( "Unexpected end of test list" ) );
}


void SieveDecoder::onCommandStart( const QString & identifier )
{
  if( mGotError )
    return; // ignore everything

  if ( !( mCurrentComponent->isRuleList() || mCurrentComponent->isRule() ) )
  {
    // unrecognized command
    mGotError = true; 
    setLastError( i18n( "Unexpected start of command '%1' outside of a rule list/rule", identifier ) );
    return;
  }

  Rule * rule;
  Action::RuleList * ruleList;

  if(
      ( QString::compare( identifier, QLatin1String( "if" ), Qt::CaseInsensitive ) == 0 ) ||
      ( QString::compare( identifier, QLatin1String( "elsif" ), Qt::CaseInsensitive ) == 0 ) ||
      ( QString::compare( identifier, QLatin1String( "else" ), Qt::CaseInsensitive ) == 0 )
    )
  {
    mCurrentSimpleCommandName = QString(); // this is not a simple command

    // conditional rule start
    if( mCurrentComponent->isRuleList() )
    {
      // a rule for the current rule list
      ruleList = static_cast< Action::RuleList * >( mCurrentComponent );
      rule = factory()->createRule( mCurrentComponent );
      ruleList->addRule( rule );
      mCurrentComponent = rule;
      return;
    }

    // a rule-list action
    Q_ASSERT( mCurrentComponent->isRule() );

    // create the rule list and add it as action
    rule = static_cast< Rule * >( mCurrentComponent );
    ruleList = factory()->createRuleList( mCurrentComponent );
    rule->addAction( ruleList );

    // create the rule and add it to the rule list we've just created
    rule = factory()->createRule( mCurrentComponent );
    ruleList->addRule( rule );
    mCurrentComponent = rule;

    return;
  }

  // We delay the creation of simple commands to the onCommandEnd() so we have all the arguments
  mCurrentSimpleCommandName = identifier;
  mCurrentSimpleCommandArguments.clear();
}

void SieveDecoder::onCommandEnd()
{
  if( mGotError )
    return; // ignore everything

  if( !mCurrentSimpleCommandName.isEmpty() )
  {
    // delayed simple command creation

    Q_ASSERT( mCurrentComponent->isRuleList() || mCurrentComponent->isRule() );

    Action::Base * action;
    if (
         ( QString::compare( mCurrentSimpleCommandName, QLatin1String("stop" ), Qt::CaseInsensitive ) == 0 ) ||
         ( QString::compare( mCurrentSimpleCommandName, QLatin1String("keep" ), Qt::CaseInsensitive ) == 0 )
      )
    {
      action = factory()->createStopAction( mCurrentComponent );
      Q_ASSERT( action );

    } else {

      action = factory()->createGenericAction( mCurrentComponent, mCurrentSimpleCommandName );

      if ( !action )
      {
        mGotError = true; 
        setLastError( i18n( "Unrecognized action '%1'", mCurrentSimpleCommandName ) );
        return;
      }
    }

    Q_ASSERT( action );
    mCurrentSimpleCommandName = QString();

    if ( mCurrentComponent->isRuleList() )
    {
      // unconditional rule start with an action
      Action::RuleList * ruleList = static_cast< Action::RuleList * >( mCurrentComponent );
      Rule * rule = factory()->createRule( mCurrentComponent );
      ruleList->addRule( rule );

      rule->addAction( action );

      mCurrentComponent = action;
      return;
    }

    Q_ASSERT( mCurrentComponent->isRule() );

    // a plain action within the current rule
    static_cast< Rule * >( mCurrentComponent )->addAction( action );

    mCurrentComponent = action;

  }

  // not a simple command

  if( !( mCurrentComponent->isRule() || mCurrentComponent->isAction()) )
  {
    mGotError = true;
    setLastError( i18n( "Unexpected end of command outside of a rule/action" ) );
    return;
  }


  // pop
  mCurrentComponent = mCurrentComponent->parent();
}

void SieveDecoder::onBlockStart()
{
  if( mGotError )
    return; // ignore everything

  if ( !mCurrentComponent->isRule() )
  {
    mGotError = true;
    setLastError( i18n( "Unexpected start of block outisde of a rule" ) );
    return;
  }
}

void SieveDecoder::onBlockEnd()
{
  if( mGotError )
    return; // ignore everything

}

Program * SieveDecoder::run()
{
  QString script = QString::fromUtf8(
       "# Test script\r\n" \
       "if allof( size :above 100K, size :below 200K) {\r\n" \
       "  fileinto \"huge\";\r\n" \
       "  stop;\r\n" \
       "}\r\n"
       "if allof( size :below 100K, not( size :below 20K ), not( header :matches \"from\" \"x\", header :matches \"to\" \"y\" ) ) {\r\n" \
       "  fileinto \"medium\";\r\n" \
       "  if not allof( size :below 80K, not size :below [\"From\", \"To\"] 70K) {\r\n" \
       "    fileinto \"strange\";\r\n" \
       "    keep;\r\n" \
       "  }\r\n"
       "}\r\n"
    );

  QByteArray utf8Script = script.toUtf8();

  KSieve::Parser parser( utf8Script.data(), utf8Script.data() + utf8Script.length() );

  Private::SieveReader sieveReader( this );

  parser.setScriptBuilder( &sieveReader );

  if( mProgram )
    delete mProgram;

  mProgram = factory()->createProgram( 0 );
  Q_ASSERT( mProgram ); // component factory shall never fail creating a Program

  mGotError = false;
  mCurrentComponent = mProgram;

  if( !parser.parse() )
  {
    // force an error
    if( mGotError )
    {
      mGotError = true;
      setLastError( i18n( "Internal sieve library error" ) );
    }
  }

#ifdef DEBUG_SIEVE_DECODER
  qDebug( "\nSCRIPT\n" );
  qDebug( "%s", script.toUtf8().data() );
  qDebug( "\nTREE\n" );
  if ( mProgram )
  {
    QString prefix;
    mProgram->dump( prefix );
  }
  qDebug( "\n" );
#endif //DEBUG_SIEVE_DECODER

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

