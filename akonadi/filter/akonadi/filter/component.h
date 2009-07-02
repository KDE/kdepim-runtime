/****************************************************************************** *
 *
 *  File : component.h
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

#ifndef _AKONADI_FILTER_COMPONENT_H_
#define _AKONADI_FILTER_COMPONENT_H_

#include <akonadi/filter/config-akonadi-filter.h>

#include <QtCore/QString>

namespace Akonadi
{
namespace Filter
{

/**
 * The base class of the nodes of a filtering tree.
 */
class AKONADI_FILTER_EXPORT Component
{
public:
  /**
   * These status codes are returned by the filter execution routines.
   */
  enum ProcessingStatus
  {
    /**
     * The filter node execution completed succesfully. Continue
     * executing the script.
     */
    SuccessAndContinue,

    /**
     * The filter node execution completed succesfully. Stop
     * the script execution.
     */
    SuccessAndStop,

    /**
     * The filter node execution failed. More informations
     * might be available via Component::lastError()
     */
    Failure
  };

public:

  /**
   * Create an instance of the Component with the specified parent.
   * The parent Component may be null.
   */
  Component( Component * parent );

  /**
   * Destroy the component and any associated resources.
   */
  virtual ~Component();

protected:

  /**
   * The last error occured in the execution of this Component methods.
   */
  QString mLastError;

  /**
   * The parent component: shallow, may be null.
   */
  Component * mParent;
public:

  /**
   * Returns the pointer to the parent Component or NULL if
   * this Component has no parent.
   */
  Component * parent() const
  {
    return mParent;
  }

  /**
   * This is reimplemented to return true by the Program components.
   * The default implementation returns false.
   */
  virtual bool isProgram() const;

  /**
   * This is reimplemented to return true by the Rule and derived components.
   * The default implementation returns false.
   */
  virtual bool isRule() const;

  /**
   * This is reimplemented to return true by the Action::Base and derived components.
   * The default implementation returns false.
   */
  virtual bool isAction() const;

  /**
   * This is reimplemented to return true by the Condition::Base and derived components.
   * The default implementation returns false.
   */
  virtual bool isCondition() const;

  /**
   * This is reimplemented to return true by the Action::RuleList and derived components.
   * The default implementation returns false.
   */
  virtual bool isRuleList() const;

  /**
   * Returns the last error occured in this component execution run.
   */
  const QString & lastError() const
  {
    return mLastError;
  }

  /**
   * This is mainly used for debug purposes: dumps the
   * component contents to the console. The prefix string is inserted
   * at the beginning of each output line.
   */
  virtual void dump( const QString &prefix );

protected:

  /**
   * Convenience function for the dump() routine reimplementation.
   * Subclasses should use this to print their output on the console.
   */
  void debugOutput( const QString &prefix, const char *text );

  /**
   * Convenience function for the dump() routine reimplementation.
   * Subclasses should use this to print their output on the console.
   */
  void debugOutput( const QString &prefix, const QString &text );

  /**
   * Sets the last error occured in this component execution run.
   * Subclasses should use this method to provide mode information
   * about an execution failure.
   */
  void setLastError( const QString &error )
  {
    mLastError = error;
  }

}; // class Component

} // namespace Filter

} // namespace Akonadi

#endif //!_AKONADI_FILTER_COMPONENT_H_
