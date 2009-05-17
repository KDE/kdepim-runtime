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
#include <QVariant>

namespace Akonadi
{
namespace Filter
{

class Property;
class Data;
class Operator;

namespace Condition
{

class AKONADI_FILTER_EXPORT Base : public Component
{
public:
  enum ConditionType
  {
    ConditionTypeAnd,
    ConditionTypeOr,
    ConditionTypeNot,
    ConditionTypePropertyTest
  };
public:
  Base( ConditionType type, Component * parent );
  virtual ~Base();
protected:
  ConditionType mConditionType;
public:
  virtual bool isCondition() const;

  ConditionType conditionType() const
  {
    return mConditionType;
  }
  virtual bool matches( Data * data );

  virtual void dump( const QString &prefix );
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

  void dumpChildConditions( const QString &prefix );
  virtual void dump( const QString &prefix );
};

class AKONADI_FILTER_EXPORT Not : public Base
{
public:
  Not( Component * parent );
  ~Not();
protected:
  Condition::Base * mChildCondition;
public:

  Condition::Base * childCondition()
  {
    return mChildCondition;
  }

  Condition::Base * releaseChildCondition()
  {
    Condition::Base * cond = mChildCondition;
    mChildCondition = 0;
    return cond;
  }


  void setChildCondition( Condition::Base * condition )
  {
    Q_ASSERT( !mChildCondition );
    mChildCondition = condition;
  }
  virtual bool matches( Data * data );

  virtual void dump( const QString &prefix );
};

class AKONADI_FILTER_EXPORT And : public Multi
{
public:
  And( Component * parent );
  virtual ~And();
public:
  virtual bool matches( Data * data );

  virtual void dump( const QString &prefix );
}; // class And

class AKONADI_FILTER_EXPORT Or : public Multi
{
public:
  Or( Component * parent );
  virtual ~Or();
public:
  virtual bool matches( Data * data );

  virtual void dump( const QString &prefix );
}; // class Or

class AKONADI_FILTER_EXPORT PropertyTest : public Base
{
public:
  PropertyTest( Component * parent, const Property * property, const QString &propertyArgument, const Operator * op, const QVariant &operand );
  virtual ~PropertyTest();
protected:
  const Property * mProperty; // shallow, never null
  QString mPropertyArgument; // may be empty (no argument given)
  const Operator * mOperator; // may be null (only for boolean properties)
  QVariant mOperand; // may be null (only for boolean properties)
public:
  virtual bool matches( Data * data );

  virtual void dump( const QString &prefix );
};

} // namespace Condition

} // namespace Filter

} // namespace Akonadi

#endif //!_AKONADI_FILTER_CONDITION_H_
