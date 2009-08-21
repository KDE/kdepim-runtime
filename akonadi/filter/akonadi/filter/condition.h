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

/**
 * @namespace Akonadi::Filter::Condition
 * @brief The namespace containing the conditions a filter can verify
 *
 * Each Rule inside a Program has a condition which must be matched
 * in order for the Rule to apply. The conditions can be very simple
 * (a single test or even an "always true" test) or very complex
 * (a multilevel tree).
 *
 *
 * Some of the conditions are builtin while others are provided
 * by the context-specific implementation. At the time of writing the builtins
 * are the True condition (always matches), the False condition (never matches)
 * the Not conditon (which inverts the match of a child condition), the
 * And condition (which matches if all its children match) and the Or condition
 * (which matches if at least one of the children matches). The PropertyTest
 * condition is a skeleton for implementing the context-specific tests.
 */
namespace Condition
{

/**
 * A rough classification of the condition types.
 */
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

/**
 * @class Akonadi::Filter::Condition::Base
 * @brief The base class of all the condition objects.
 *
 * This class contains pure virtuals which must
 * be implemented by the real condition subclasses.
 */
class AKONADI_FILTER_EXPORT Base : public Component
{
public:

  /**
   * The possible results of the matches() function.
   */
  enum MatchResult
  {
    /**
     * Condition matches, no error.
     */
    ConditionMatches,

    /**
     * Condition does not match, no error.
     */
    ConditionDoesNotMatch,

    /**
     * Error evaluating the condition, match undefined.
     */
    ConditionMatchError
  };

public:

  /**
   * Create a condition with a specific type
   * and parent Component (which will be either a Rule
   * object or another Base subclass).
   *
   * @param type The condition type
   * @param parent The parent Component
   */
  Base( ConditionType type, Component * parent );

  /**
   * Destroy the condition and all the children.
   */
  virtual ~Base();

protected:

  /**
   * The real type of the condition. This is set to a
   * specific value by the subclasses.
   */
  ConditionType mConditionType;

public:

  /**
   * Reimplemented from Component: returns true.
   */
  virtual bool isCondition() const;

  /**
   * Returns the type of this condition.
   */
  ConditionType conditionType() const
  {
    return mConditionType;
  }

  /**
   * This method must be reimplemented in subclasses in order
   * to verify the condition match on the specified Data object.
   *
   * If the specific implementation of the condition matches then
   * the method must return ConditionMatches. If the condition doesn't
   * match then it must return ConditionDoesNotMatch. If there is
   * an error in the condition evaluation then the method must
   * return ConditionMatchError. Quite simple.
   *
   * ConditionMatchError will stop the evaluation of the filter on
   * this instance of Data.
   *
   * If you override this method then you should be prepared to handle
   * multiple matches() calls one after another with different instances of data:
   * the condition objects are intended to be reusable.
   *
   * @param data The Data subclass on that the test has to be performed.
   *             Must not be null.
   */
  virtual MatchResult matches( Data * data ) = 0;

}; // class Base


/**
 * @class Akonadi::Filter::Condition::True
 * @brief A trivial always-true condition
 */
class AKONADI_FILTER_EXPORT True : public Base
{
public:

  True( Component * parent );

  virtual ~True();

public:

  /**
   * Reimplemented from Condition::Base.
   * Returns ConditionMatches, unconditionally.
   */
  virtual MatchResult matches( Data * data );

  /**
   * Reimplemented from Component. Debugging aid.
   */
  virtual void dump( const QString &prefix );

}; // class True


/**
 * @class Akonadi::Filter::Condition::False
 * @brief A trivial always-false condition
 */
class AKONADI_FILTER_EXPORT False : public Base
{
public:

  False( Component * parent );

  virtual ~False();

public:

  /**
   * Reimplemented from Condition::Base.
   * Returns ConditionDoesNotMatch, unconditionally.
   */
  virtual MatchResult matches( Data * data );

  /**
   * Reimplemented from Component. Debugging aid.
   */
  virtual void dump( const QString &prefix );

}; // class False


/**
 * @class Akonadi::Filter::Condition::Multi
 * @brief The base class of conditions with multiple children
 *
 * This is used as base class for the And and the Or conditions
 * and still contains pure virtuals from Base.
 */
class AKONADI_FILTER_EXPORT Multi : public Base
{
public:

  /**
   * Create a Multi condition with a specific type
   * and parent Component (which will be either a Rule
   * object or another Base subclass).
   *
   * @param type The condition type
   * @param parent The parent Component
   */
  Multi( ConditionType type, Component * parent );

  /**
   * Destroy the condition and all the children.
   */
  virtual ~Multi();

protected:

  /**
   * The list of child conditions. Owned pointers.
   */
  QList< Condition::Base * > mChildConditions;

public:

  /**
   * Add a child condition. The ownership of the pointer
   * is taken by this class.
   *
   * @param condition The child condition to add. Must not be null.
   */
  void addChildCondition( Condition::Base * condition )
  {
    mChildConditions.append( condition );
  }

