/****************************************************************************** *
 *
 *  File : condition.h
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

#ifndef _AKONADI_FILTER_CONDITION_H_
#define _AKONADI_FILTER_CONDITION_H_

#include "config-akonadi-filter.h"

#include "component.h"

#include <QList>

namespace Akonadi
{
namespace Filter
{

class Data;

namespace Condition
{

class AKONADI_FILTER_EXPORT Base : public Component
{
public:
  enum ConditionType
  {
    ConditionTypeAnd,
    ConditionTypeOr
  };
public:
  Base( ConditionType type, Component * parent );
  virtual ~Base();
protected:
  ConditionType mConditionType;
public:
  ConditionType conditionType() const
  {
    return mConditionType;
  }
  virtual bool matches( Data * data );
}; // class Base

class AKONADI_FILTER_EXPORT Multi : public Base
{
public:
  Multi( ConditionType type, Component * parent );
  ~Multi();
protected:
  QList< Condition::Base * > mChildConditions;
public:
  void addChildCondition( Condition::Base * condition )
  {
    mChildConditions.append( condition );
  }
};

class AKONADI_FILTER_EXPORT And : public Multi
{
public:
  And( Component * parent );
  virtual ~And();
public:
  virtual bool matches( Data * data );
}; // class And

class AKONADI_FILTER_EXPORT Or : public Multi
{
public:
  Or( Component * parent );
  virtual ~Or();
public:
  virtual bool matches( Data * data );
}; // class Or

} // namespace Condition

} // namespace Filter

} // namespace Akonadi

#endif //!_AKONADI_FILTER_CONDITION_H_
