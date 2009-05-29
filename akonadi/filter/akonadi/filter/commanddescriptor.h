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

#include "config-akonadi-filter.h"

#include "datatype.h"

#include <QString>
#include <QList>

namespace Akonadi
{
namespace Filter
{

enum CommandIdentifiers
{
  // standard commands
  StandardCommandLeaveMessageOnServer = 1,
  // custom commands
  CommandCustomFirst = 10000
};


/**
 * 
 *
 */
class AKONADI_FILTER_EXPORT CommandDescriptor
{
public:
  class ParameterDescriptor
  {
  protected:
    QString mName;
    DataType mDataType;
    bool mIsTerminal;
  public:
    ParameterDescriptor( DataType dataType, const QString &name )
      : mName( name ), mDataType( dataType )
    {
    }
  public:

    const QString & name() const
    {
      return mName;
    }
    DataType dataType() const
    {
      return mDataType;
    }
  };
public:
  /**
   * Create a command descriptor with the specified keyword
   *
   */
  CommandDescriptor(
      int id,                          //< The id of the command: it should be unique within an application
      const QString &keyword,          //< Unique command keyword: it matches the keyword used in Sieve scripts.
      const QString &name,             //< The token that is displayed in the UI editors.
      bool isTerminal = false          //< Is this command terminal ?
    );
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
   * Is this command a terminal one (that is.. does it stop processing with success ?)
   */
  bool mIsTerminal;

public:

  int id() const
  {
    return mId;
  }

  const QString & keyword() const
  {
    return mKeyword;
  }

  const QString & name() const
  {
    return mName;
  }

  bool isTerminal() const
  {
    return mIsTerminal;
  }

  const QList< ParameterDescriptor * > * parameters() const
  {
    return &mParameters;
  }

  void addParameter( ParameterDescriptor * parameter )
  {
    mParameters.append( parameter );
  }

}; // class CommandDescriptor

} // namespace Filter

} // namespace Akonadi

#endif //!_AKONADI_FILTER_COMMANDDESCRIPTOR_H_
