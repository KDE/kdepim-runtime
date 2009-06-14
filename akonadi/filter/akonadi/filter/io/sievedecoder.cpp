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

#include <akonadi/filter/io/sievedecoder.h>

#include <akonadi/filter/io/sievereader.h>

#include <akonadi/filter/componentfactory.h>
#include <akonadi/filter/program.h>
#include <akonadi/filter/rule.h>
#include <akonadi/filter/condition.h>
#include <akonadi/filter/rulelist.h>
#include <akonadi/filter/functiondescriptor.h>

#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtCore/QByteArray>

#include <KDebug>
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

SieveDecoder::SieveDecoder( ComponentFactory * componentFactory )
  : Decoder( componentFactory ), mProgram( 0 ), mCreationOfCustomDataMemberDescriptorsEnabled( true )
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
      case Condition::ConditionTypeAnd:
      case Condition::ConditionTypeOr:
      {
        Condition::Multi * multi = static_cast< Condition::Multi * >( mCurrentComponent );
        multi->addChildCondition( condition );
        mCurrentComponent = condition;
        return true;
      }
      break;
      case Condition::ConditionTypeNot:
      {
        Condition::Not * whoTheHeckDefinedNotToTheExclamationMark = static_cast< Condition::Not * >( mCurrentComponent );
        Condition::Base * currentNotChild = whoTheHeckDefinedNotToTheExclamationMark->childCondition();
        if( currentNotChild )
        {
          // ugly.. the not alreay has a child condition: it should have.
          // we fix this by creating an inner "or"
          whoTheHeckDefinedNotToTheExclamationMark->releaseChildCondition();
          Condition::Or * orCondition = componentFactory()->createOrCondition( mCurrentComponent );
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
    condition = componentFactory()->createAndCondition( mCurrentComponent );
  else if( QString::compare( identifier, QLatin1String( "anyof" ), Qt::CaseInsensitive ) == 0 )
    condition = componentFactory()->createOrCondition( mCurrentComponent );
  else if( QString::compare( identifier, QLatin1String( "not" ), Qt::CaseInsensitive ) == 0 )
    condition = componentFactory()->createNotCondition( mCurrentComponent );
  else if( QString::compare( identifier, QLatin1String( "true" ), Qt::CaseInsensitive ) == 0 )
    condition = componentFactory()->createTrueCondition( mCurrentComponent );
  else if( QString::compare( identifier, QLatin1String( "false" ), Qt::CaseInsensitive ) == 0 )
    condition = componentFactory()->createFalseCondition( mCurrentComponent );
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
    //   testname {:<modifier> {<modifierArgument>}} [:<operator>] {:<modifier> {<modifierArgument>}} [ field_list ] [ value_list ]
    //

    // kill the ugly modifiers we don't support and split the arguments in tagged and not tagged
    // FIXME: we should take care of the non tagged arguments that live "in between" the tagged ones: these should be discarded too!

    QString arg;

    QList< QString > taggedArguments;
    QList< QVariant > nonTaggedArguments;

    QList< QVariant >::Iterator it = mCurrentSimpleTestArguments.begin();
    while( it != mCurrentSimpleTestArguments.end() )
    {
      if( ( *it ).type() != QVariant::String )
      {
        nonTaggedArguments.append( *it );
        ++it;
        continue;
      }

      arg = ( *it ).toString();

      if( arg.length() < 1 )
      {
        nonTaggedArguments.append( *it );
        ++it;
        continue;
      }

      if( arg.at( 0 ) != QChar(':') )
      {
        nonTaggedArguments.append( *it );
        ++it;
        continue; // not tagged
      }

      if( QString::compare( arg, QString::fromAscii(":comparator"), Qt::CaseInsensitive ) == 0 )
      {
        // kill it, and the following argument too
        ++it;
        if( it != mCurrentSimpleTestArguments.end() )
          ++it;
        continue;
      }

      if( QString::compare( arg, QString::fromAscii(":content"), Qt::CaseInsensitive ) == 0 )
      {
        // kill it, and the following argument too
        ++it;
        if( it != mCurrentSimpleTestArguments.end() )
          ++it;
        continue;
      }

      if( QString::compare( arg, QString::fromAscii(":all"), Qt::CaseInsensitive ) == 0 )
      {
        // bleah.. this is the default
        ++it;
        continue;
      }

      if( QString::compare( arg, QString::fromAscii(":raw"), Qt::CaseInsensitive ) == 0 )
      {
        ++it;
        continue;
      }

      if( QString::compare( arg, QString::fromAscii(":text"), Qt::CaseInsensitive ) == 0 )
      {
        ++it;
        continue;
      }

      taggedArguments.append( arg.mid( 1 ) );
      ++it;
    }

    mCurrentSimpleTestArguments.clear();

    // now play with the tagged arguments: the last one should be the operator (if any)

    QVariant argVariant;
    QString operatorKeyword;
    QString functionKeyword = mCurrentSimpleTestName;

    if( taggedArguments.count() > 0 )
    {
      operatorKeyword = taggedArguments.last();
      taggedArguments.removeLast();

      foreach( arg, taggedArguments )
      {
        functionKeyword += QChar(':');
        functionKeyword += arg;
      }
    }

    // lookup the function

    const FunctionDescriptor * function = componentFactory()->findFunction( functionKeyword );
    if ( !function )
    {
      // unrecognized test
      mGotError = true; 
      setLastError( i18n( "Unrecognized function '%1'", functionKeyword ) );
      return;
    }


    if( operatorKeyword.isEmpty() )
    {
      // FIXME: Use a default "forwarding" operator that accepts only boolean results ?
      kDebug() << "OperatorDescriptor keyword is empty in function" << functionKeyword;
    }

    // look up the operator

    const OperatorDescriptor * op = 0;

    op = componentFactory()->findOperator( operatorKeyword, function->outputDataTypeMask() );

    if( !op )
    {
      // unrecognized test
      mGotError = true;
      setLastError( i18n( "Unrecognized operator '%1' for function '%2'", operatorKeyword, functionKeyword ) );
      return;
    }

    // now we may or may not have additional arguments: [field_list] [value_list]
    // if the operator *requires* a value then we *must* have it.
    QVariant values;
    QVariant fields;

    if( op->rightOperandDataType() != DataTypeNone )
    {
      // we expect a right operand (list)
      if( nonTaggedArguments.isEmpty() )
      {
        mGotError = true;
        setLastError( i18n( "The operator '%1' expects a value argument", operatorKeyword ) );
        return;
      }

      values = nonTaggedArguments.last();
      nonTaggedArguments.removeLast();
    } else {
      // we don't expect a right operand
    }

    // now we may or may not have an argument (field).
    // the empty field is considered as "item" (whole item, in most cases)

    if( !nonTaggedArguments.isEmpty() )
      fields = nonTaggedArguments.last();

    QStringList fieldList;

    if ( !fields.isNull() )
    {
      if( !qVariantCanConvert< QStringList >( fields ) )
      {
        mGotError = true;
        setLastError( i18n( "The field name argument must be convertible to a string list" ) );
        return;
      }

      fieldList = fields.toStringList();
    } else {
      // use a single "item" data member (the whole item)
      fieldList.append( "item" );
    }
    

    QList< QVariant > valueList;

    if( values.isNull() )
    {
       valueList.append( values );
    } else {
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
    }

    bool multiTest = false;

    // if there is more than one field or more than one value then we use an "or" test as parent
    if( ( valueList.count() > 1 ) || ( fieldList.count() > 1 ) )
    {
      Condition::Or * orCondition = componentFactory()->createOrCondition( mCurrentComponent );
      Q_ASSERT( orCondition );

      if( !addConditionToCurrentComponent( orCondition, QLatin1String( "anyof" ) ) )
        return; // error already signaled

      multiTest = true;
    }

    int first = true;
    foreach( QString field, fieldList )
    {
      Q_ASSERT( !field.isEmpty() );

      foreach( QVariant value, valueList )
      {
        const DataMemberDescriptor * dataMember = componentFactory()->findDataMember( field );
        if( !dataMember )
        {
          if( ( !mCreationOfCustomDataMemberDescriptorsEnabled ) || ( !( function->acceptableInputDataTypeMask() & DataTypeString ) ) )
          {
            mGotError = true;
            setLastError( i18n( "Test on data member '%1' is not supported.", field ) );
            return;
          }

          QString name = i18n( "the field '%1'", field );
          DataMemberDescriptor * newDataMemberDescriptor = new DataMemberDescriptor( field, name, DataTypeString );
          componentFactory()->registerDataMember( newDataMemberDescriptor );
          dataMember = newDataMemberDescriptor;
        }

        Q_ASSERT( dataMember );

        if( !( dataMember->dataType() & function->acceptableInputDataTypeMask() ) )
        {
          mGotError = true; 
          setLastError( i18n( "Function '%1' can't be applied to data member '%2'.", function->keyword(), field ) );
          return;
        }

        Condition::Base * condition = componentFactory()->createPropertyTestCondition( mCurrentComponent, function, dataMember, op, value );
        if ( !condition )
        {
          // unrecognized test
          mGotError = true; 
          setLastError( i18n( "Test on function '%1' is not supported: %2", field, componentFactory()->lastError() ) );
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
      case Condition::ConditionTypeAnd:
      case Condition::ConditionTypeOr:
      case Condition::ConditionTypeNot:
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
      case Condition::ConditionTypeAnd:
      case Condition::ConditionTypeOr:
      case Condition::ConditionTypeNot:
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


void SieveDecoder::onCommandDescriptorStart( const QString & identifier )
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
      rule = componentFactory()->createRule( mCurrentComponent );
      ruleList->addRule( rule );
      mCurrentComponent = rule;
      return;
    }

    // a rule-list action
    Q_ASSERT( mCurrentComponent->isRule() );

    // create the rule list and add it as action
    rule = static_cast< Rule * >( mCurrentComponent );
    ruleList = componentFactory()->createRuleList( mCurrentComponent );
    rule->addAction( ruleList );

    // create the rule and add it to the rule list we've just created
    rule = componentFactory()->createRule( mCurrentComponent );
    ruleList->addRule( rule );
    mCurrentComponent = rule;

    return;
  }

  // We delay the creation of simple commands to the onCommandDescriptorEnd() so we have all the arguments
  mCurrentSimpleCommandName = identifier;
  mCurrentSimpleCommandArguments.clear();
}

void SieveDecoder::onCommandDescriptorEnd()
{
  if( mGotError )
    return; // ignore everything

  if( !mCurrentSimpleCommandName.isEmpty() )
  {
    // delayed simple command creation

    Q_ASSERT( mCurrentComponent->isRuleList() || mCurrentComponent->isRule() );

    Action::Base * action;
    if(
         ( QString::compare( mCurrentSimpleCommandName, QLatin1String("stop" ), Qt::CaseInsensitive ) == 0 ) ||
         ( QString::compare( mCurrentSimpleCommandName, QLatin1String("keep" ), Qt::CaseInsensitive ) == 0 )
      )
    {
      action = componentFactory()->createStopAction( mCurrentComponent );
      Q_ASSERT( action );

    } else {
      // a "command" action

      const CommandDescriptor * command = componentFactory()->findCommand( mCurrentSimpleCommandName );
      if( !command )
      {
        mGotError = true; 
        setLastError( i18n( "Unrecognized action '%1'", mCurrentSimpleCommandName ) );
        return;
      }

      if( mCurrentSimpleCommandArguments.count() < command->parameters()->count() )
      {
        mGotError = true; 
        setLastError( i18n( "Action '%1' required at least %2 arguments", mCurrentSimpleCommandName, command->parameters()->count() ) );
        return;
      }

      action = componentFactory()->createCommandAction( mCurrentComponent, command, mCurrentSimpleCommandArguments );
      if( !action )
      {
        mGotError = true; 
        setLastError( i18n( "Could not create action '%1': %2", mCurrentSimpleCommandName, componentFactory()->lastError() ) );
        return;
      }

    }

    Q_ASSERT( action );
    mCurrentSimpleCommandName = QString();

    if ( mCurrentComponent->isRuleList() )
    {
      // unconditional rule start with an action
      Action::RuleList * ruleList = static_cast< Action::RuleList * >( mCurrentComponent );
      Rule * rule = componentFactory()->createRule( mCurrentComponent );
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
    setLastError( i18n( "Unexpected start of block outside of a rule." ) );
    return;
  }
}

void SieveDecoder::onBlockEnd()
{
  if( mGotError )
    return; // ignore everything

}

Program * SieveDecoder::run( const QString &source )
{
  QByteArray utf8Script = source.toUtf8();

  KSieve::Parser parser( utf8Script.data(), utf8Script.data() + utf8Script.length() );

  Private::SieveReader sieveReader( this );

  parser.setScriptBuilder( &sieveReader );

  if( mProgram )
    delete mProgram;

  mProgram = componentFactory()->createProgram();
  Q_ASSERT( mProgram ); // component componentfactory shall never fail creating a Program

  mGotError = false;
  mCurrentComponent = mProgram;

  if( !parser.parse() )
  {
    // force an error
    if( !mGotError )
    {
      mGotError = true;
      setLastError( i18n( "Internal sieve library error" ) );
    }
  }

#ifdef DEBUG_SIEVE_DECODER
  qDebug( "\nDECODING SIEVE SCRIPT\n" );
  qDebug( "%s", source.toUtf8().data() );
  qDebug( "\nDECODED TREE\n" );
  if ( mProgram )
  {
    QString prefix;
    mProgram->dump( prefix );
  }
  qDebug( "\n" );
#endif //DEBUG_SIEVE_DECODER

  if( mGotError )
  {
    qDebug( "DECODING ERROR: %s", lastError().toUtf8().data() );
    return 0;
  }

  Program * prog = mProgram;
  mProgram = 0;
  return prog;
}


} // namespace IO

} // namespace Filter

} // namespace Akonadi

