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

class AKONADI_FILTER_EXPORT RuleList : public Base
{
public:
  RuleList( Component * parent );
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

  void clear()
  {
    mRuleList.clear();
  }

  void addRule( Rule * rule )
  {
    mRuleList.append( rule );
  }

  virtual bool isRuleList() const;

  /**
   * Returns false. The rule list *COULD* actually
   * contain a terminal leaf that is reached unconditionally
   * but this is a particular case that is hard to detect with
   * a simple code. If you really need to decide if a rule list is
   * terminal then drop me a mail at s dot stefanek at gmail dot com
   * and I'll see what can be done about it :D
   */
  virtual bool isTerminal() const;

  /**
   * Runs this filtering rule list on the specified filtering Data set.
   * Returns SuccessAndStop if the execution was succesfull and was
   * explicitly stopped by an inner rule. Returns SuccessAndContinue
   * if the execution was succesfull and it wasn't stopped explicitly
   * by any inner rule. Returns Failure in case of a processing error:
   * lastError() can be used to obtain more informations.
   */
  virtual ProcessingStatus execute( Data * data );

  virtual void dumpRuleList( const QString &prefix );
  virtual void dump( const QString &prefix );
};

} // namespace Action

} // namespace Filter

} // namespace Akonadi

#endif //!_AKONADI_FILTER_RULELIST_H_