  /**
   * Returns the list of child conditions contained in this object.
   * The returned pointer is never null.
   */
  const QList< Condition::Base * > * childConditions()
  {
    return &mChildConditions;
  }

  /**
   * Removes all the child conditions, deleting them.
   */
  void clearChildConditions();

  /**
   * Debugging aid used by dump()
   */
  void dumpChildConditions( const QString &prefix );

  /**
   * Reimplemented from Component. Debugging aid.
   * Dumps the condition and all the children on the console.
   */
  virtual void dump( const QString &prefix );

}; // class Multi


/**
 * @class Akonadi::Filter::Condition::And
 * @brief Matches if all the children match
 */
class AKONADI_FILTER_EXPORT And : public Multi
{
public:

  And( Component * parent );
  virtual ~And();

public:

  /**
   * Reimplemented from Condition::Base.
   * Returns ConditionMatches if all the children conditions match.
   * This implementation is short circuiting: if the N-th child condition
   * doesn't match then the following ones aren't tested at all.
   *
   * An And without child conditions always matches.
   */
  virtual MatchResult matches( Data * data );

  /**
   * Reimplemented from Component. Debugging aid.
   */
  virtual void dump( const QString &prefix );

}; // class And


/**
 * @class Akonadi::Filter::Condition::Or
 * @brief Matches if at least one child matches
 */
class AKONADI_FILTER_EXPORT Or : public Multi
{
public:

  Or( Component * parent );
  virtual ~Or();

public:

  /**
   * Reimplemented from Condition::Base.
   * Returns ConditionMatches if at least one child condition matches.
   * This implementation is short circuiting: if the N-th child condition
   * matches then the following ones aren't tested at all.
   *
   * An Or without child conditions never matches.
   */
  virtual MatchResult matches( Data * data );

  /**
   * Reimplemented from Component. Debugging aid.
   */
  virtual void dump( const QString &prefix );

}; // class Or


/**
 * @class Not
 * @brief Negates the result of a child condition
 *
 * The Not condition has a single child. The Not's implementation
 * of the matches() function negates the result returned by
 * a call to the child matches().
 *
 * A Not without a child condition never matches (as an empty
 * condition is assumed to always match).
 */
class AKONADI_FILTER_EXPORT Not : public Base
{
public:

  Not( Component * parent );
  virtual ~Not();

protected:

  /**
   * The child condition of this Not. Owned pointer.
   */
  Condition::Base * mChildCondition;

public:

  /**
   * Returns the pointer to the child condition.
   * The returned pointer may be null.
   */
  Condition::Base * childCondition()
  {
    return mChildCondition;
  }

  /**
   * Releases the child condition without deleting it.
   */
  Condition::Base * releaseChildCondition()
  {
    Condition::Base * cond = mChildCondition;
    mChildCondition = 0;
    return cond;
  }

  /**
   * Set the child condition pointer. The ownership
   * is transferred to this object.
   * Any previously set condition is deleted.
   */
  void setChildCondition( Condition::Base * condition );

  /**
   * Reimplemented from Condition::Base.
   * If the child condition evaluates succesfully then it
   * returns its result negated. If the child condition evaluation
   * fails then propagates the ConditionMatchError to the caller.
   */
  virtual MatchResult matches( Data * data );

