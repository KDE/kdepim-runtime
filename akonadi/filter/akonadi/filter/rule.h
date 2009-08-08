/****************************************************************************** *
 *
 *  File : rule.h
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

#ifndef _AKONADI_FILTER_RULE_H_
#define _AKONADI_FILTER_RULE_H_

#include <akonadi/filter/config-akonadi-filter.h>

#include <QtCore/QString>
#include <QtCore/QList>

#include <akonadi/filter/action.h>
#include <akonadi/filter/component.h>
#include <akonadi/filter/propertybag.h>

namespace Akonadi
{
namespace Filter
{

class Data;

namespace Condition
{
  class Base;
} // namespace Condition

/**
 * A single rule in the filtering program.
 *
 * A rule is made of a condition and a set of actions.
 * If the condition matches then the actions are executed in sequence.
 */
class AKONADI_FILTER_EXPORT Rule : public Component, public PropertyBag
{
public:

  /**
   * Creates a rule with the specified parent as component.
   * The parent will be usually a RuleList.
   */
  Rule( Component * parent );

  /**
   * Destroys the Rule object and all the objects contained inside.
   */
  virtual ~Rule();

protected:

  /**
   * The condition attached to this rule. Owned pointer. May be null (null condition is always true)
   */
  Condition::Base * mCondition;

  /**
   * The list of actions attachhed to this rule, owned pointers. May be empty.
   */
  QList< Action::Base * > mActionList;

public:

  /**
   * Returns the description of this program.
   * This is equivalent to property( QString::fromAscii( "description" ) ).
   */
  QString description() const;

  /**
   * Sets the user supplied name of this filtering program.
   * This is equivalent to setProperty( QString::fromAscii( "description" ), description ).
   */
  void setDescription( const QString &description );

  /**
   * Returns the Condition object stored in this rule.
   * The returned pointer is owned by this class and may
   * be null. A null condition is equivalent to a true condition
   * (so it always matches).
   */
  Condition::Base * condition() const
  {
    return mCondition;
  }

  /**
   * Sets the condition for this rule replacing any
   * previously existing one. The pointer ownership
   * passes to this class. The condition parameter may be null
   * and in that case the condition is assumed to be always true.
   */
  void setCondition( Condition::Base * condition );

  /**
   * Returns the list of actions associated to this rule.
   * The returned pointer is owned by this class and is never null
   * (though the list of actions may be empty)
   */
  QList< Action::Base * > * actionList()
  {
    return &mActionList;
  }

  /**
   * Clears the list of actions associated to this Rule.
   */
  void clearActionList()
  {
    mActionList.clear();
  }

  /**
   * Appends an action to the list of actions belonging to this Rule.
   * The pointer ownership passes to this class. The action parameter
   * must not be null.
   */
  void addAction( Action::Base * action )
  {
    mActionList.append( action );
  }

  /**
   * Returns true if this Rule is "empty" and false otherwise.
   * an empty rule has a null condition and an empty list of actions.
   */
  bool isEmpty() const;

  /**
   * Executes this rule on the specified Data set and returns the execution status result.
   * This function first calls matches() on the underlying condition. If matches()
   * returns false then the function returns SuccessAndContinue.
   * If matches() returns true then the list of actions is executed() in sequence
   * and the ProcessingStatus result is propagated from that.
   * You shouldn't need to override this function unless you want to implement
   * a very strange filter.
   *
   * If the execution of the rule fails for some reason then a description
   * of the error should be available via errorStack().
   */
  virtual ProcessingStatus execute( Data * data );

  /**
   * Reimplemented from Component to return true.
   */
  virtual bool isRule() const;

  /**
   * Dumps the contents of the property to the console: this is a debugging aid.
   */
  virtual void dump( const QString &prefix );

};

} // namespace Filter

} // namespace Akonadi

#endif //!_AKONADI_FILTER_RULE_H_
