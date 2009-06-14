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

#include <akonadi/filter/config-akonadi-filter.h>

#include <akonadi/filter/component.h>

#include <QtCore/QList>
#include <QtCore/QVariant>

namespace Akonadi
{
namespace Filter
{

class Data;
class DataMemberDescriptor;
class FunctionDescriptor;
class OperatorDescriptor;

namespace Condition
{

enum ConditionType
{
  ConditionTypeAnd,
  ConditionTypeOr,
  ConditionTypeNot,
  ConditionTypeTrue,
  ConditionTypeFalse,
  ConditionTypePropertyTest,
  ConditionTypeUnknown // this is used only in the editor
};

class AKONADI_FILTER_EXPORT Base : public Component
{
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

  const QList< Condition::Base * > * childConditions()
  {
    return &mChildConditions;
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

class AKONADI_FILTER_EXPORT True : public Base
{
public:
  True( Component * parent );
  virtual ~True();
public:
  virtual bool matches( Data * data );
  virtual void dump( const QString &prefix );
};

class AKONADI_FILTER_EXPORT False : public Base
{
public:
  False( Component * parent );
  virtual ~False();
public:
  virtual bool matches( Data * data );
  virtual void dump( const QString &prefix );
};

/**
 * The PropertyTest is a highly customizable condition with a "standard" implementation.
 * The basic schema is something like
 *
 *    PropertyTest( Data ) = FunctionDescriptor( DataMemberDescriptor( Data ) ) [ OperatorDescriptor [ Operand ] ]
 *
 * Which in most cases (when OperatorDescriptor and Operand aren't omitted) can be seen also as
 *
 *    PropertyTest( Data ) = OperatorDescriptor( FunctionDescriptor( DataMemberDescriptor( Data ) ) , Operand )
 *
 * For instance a condition like
 *
 *    sizeof( From ) >= 100KB
 *
 * is turned into
 *
 *    DataMemberDescriptor = "From field" (returns String)
 *    FunctionDescriptor = "sizeof" (expects a String parameter, returns Integer)
 *    OperatorDescriptor = "greater or equal" (expects an Integer on the left)
 *    Operand = 100KB (translated into Integer)
 *
 * The full combination of the FunctionDescriptor, DataMemberDescriptor and the OperatorDescriptor must return
 * a Boolean value which is exactly the result of this PropertyTest Condition.
 *
 * In certain cases the Operand may be omitted (so a Null variant is used).
 * For instance the condition "if any address extracted from the CC field is in the addressbook"
 * is encoded as 
 *
 *    isinaddressbook( anyaddress( CC ) )
 *
 * in that
 *
 *    DataMemberDescriptor = "CC field" (returns String)
 *    FunctionDescriptor = "anyaddress" (expects Address, AddressList, String or StringList as input, returns Address or AddressList)
 *    OperatorDescriptor = "isinaddressbook" (expects an Address or AddressList on the left)
 *    Operand = none (null variant)
 *
 * In very special cases the OperatorDescriptor can be omitted too (0 is used). This may
 * happen only if the FunctionDescriptor object returns a Boolean value (so no operator is required
 * to turn the result into a boolean). For instance the "exists X-Mailer field" condition
 * is simply
 *
 *    DataMemberDescriptor = "X-Mailer" (returns String)
 *    FunctionDescriptor = "exists" (expects any data type, returns Boolean)
 *    OperatorDescriptor = none (0)
 *    Operand = none (null variant)
 *
 * There two main reasons that lead to this kind of rappresentation of property tests.
 *
 * In the first place this is the simplest model that can "enclose" the Sieve format
 * which we're using to store the filters on disk.
 *
 * In the second place, this model allows an extensible but, in the end, still manageable
 * implementation. The DataMemberDescriptor objects describe the "fields" that you retrieve directly
 * from the filtered Data. The core filtering library doesn't actually know the format of
 * the data and thus provides only an interface for it. The FunctionDescriptor objects describe the
 * common "derived" data that you can obtain by manipulating the results of DataMemberDescriptor
 * extraction. There is a set of "standard" FunctionDescriptors (sizeof(), countof(), exists etc..)
 * which the core library can provide. You can, then, provide optimized implementations
 * of the FunctionDescriptor(DataMemberDescriptor) combinations or even register your own FunctionDescriptors.
 * The OperatorDescriptor objects describe very common operations which are applied to the
 * result of the FunctionDescriptor(DataMemberDescriptor) combination above. It's very unlikely that
 * you'll need an OperatorDescriptor which is not already provided by the core filtering library.
 * The Operand, then, is a parameter that depends on the OperatorDescriptor and is again handled
 * internally by the core filtering library.
 */
class AKONADI_FILTER_EXPORT PropertyTest : public Base
{
public:
  PropertyTest( Component * parent, const FunctionDescriptor * function, const DataMemberDescriptor * dataMember, const OperatorDescriptor * op, const QVariant &operand );
  virtual ~PropertyTest();
protected:
  const FunctionDescriptor * mFunctionDescriptor; // shallow, never null
  const DataMemberDescriptor * mDataMemberDescriptor; // shallow, never null
  const OperatorDescriptor * mOperatorDescriptor; // may be null (only when FunctionDescriptor returns Boolean)
  QVariant mOperand; // may be null (only if FunctionDescriptor is boolean or OperatorDescriptor is unary and thus doesn't expect a right operand)
public:
  const FunctionDescriptor * function() const
  {
    return mFunctionDescriptor;
  }

  const DataMemberDescriptor * dataMember() const
  {
    return mDataMemberDescriptor;
  }

  const OperatorDescriptor * functionOperatorDescriptor() const
  {
    return mOperatorDescriptor;
  }

  const QVariant & operand() const
  {
    return mOperand;
  }

  virtual bool matches( Data * data );

  virtual void dump( const QString &prefix );
}; // class PropertyTest

} // namespace Condition

} // namespace Filter

} // namespace Akonadi

#endif //!_AKONADI_FILTER_CONDITION_H_
