/****************************************************************************** *
 *
 *  File : action.h
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

#ifndef _AKONADI_FILTER_ACTION_H_
#define _AKONADI_FILTER_ACTION_H_

#include <akonadi/filter/config-akonadi-filter.h>

#include <akonadi/filter/component.h>

#include <QtCore/QList>
#include <QtCore/QVariant>

namespace Akonadi
{
namespace Filter
{

class Data;
class CommandDescriptor;

/**
 * Each rule inside a filter has a list of associated actions which are executed
 * in sequence.
 *
 * Some of the actions are "builtin" while others are provided
 * by the context-specific implementation. At the time of writing the builtins
 * are the Stop action (which succesfully terminates the script) and
 * the RuleList action which jumps inside a filtering "sub-program".
 */
namespace Action
{

/**
 * The action type.
 *
 * This, in fact, is a "rough" classification: the "commands", for instance,
 * can be of several types. However, this is enough for several components to make
 * their choices about the methods of creation and editing of the Action objects.
 */
enum ActionType
{
  /**
   * The action is a simple stop action: it immediately and succesfully terminates
   * the processing in the filter.
   */
  ActionTypeStop,

  /**
   * The action is a sub-sequence of rules (just like a nested sub-filter).
   * The sub-sequence will have it's own conditions and actions.
   */
  ActionTypeRuleList,

  /**
   * The action is a command.
   */
  ActionTypeCommand,

  /**
   * This member indicates "no action" or "unknown action".
   * It is used in the UI editors.
   */
  ActionTypeUnknown

}; // enum ActionType

/**
 * The base class of all the actions that can be executed by the filters.
 * It contains some pure virtuals which must be implemented by the subclasses.
 */
class AKONADI_FILTER_EXPORT Base : public Component
{
public:
  /**
   * Creates a Base action object with the specified type
   * and the specified parent component (which is usually a Rule).
   */
  Base( ActionType type, Component * parent );

  /**
   * Destroys a Base object. This implementation does nothing.
   */
  virtual ~Base();

protected:
  /**
   * The action type this object belongs to.
   */
  ActionType mActionType;

public:
  /**
   * Returns the action type
   */
  ActionType actionType() const
  {
    return mActionType;
  }

  /**
   * Reimplemented from Component to return true.
   */
  virtual bool isAction() const;

  /**
   * Returns true if this action is terminal (that is it stops processing) or false otherwise.
   */
  virtual bool isTerminal() const = 0;

  /**
   * Executes the action on the specified data object
   * and returs a ProcessingStatus (which is either SuccessAndContinue,
   * SuccessAndStop or Failure).
   *
   * This is a pure virtual that must be implemented by the subclasses.
   */
  virtual ProcessingStatus execute( Data * data ) = 0;

  /**
   * Used to dump the action on the console (debugging).
   * Reimplemented from Component.
   */
  virtual void dump( const QString &prefix );

}; // class Base

/**
 * A standard "succesfull stop" action.
 *
 * The execution of this action simply terminates the filtering script
 * with the result SuccessAndStop.
 *
 * In sieve scripts this corresponds to the "keep" keyword.
 */
class AKONADI_FILTER_EXPORT Stop : public Base
{
public:
  /**
   * Create an instance of the Stop action with the specified parent component
   * (which is usually a Rule).
   */
  Stop( Component * parent );

  /**
   * Destroys the Stop action.
   */
  virtual ~Stop();

public:

  /**
   * Returns true.
   */
  virtual bool isTerminal() const;

  /**
   * Executes the Stop action. This function simply returns SuccessAndStop
   * which will be propagated to the upper execution levels.
   */
  virtual ProcessingStatus execute( Data * data );

  /**
   * Used to dump the action on the console (debugging).
   * Reimplemented from Base.
   */
  virtual void dump( const QString &prefix );

}; // class Stop

/**
 * A standard factory-customizable command action.
 *
 * This is the primary mean to implement actions on the agent and IO side.
 *
 * This class implements a command based on the CommandDescriptor class
 * which is stored in the ComponentFactory object. You can register
 * several CommandDescriptor objects: all of the will be "automagically"
 * handled by this class.
 */
class AKONADI_FILTER_EXPORT Command : public Base
{
public:

  /**
   * Create an instance of the Command action with the specified
   * parent Component, the specified CommandDescriptor and the
   * specified list of actual parameters. The parameters are stored as variants
   * and must correspond to the list of formal parameters described in the
   * CommandDescriptor object. The parent and command parameters must not be NULL.
   *
   * This class is usually instantiated via the I/O classes (see IO::Decoder).
   */
  Command( Component * parent, const CommandDescriptor * command, const QList< QVariant > &params );

  /**
   * Destroys the command action and any data associated with it.
   */
  virtual ~Command();

protected:

  /**
   * The descriptor of this command. This is a shallow pointer:
   * the ownership is of the caller (read: the ComponentFactory)
   * which must ensure the validity for the entire lifetime of the object.
   */
  const CommandDescriptor * mCommandDescriptor;

  /**
   * The list of actual parameters for this command.
   */
  QList< QVariant > mParams;

public:

  /**
   * Returns the CommandDescriptor object associated with this Command object.
   * This is never NULL.
   */
  const CommandDescriptor * commandDescriptor() const
  {
    return mCommandDescriptor;
  }

  /**
   * Returns true if this command is terminal (that is it stops processing) or false otherwise.
   * This depends on the CommandDescriptor.
   */
  virtual bool isTerminal() const;

  /**
   * Returns the list of actual parameters associated with this command.
   */
  const QList< QVariant > * parameters() const
  {
    return &mParams;
  }

  /**
   * Executes this command on the specified Data set
   * and returns the result of the processing.
   *
   * Reimplemented from Action::Base.
   */
  virtual ProcessingStatus execute( Data * data );

  /**
   * This is mainly used for debug purposes: dumps the
   * action to the console. The prefix string is inserted
   * at the beginning of each output line.
   */
  virtual void dump( const QString &prefix );

}; // class Command

} // namespace Action

} // namespace Filter

} // namespace Akonadi

#endif //!_AKONADI_FILTER_ACTION_H_