  /**
   * Reimplemented from Component. Debugging aid.
   */
  virtual void dump( const QString &prefix );

}; // class Not


/**
 * @class Akonadi::Filter::Condition::PropertyTest
 * @brief Highly customizable condition with a "standard" implementation.
 *
 * The most general schema is something like
 *
 * @code
 *    PropertyTest( Data ) = FunctionDescriptor( DataMemberDescriptor( Data ) ) [ OperatorDescriptor [ Operand ] ]
 * @endcode
 *
 * Which in most cases (when OperatorDescriptor and Operand aren't omitted) can be seen also as
 *
 * @code
 *    PropertyTest( Data ) = OperatorDescriptor( FunctionDescriptor( DataMemberDescriptor( Data ) ) , Operand )
 * @endcode
 *
 * For instance a condition like
 *
 * @code
 *    sizeof( From ) >= 100KB
 * @endcode
 *
 * is turned into
 *
 * @code
 *    DataMemberDescriptor = "From field" (returns String)
 *    FunctionDescriptor = "sizeof" (expects a String parameter, returns Integer)
 *    OperatorDescriptor = "greater or equal" (expects an Integer on the left)
 *    Operand = 100KB (translated into Integer)
 * @endcode
 *
 * The application of a DataMember[Descriptor] (or "data extraction") always returns a single definite data type.
 * The application of a Function[Descriptor] (or "data manipulation") to a data member either returns a definite
 * data type or "passes through" the data type of the DataMember[Descriptor].
 * The application of a Operator[Descriptor] (or "data testing") accepts a single data type on left and
 * a single (but possibly different) data type on the right side (Operand).
 *
 * The full combination of the FunctionDescriptor, DataMemberDescriptor and the OperatorDescriptor must return
 * a Boolean value which is exactly the result of this PropertyTest Condition.
 *
 * In certain cases the Operand may be omitted (so a Null variant is used).
 * For instance the condition "if any address extracted from the CC field is in the addressbook"
 * is encoded as 
 *
 * @code
 *    isinaddressbook( anyaddress( CC ) )
 * @endcode
 *
 * in that
 *
 * @code
 *    DataMemberDescriptor = "CC field" (returns String)
 *    FunctionDescriptor = "anyaddress" (expects Address, AddressList, String or StringList as input, returns Address or AddressList)
 *    OperatorDescriptor = "isinaddressbook" (expects an Address or AddressList on the left)
 *    Operand = none (null variant)
 * @endcode
 *
 * In very special cases the OperatorDescriptor can be omitted too (0 is used). This may
 * happen only if the FunctionDescriptor object returns a Boolean value (so no operator is required
 * to turn the result into a boolean). For instance the "exists X-Mailer field" condition
 * is simply
 *
 * @code
 *    DataMemberDescriptor = "X-Mailer" (returns String)
 *    FunctionDescriptor = "exists" (expects any data type, returns Boolean)
 *    OperatorDescriptor = none (0)
 *    Operand = none (null variant)
 * @endcode
 *
 * Finally the FunctionDescriptor object can be omitted too. This means that the PropertyTest
 * will be aplied directory to the result of the extraction of the DataMember[Descriptor].
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
 *
 * The OperatorDescriptor objects define very common operations which are applied to the
 * result of the FunctionDescriptor(DataMemberDescriptor) combination above. It's very unlikely that
 * you'll need an OperatorDescriptor which is not already provided by the core filtering library.
 * The Operand, then, is a parameter that depends on the OperatorDescriptor and is again handled
 * internally by the core filtering library.
 */
class AKONADI_FILTER_EXPORT PropertyTest : public Base
{
public:

  /**
   * Create a PropertyTest object.
   *
   * The function, dataMember and op parameters, if not null,
   * must be kept alive during the lifetime of this object.
   * The owner of these pointers is usually a ComponentFactory instance.
   */
  PropertyTest(
      Component * parent,
      const FunctionDescriptor * function,
      const DataMemberDescriptor * dataMember,
      const OperatorDescriptor * op,
      const QVariant &operand
    );

  /**
   * Destroy a property test object.
   */
  virtual ~PropertyTest();

protected:

  /**
   * The descriptor of the data member this PropertyTest is applied to.
   * The pointer is owned by the ComponentFactory class and is never null.
   */
  const DataMemberDescriptor * mDataMemberDescriptor;

  /**
   * The descriptor of the function applied to the data member.
   * The pointer is owned by the ComponentFactory class and may be null
   * (when the PropertyTest is applied on the plain data member).
   */
  const FunctionDescriptor * mFunctionDescriptor;

  /**
   * The descriptor of the operator applied on the result of the function
   * (the left operand) and optionally a constant right operand.
   * The pointer is owned by the ComponentFactory class and may be null
   * (when the result of the function is boolean).
   */
  const OperatorDescriptor * mOperatorDescriptor;

  /**
   * The operand used with the operator defined by mOperatorDescriptor.
   * This member is meaningful only if mOperatorDescriptor is non null
   * and its DataType is not DataTypeNone (so not an unary operator).
   */
  QVariant mOperand;

public:

  /**
   * The descriptor of the function applied to the data member.
   * The returned pointer may be null (when the PropertyTest is applied on the plain data member).
   */
  const FunctionDescriptor * function() const
  {
    return mFunctionDescriptor;
  }

  /**
   * Returns the descriptor of the data member this PropertyTest is applied to.
   * The returned pointer is never null.
   */
  const DataMemberDescriptor * dataMember() const
  {
    return mDataMemberDescriptor;
  }

  /**
   * Returns the descriptor of the operator applied on the result of the function
   * (the left operand) and optionally a constant right operand.
   * The returned pointer may be null (when the result of the function is boolean).
   */
  const OperatorDescriptor * operatorDescriptor() const
  {
    return mOperatorDescriptor;
  }

  /**
   * Returns the operand used with the operator defined by mOperatorDescriptor.
   * This member is meaningful only if mOperatorDescriptor is non null
   * and its DataType is not DataTypeNone (so not an unary operator).
   */
  const QVariant & operand() const
  {
    return mOperand;
  }

  /**
   * Implements the PropertyMatch algorithm. See the main class documentation
   * for more informations.
   */
  virtual MatchResult matches( Data * data );

  /**
   * Reimplemented from Component. Debugging aid.
   */
  virtual void dump( const QString &prefix );

private:

  /**
   * Pushes a description of the PropertyTest on the error stack.
   */
  void pushStandardErrors();

}; // class PropertyTest

} // namespace Condition

} // namespace Filter

} // namespace Akonadi

#endif //!_AKONADI_FILTER_CONDITION_H_
