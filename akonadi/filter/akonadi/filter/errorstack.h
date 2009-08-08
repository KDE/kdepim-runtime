/****************************************************************************** *
 *
 *  File : errorstack.h
 *  Created on Thu 06 Aug 2009 23:37:16 by Szymon Tomasz Stefanek
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

#ifndef _AKONADI_FILTER_ERRORSTACK_H_
#define _AKONADI_FILTER_ERRORSTACK_H_

#include <akonadi/filter/config-akonadi-filter.h>

#include <QtCore/QList>
#include <QtCore/QPair>
#include <QtCore/QString>

namespace Akonadi
{
namespace Filter
{

/**
 * A stack of errors. Each error has a location and a description.
 * You push errors one after another and then format a nice
 * "backtrace-style" message to show to the user.
 *
 * This is useful in diagnosing problems in execution of filters
 * and as a "side effect" used in other parts of the filtering framework.
 *
 * This class is meant to be either inherited (and this is why
 * the methods have more verbose names than expected) or contained.
 */
class AKONADI_FILTER_EXPORT ErrorStack
{
public:

  /**
   * Creates an empty ErrorStack.
   */
  ErrorStack();

  /**
   * Destroy the ErrorStack and all the erroneous errors it contains.
   */
  virtual ~ErrorStack();

private:

  /**
   * The stack of errors. The last error is the top of the stack.
   * The first element in the pair contains the location of the error,
   * the second element contains the description.
   */
  QList< QPair< QString, QString > > mErrorList;

public:

  /**
   * Returns the internal list of error contained in this ErrorStack object.
   * The last error is the top of the stack.
   * The first element in the pair contains the location of the error,
   * the second element contains the description.
   */
  const QList< QPair< QString, QString > > & errors() const
  {
    return mErrorList;
  }

  /**
   * Clears the whole error stack.
   */
  void clearErrors();

  /**
   * Returns true if this error stack contains at least one error.
   */
  bool hasErrors() const
  {
    return mErrorList.count() > 0;
  }

  /**
   * Pushes an error on top of the stack. Location can be an empty string,
   * if unknown/unspecified. The description shouldn't be empty and it should
   * be localized.
   */
  void pushError( const QString &description, const QString &location = QString() );

  /**
   * Pushes a whole error stack on top of this stack.
   */
  void pushErrorStack( const ErrorStack &stack );

  /**
   * Returns a nicely formatted plain text error message to be displayed to the user.
   * The message spans multiple lines (separated by \n) and may contain
   * some indentation.
   *
   * If topError is not an empty string then it's used as the "topmost",
   * most generalized error message. This will be usually something like
   * "Filter Execution Failed" (and the detailed stack follows).
   */
  QString errorMessage( const QString &topError = QString() ) const;

  /**
   * Returns a nicely formatted html error message to be displayed to the user
   * (possibly via message box).
   * The message spans multiple lines (separated by <br>) and may contain
   * additional html formatting (bold or undelined stuff).
   *
   * If topError is not an empty string then it's used as the "topmost",
   * most generalized error message. This will be usually something like
   * "Filter Execution Failed" (and the detailed stack follows).
   */
  QString htmlErrorMessage( const QString &topError = QString() ) const;

  /**
   * Dumps the errorMessage() to the console. Useful for debugging.
   */
  void dumpErrorMessage( const QString &topError = QString() ) const;

}; // class ErrorStack

} // namespace Filter

} // namespace Akonadi

#endif //!_AKONADI_FILTER_ERRORSTACK_H_
