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

#include "config-akonadi-filter.h"

#include <QString>
#include <QList>

#include "component.h"
#include "action.h"

namespace Akonadi
{
namespace Filter
{

class Data;

namespace Condition
{
  class Base;
} // namespace Condition

class AKONADI_FILTER_EXPORT Rule : public Component
{
public:
  Rule( Component * parent );
  virtual ~Rule();
protected:

  /**
   * The condition attached to this rule. Owned pointer. May be null (null condition is always true)
   */
  Condition::Base * mCondition;

  QList< Action::Base * > mActionList;

public:

  Condition::Base * condition() const
  {
    return mCondition;
  }

  void setCondition( Condition::Base * condition );

  virtual ProcessingStatus execute( Data * data );
};

} // namespace Filter

} // namespace Akonadi

#endif //!_AKONADI_FILTER_RULE_H_
