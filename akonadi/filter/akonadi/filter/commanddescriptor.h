/****************************************************************************** *
 *
 *  File : commanddescriptor.h
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

#ifndef _AKONADI_FILTER_COMMANDDESCRIPTOR_H_
#define _AKONADI_FILTER_COMMANDDESCRIPTOR_H_

#include <akonadi/filter/config-akonadi-filter.h>

#include <akonadi/filter/datatype.h>

#include <QtCore/QString>
#include <QtCore/QList>

namespace Akonadi
{
namespace Filter
{

/**
 * The "well-known" command identifiers used in the CommandDescriptor class.
 * Custom commands should use identifiers starting at CommandCustomFirst.
 */
enum CommandIdentifiers
{
  /**
   * The standard "Leave Message on Server" command.
   * In Sieve this is encoded as "keep".
   */
  StandardCommandLeaveMessageOnServer = 1,

  /**
   * The standard "Move Item to Collection" command.
   */
  StandardCommandMoveItemToCollection = 2,

  /**
   * The base identifier for user defined commands.
   */
  CommandCustomFirst = 10000
};


/**
 * This class describes a ComponentFactory-customizable action.
 *
 * Your agent / client-side implementation may register several
 * CommandDescriptor objects with the ComponentFactory.
 *
 * The ComponentFactory will instantiate an Action::Command
 * class which will handle the operation automagically (in the filtering agent).
 *
 * The I/O routines (see IO::Encoder and IO::Decoder) will also
 * use this class to encode/decode the action to/from the storage format.
 */
class AKONADI_FILTER_EXPORT CommandDescriptor
{
public:

  /**
   * A single formal parameter for the command described by CommandDescriptor.
   *
   * The parameter has a name (which is used as command token in the storage)
   * and a data type.
   */
  class ParameterDescriptor
  {
  protected:

    /**
     * The name of this parameter
     */
    QString mName;

    /**
     * The data type of this parameter.
     */
    DataType mDataType;

  public:

    /**
     * Create an instance of the ParameterDescriptor
     */
    ParameterDescriptor( DataType dataType, const QString &name )
      : mName( name ), mDataType( dataType )
    {
    }

  public:

    /**
     * Returns the name associated to this parameter
     */
    const QString & name() const
    {
      return mName;
    }

    /**
     * Returns the expected data type of this parameter.
     */
    DataType dataType() const
    {
      return mDataType;
    }
  }; // class ParameterDescriptor
public:

  /**
   * Create a command descriptor with the specified keyword
   */
  CommandDescriptor(
      int id,                          //< The id of the command: it should be unique within an application
      const QString &keyword,          //< Unique command keyword: it matches the keyword used in Sieve scripts.
      const QString &name,             //< The token that is displayed in the UI editors.
      bool isTerminal = false          //< Is this command terminal ? Terminal commands stop the script upon succesfull execution.
    );

  /**
   * Destroy a CommandDescriptor object.
   */
  virtual ~CommandDescriptor();

protected:

  /**
   * The unique id of the commanddescriptor.
   */
  int mId;

  /**
   * The non-localized keywordentifier of the command.
   */
  QString mKeyword;

  /**
   * The localized name of the command (this is what is shown in rule selection combos)
   */
  QString mName;

  /**
   * The list of parameter descriptors for this command. Owned pointers.
   */
  QList< ParameterDescriptor * > mParameters;

  /**
   * Is this command a terminal one (that is.. does it stop processing upon successfull execution ?)
   */
  bool mIsTerminal;

public:

  /**
   * Returns the unique identifier associated to this command action.
   */
  int id() const
  {
    return mId;
  }

  /**
   * Returns the non-localized keyword for this command action.
   */
  const QString & keyword() const
  {
    return mKeyword;
  }

  /**
   * Returns the localized name for this command action.
   */
  const QString & name() const
  {
    return mName;
  }

  /**
   * Returns true if this action is terminal and terminates the filtering
   * script execution upon succesfull execution. Returns false otherwise.
   */
  bool isTerminal() const
  {
    return mIsTerminal;
  }

  /**
   * Returns the list of parameter descriptors associated with this command.
   */
  const QList< ParameterDescriptor * > * parameters() const
  {
    return &mParameters;
  }

  /**
   * Adds a single parameter to this command. The ownership of the pointer
   * is taken by this class.
   */
  void addParameter( ParameterDescriptor * parameter )
  {
    mParameters.append( parameter );
  }

}; // class CommandDescriptor

} // namespace Filter

} // namespace Akonadi

#endif //!_AKONADI_FILTER_COMMANDDESCRIPTOR_H_
