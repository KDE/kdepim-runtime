/****************************************************************************** *
 *
 *  File : rulelist.h
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

#ifndef _AKONADI_FILTER_RULELIST_H_
#define _AKONADI_FILTER_RULELIST_H_

#include <akonadi/filter/config-akonadi-filter.h>

#include <akonadi/filter/component.h>
#include <akonadi/filter/rule.h>
#include <akonadi/filter/action.h>

#include <QtCore/QList>

namespace Akonadi
{
namespace Filter
{

class Data;

namespace Action
{

/**
 * @class Akonadi::Filter::Action::RuleList
 * @brief A list of Rule objects to be applied in sequence.
 *
 * The RuleList is either meant to be a toplevel object (see @ref Program)
 * or a child Action of a Rule. This allows infinite nesting in the tree
 * and thus filtering programs trees of arbitrary complexity.
 *
 * To execute a RuleList you usually throw a Data object into the
 * execute() method which will apply each contained Rule in turn
 * until it runs out of rules, it encounters an error or one of the
 * action returns SuccessAndStop.
 */
class AKONADI_FILTER_EXPORT RuleList : public Base
{
public:
  /**
   * Create a rule with the specified parent Component.
   * The parent will be usually a Rule instance.
   */
  RuleList( Component * parent );

  /**
   * Destroy the Rule object and all the children.
   */
  ~RuleList();

protected:

  /**
   * The list of rules this filtering sub-program is made of.
   * The Rule objects are owned.
   */
  QList< Rule * > mRuleList;

public:

  /**
   * Returns the list of rules that this program is composed of.
   * The returned pointer is never null.
   */
  const QList< Rule * > * ruleList() const
  {
    return &mRuleList;
  }

  /**
   * Remove and delete all the rules contained inside the rule list.
   */
  void clear();

  /**
   * Add a rule to this RuleList. The ownership of the Rule passes
   * to this object.
   *
   * @param rule The Rule object to add. Must not be null.
   */
  void addRule( Rule * rule )
  {
    mRuleList.append( rule );
  }

  /**
   * Runs this filtering rule list on the specified filtering Data set.
   * Returns SuccessAndStop if the execution was successful and was
   * explicitly stopped by an inner rule. Returns SuccessAndContinue
   * if the execution was successful and it wasn't stopped explicitly
   * by any inner rule. Returns Failure in case of a processing error:
   * lastError() can be used to obtain more informations.
   *
   * If you override this method then you should be prepared to handle
   * multiple execute() calls one after another with different instances of data:
   * the RuleList objects are intended to be reusable.
   */
  virtual ProcessingStatus execute( Data * data );

  /**
   * Reimplemented from Component: returns true.
   */
  virtual bool isRuleList() const;

  /**
   * Reimplemented from Action: returns false. The rule list *COULD* actually
   * contain a terminal leaf that is reached unconditionally
   * but this is a particular case that is hard to detect with
   * a simple code. If you really need to decide if a rule list is
   * terminal then drop me a mail at s dot stefanek at gmail dot com
   * and I'll see what can be done about it :D
   */
  virtual bool isTerminal() const;

  /**
   * Debugging aid.
   */
  virtual void dumpRuleList( const QString &prefix );

  /**
   * Reimplemented from Component. Debugging aid.
   */
  virtual void dump( const QString &prefix );

}; // class RuleList

} // namespace Action

} // namespace Filter

} // namespace Akonadi

#endif //!_AKONADI_FILTER_RULELIST_H_
