/****************************************************************************** *
 *
 *  File : sievedecoder.h
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

#ifndef _AKONADI_FILTER_IO_SIEVEDECODER_H_
#define _AKONADI_FILTER_IO_SIEVEDECODER_H_

#include <akonadi/filter/config-akonadi-filter.h>

#include <akonadi/filter/io/decoder.h>

#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QList>
#include <QtCore/QHash>
#include <QtCore/QVariant>

namespace Akonadi
{
namespace Filter
{

class Program;
class Component;
class Rule;

namespace Condition
{
  class Base;
}

namespace IO
{

namespace Private
{
  class SieveReader;
} // namespace Private

/**
 * A sieve decoder for filtering programs.
 *
 * This decoder supports a large subset of constructs reccomended by RFC 5228
 * but not all of them. Some exotic filters can't be parsed due to their
 * ambiguous nature or ugly required implementation. The decoder, also,
 * needs a ComponentFactory able to decode the commands used in the script.
 * For instance: "keep" is a command of the sieve specification which
 * must be registered with the ComponentFactory in order for this class to handle it.
 *
 * The SieveDecoder also supports some extensions to the Sieve language
 * which aren't described by any RFC. This is needed in order to be able
 * to store all the extended properties specific to Akonadi filtering.
 *
 * The filtering framework contains a corresponding encoder class (see SieveEncoder).
 * This decoder is able to read all the filters encoded by SieveEncoder.
 */
class AKONADI_FILTER_EXPORT SieveDecoder : public Decoder
{
  friend class Private::SieveReader;
public:

  /**
   * Create a sieve decoder associated to the specified ComponentFactory.
   */
  SieveDecoder( ComponentFactory * componentFactory );

  /**
   * Destroy the sieve decoder and all the associated resrouces.
   */
  virtual ~SieveDecoder();

protected:

  /**
   * The source that this decoder is operating on.
   */
  QString mSource;

  /**
   * The currently parsed Program. Owned pointer (until released).
   * May be null.
   */
  Program * mProgram;

  /**
   * The currently parsed Program component. This is usually
   * a shallow pointer to an object owned by the Program above.
   */
  Component * mCurrentComponent;

  /**
   * The name of the current "simple test" we're parsing.
   */
  QString mCurrentSimpleTestName;

  /**
   * The arguments of the current "simple test" we're parsing
   */
  QList< QVariant > mCurrentSimpleTestArguments;

  /**
   * The name of the current "simple command" we're parsing.
   */
  QString mCurrentSimpleCommandName;

  /**
   * The arguments of the current "simple command" we're parsing
   */
  QList< QVariant > mCurrentSimpleCommandArguments;

  /**
   * The currently parsed string list (parameters)
   */
  QStringList mCurrentStringList;

  /**
   * True if we encountered a decoding error, false otherwise.
   */
  bool mGotError;

  /**
   * True if we're allowed to create "custom" data members
   * which aren't described by the associated ComponentFactory.
   */
  bool mCreationOfCustomDataMemberDescriptorsEnabled;

  /**
   * The hash of the properties that we'll associate to the
   * currently parsed rule. This is an extension to sieve.
   */
  QHash< QString, QVariant > mCurrentRuleProperties;

  /**
   * Keeps track of the current sieve source line number.
   */
  int mLineNumber;

public:

  /**
   * Enables or disables automatic creation of data members
   * not explicitly described by the ComponentFactory we're attached to.
   */
  void setSreationOfCustomDataMemberDescriptorsEnabled( bool enable )
  {
    mCreationOfCustomDataMemberDescriptorsEnabled = enable;
  }

  /**
   * Returns true if the automatic creation of data members
   * is enabled and false otherwise.
   */
  bool creationOfCustomDataMemberDescriptorsEnabled()
  {
    return mCreationOfCustomDataMemberDescriptorsEnabled;
  }

  /**
   * Reimplemented from Decoder.
   *
   * This is the main decoding routine. You pass it an UTF8 encodedFilter
   * and get in return a complete filtering Program tree or 0 in case
   * of error. In the latter case you can use the errorStack() to
   * obtain a description of the error.
   */
  virtual Program * run( const QByteArray &encodedFilter );

protected:

  // Interface exposed only to Private::SieveReader

  void onCommandDescriptorStart( const QString & identifier );
  void onCommandDescriptorEnd();
  void onTestStart( const QString & identifier );
  void onTestEnd();
  void onTestListStart();
  void onTestListEnd();
  void onError( const QString &error );
  void onBlockStart();
  void onBlockEnd();
  void onTaggedArgument( const QString & tag );
  void onStringArgument( const QString & string, bool multiLine, const QString & embeddedHashComment );
  void onNumberArgument( unsigned long number, char quantifier );
  void onStringListArgumentStart();
  void onStringListEntry( const QString & string, bool multiLine, const QString & embeddedHashComment );
  void onStringListArgumentEnd();
  void onComment( const QString &comment );
  void onLineFeed();

private:

  // Private stuff: don't look

  void pushError( const QString &error );
  Rule * createRule( Component * parentComponent );
  bool addConditionToCurrentComponent( Condition::Base * condition, const QString &identifier );

}; // class SieveDecoder

} // namespace IO

} // namespace Filter

} // namespace Akonadi

#endif //!_AKONADI_FILTER_IO_DECODER_H_
