/****************************************************************************** *
 *
 *  File : componentfactory.h
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

#ifndef _AKONADI_FILTER_COMPONENTFACTORY_H_
#define _AKONADI_FILTER_COMPONENTFACTORY_H_

#include <akonadi/filter/config-akonadi-filter.h>

#include <QtCore/QString>
#include <QtCore/QList>
#include <QtCore/QHash>


#include <akonadi/filter/functiondescriptor.h>
#include <akonadi/filter/commanddescriptor.h>
#include <akonadi/filter/datatype.h>
#include <akonadi/filter/datamemberdescriptor.h>
#include <akonadi/filter/operatordescriptor.h>

namespace Akonadi
{
namespace Filter
{

// Forward decls

class Program;
class Rule;
class Component;

namespace Condition
{
  class Base;
  class And;
  class Or;
  class Not;
  class True;
  class False;
} // namespace Condition

namespace Action
{
  class Base;
  class Command;
  class Stop;
  class RuleList;
} // namespace Action

/**
 * The feature bits used for logically matching components in the editors and
 * the IO classes.
 */
enum Features
{
  /**
   * The component/object contains a QDateTime field.
   */
  FeatureContainsDate = 1,

  /**
   * The first bit that can be used for custom features.
   * Use this bit in your own enumeration and shift it one
   * position left at each feature you add.
   */
  FeatureCustomFirstBit = (1 << 1)

}; // enum Features

/**
 * The Akonadi::Filter::ComponentFactory class plays a very central role in the filter management and customisation.
 *
 * It's responsable for the creation of the filter tree components, either standard ones (the Program,
 * the Rule, the RuleList, the And/Or/Not condition etc...) or customized ones.
 *
 * It acts as repository of the FunctionDescriptors that can be tested in filter conditions.
 *
 * It acts as repository of the Actions that the filter can perform.
 *
 * Both in your filtering engine and in your editing program you will need an instance of a ComponentFactory.
 * The factory will have to contain the set of functions, operators, data members and actions that your
 * filtering framework will handle. The same sets are needed on both sides (filtering engine and editing program).
 *
 * This class is in fact only the interface that provides the minimum necessary capabilities for filtering
 * and it's substantially data type independant. Your filter implementation will almost surely need
 * to register additional functions, data members and actions.
 *
 * The ComponentFactoryRfc822 is an example of how a real-world component factory might look like.
 */
class AKONADI_FILTER_EXPORT ComponentFactory
{
public:
  ComponentFactory();
  virtual ~ComponentFactory();

private:

  /**
   * Internal list of registered function descriptors.
   * The pointers here are shallow (mFunctionDescriptorHash below owns them).
   */
  QList< const FunctionDescriptor * > mFunctionDescriptorList;

  /**
   * Internal map of registered function descriptors indicized by
   * their keyword (which must be unique). The pointers here are owned.
   */
  QHash< QString, FunctionDescriptor * > mFunctionDescriptorHash;

  /**
   * Internal map of registered operators indicized by
   * their keyword (which must be unique). The pointers here are owned.
   */
  QHash< QString, OperatorDescriptor * > mOperatorDescriptorHash;

  /**
   * Internal list of registered commands.
   * The pointers here are shallow (mCommandDescriptorHash below owns them).
   */
  QList< const CommandDescriptor * > mCommandDescriptorList;

  /**
   * Internal map of registered commands indicized by
   * their keyword (which must be unique). The pointers here are owned.
   */ 
  QHash< QString, CommandDescriptor * > mCommandDescriptorHash;

  /**
   * Internal map of registered data members indicized by
   * their keyword (which must be unique). The pointers here are owned.
   */ 
  QHash< QString, DataMemberDescriptor * > mDataMemberDescriptorHash;

  /**
   * The last error occured in this factory.
   */ 
  QString mLastError;

public:

  /**
   * Registers a FunctionDescriptor to be used by the filter condtions.
   */
  void registerFunction( FunctionDescriptor * function );

  /**
   * Finds a registered FunctionDescriptor by its keyword. This is used
   * by the Decoder subclasses.
   *
   * Returns 0 if no FunctionDescriptor is registered with the specified keyword.
   *
   * Please note that the keywords are case insensitive.
   */
  virtual const FunctionDescriptor * findFunction( const QString &keyword );

  /**
   * Enumerates the avaiable FunctionDescriptor descriptors.
   *
   * This is used primairly by the UI filter editors.
   */
  virtual const QList< const FunctionDescriptor * > * enumerateFunctions();

  /**
   * Registers a CommandDescriptor to be used by the filter actions.
   */
  void registerCommand( CommandDescriptor * command );

  /**
   * Finds a registered CommandDescriptor by its keyword. This is used
   * by the Decoder subclasses.
   *
   * Returns 0 if no CommandDescriptor is registered with the specified keyword.
   *
   * Please note that the keywords are case insensitive.
   */
  virtual const CommandDescriptor * findCommand( const QString &keyword );

  /**
   * Enumerates the avaiable CommandDescriptor descriptors.
   *
   * This is used primairly by the UI filter editors.
   */
  virtual const QList< const CommandDescriptor * > * enumerateCommands();

  /**
   * Registers a DataMemberDescriptor to be used by the filtering conditions.
   */
  void registerDataMember( DataMemberDescriptor * dataMember );

  /**
   * Finds a registered DataMemberDescriptor object by its keyword. This is used
   * by the Decoder subclasses.
   *
   * Returns 0 if no DataMemberDescriptor is registered with the specified keyword.
   *
   * Please note that the keywords are case insensitive.
   */
  virtual const DataMemberDescriptor * findDataMember( const QString &keyword );

  /**
   * Enumerates the DataMemberDescriptor descriptors that are matched by the
   * specified data type and provide ALL the required feature bits.
   *
   * This is used primairly by the UI filter editors to find the
   * DataMemberDescriptor objects that can be "used by" a FunctionDescriptor with a
   * given allowed input DataType set.
   */
  virtual QList< const DataMemberDescriptor * > enumerateDataMembers( int acceptableDataTypeMask, int requiredFeatureBits );

  /**
   * Enumerates all the DataMemberDescriptor descriptors.
   */
  virtual QList< const DataMemberDescriptor * > enumerateDataMembers();


  /**
   * Registers an OperatorDescriptor to be used by the filtering conditions.
   */
  void registerOperator( OperatorDescriptor * op );

  /**
   * Finds the registered OperatorDescriptor objects which has the specified keyword and
   * supports the DataType specified by leftOperandDataType.
   *
   * This is used by the Decoder subclasses.
   *
   * Please note that the keywords are case insensitive.
   */
  virtual const OperatorDescriptor * findOperator( const QString &keyword );

  /**
   * Enumerates the OperatorDescriptor objects that are matched by the specified
   * data type mask.
   *
   * This is used primairly by the UI filter editors to find the
   * OperatorDescriptor objects that can be applied to the result of a FunctionDescriptor
   * with a given ouptut DataType and feature bits.
   */
  virtual QList< const OperatorDescriptor * > enumerateOperators( DataType leftOperandDataType, int featureBits );

  /**
   * Creates an instance of a filtering program.
   * The other components of the filtering framework will
   * call this function instead of directly instantiating the class.
   *
   * May be reimplemented by subclasses in order to return their
   * own customized implementation of the Program object.
   *
   * Returns the Program pointer in case of success and NULL
   * in case of error. In the latter case lastError() may be used
   * to obtain additional informations about the cause of failure.
   *
   * The default implementation fails only in case of out-of-memory condition.
   *
   * A failure in a subclass reimplementation should be signaled by a NULL return
   * value and should cause a call to setLastError().
   */
  virtual Program * createProgram();

  /**
   * Creates an instance of a filtering rule.
   * The other components of the filtering framework will
   * call this function instead of directly instantiating the class.
   *
   * May be reimplemented by subclasses in order to return their
   * own customized implementation of the Rule object.
   *
   * Returns the Rule pointer in case of success and NULL
   * in case of error. In the latter case lastError() may be used
   * to obtain additional informations about the cause of failure.
   *
   * The default implementation fails only in case of out-of-memory condition.
   *
   * A failure in a subclass reimplementation should be signaled by a NULL return
   * value and should cause a call to setLastError().
   */
  virtual Rule * createRule( Component * parent );

  /**
   * Creates an instance of a standard And Condition.
   * The other components of the filtering framework will
   * call this function instead of directly instantiating the class.
   *
   * May be reimplemented by subclasses in order to return their
   * own customized implementation of the Condition object.
   *
   * Returns the And pointer in case of success and NULL
   * in case of error. In the latter case lastError() may be used
   * to obtain additional informations about the cause of failure.
   *
   * The default implementation fails only in case of out-of-memory condition.
   *
   * A failure in a subclass reimplementation should be signaled by a NULL return
   * value and should cause a call to setLastError().
   *
   * In fact it's very unlikely that you'll ever need to reimplement this
   * function since the default And condition is what you want most of the times.
   */
  virtual Condition::And * createAndCondition( Component * parent );

  /**
   * Creates an instance of a standard Or Condition.
   * The other components of the filtering framework will
   * call this function instead of directly instantiating the class.
   *
   * May be reimplemented by subclasses in order to return their
   * own customized implementation of the Condition object.
   *
   * Returns the Or pointer in case of success and NULL
   * in case of error. In the latter case lastError() may be used
   * to obtain additional informations about the cause of failure.
   *
   * The default implementation fails only in case of out-of-memory condition.
   *
   * A failure in a subclass reimplementation should be signaled by a NULL return
   * value and should cause a call to setLastError().
   *
   * In fact it's very unlikely that you'll ever need to reimplement this
   * function since the default Or condition is what you want most of the times.
   */
  virtual Condition::Or * createOrCondition( Component * parent );

  /**
   * Creates an instance of a standard Not Condition.
   * The other components of the filtering framework will
   * call this function instead of directly instantiating the class.
   *
   * May be reimplemented by subclasses in order to return their
   * own customized implementation of the Condition object.
   *
   * Returns the Not pointer in case of success and NULL
   * in case of error. In the latter case lastError() may be used
   * to obtain additional informations about the cause of failure.
   *
   * The default implementation fails only in case of out-of-memory condition.
   *
   * A failure in a subclass reimplementation should be signaled by a NULL return
   * value and should cause a call to setLastError().
   *
   * In fact it's very unlikely that you'll ever need to reimplement this
   * function since the default Not condition is what you want most of the times.
   */
  virtual Condition::Not * createNotCondition( Component * parent );

  /**
   * Creates an instance of a standard "Always True" Condition.
   * The other components of the filtering framework will
   * call this function instead of directly instantiating the class.
   *
   * May be reimplemented by subclasses in order to return their
   * own customized implementation of the Condition object.
   *
   * Returns the True pointer in case of success and NULL
   * in case of error. In the latter case lastError() may be used
   * to obtain additional informations about the cause of failure.
   *
   * The default implementation fails only in case of out-of-memory condition.
   *
   * A failure in a subclass reimplementation should be signaled by a NULL return
   * value and should cause a call to setLastError().
   *
   * In fact it's very unlikely that you'll ever need to reimplement this
   * function since the default True condition is what you want most of the times.
   */
  virtual Condition::True * createTrueCondition( Component * parent );

  /**
   * Creates an instance of a standard "Always False" Condition.
   * The other components of the filtering framework will
   * call this function instead of directly instantiating the class.
   *
   * May be reimplemented by subclasses in order to return their
   * own customized implementation of the Condition object.
   *
   * Returns the False pointer in case of success and NULL
   * in case of error. In the latter case lastError() may be used
   * to obtain additional informations about the cause of failure.
   *
   * The default implementation fails only in case of out-of-memory condition.
   *
   * A failure in a subclass reimplementation should be signaled by a NULL return
   * value and should cause a call to setLastError().
   *
   * In fact it's very unlikely that you'll ever need to reimplement this
   * function since the default False condition is what you want most of the times.
   */
  virtual Condition::False * createFalseCondition( Component * parent );

  /**
   * Creates an instance of the PropertyTest Condition.
   * The other components of the filtering framework will
   * call this function instead of directly instantiating the class.
   *
   * May be reimplemented by subclasses in order to return their
   * own customized implementation of the Condition object.
   *
   * Returns the PropertyTest pointer in case of success and NULL
   * in case of error. In the latter case lastError() may be used
   * to obtain additional informations about the cause of failure.
   *
   * The default implementation fails only in case of out-of-memory condition.
   *
   * A failure in a subclass reimplementation should be signaled by a NULL return
   * value and should cause a call to setLastError().
   *
   * Please note that function may be null: it means "straight value of the dataMember".
   * op may also be null: in that case the result of the function (or the dataMember)
   * must be boolean and the operand parameter should be ignored.
   */
  virtual Condition::Base * createPropertyTestCondition(
      Component * parent,
      const FunctionDescriptor * function,
      const DataMemberDescriptor * dataMember,
      const OperatorDescriptor * op,
      const QVariant &operand
    );

  /**
   * Creates an instance of the Command Action.
   * The other components of the filtering framework will
   * call this function instead of directly instantiating the class.
   *
   * May be reimplemented by subclasses in order to return their
   * own customized implementation of the Condition object.
   *
   * Returns the Command pointer in case of success and NULL
   * in case of error. In the latter case lastError() may be used
   * to obtain additional informations about the cause of failure.
   *
   * The default implementation fails only in case of out-of-memory condition.
   *
   * A failure in a subclass reimplementation should be signaled by a NULL return
   * value and should cause a call to setLastError().
   */
  virtual Action::Command * createCommand(
      Component * parent,
      const CommandDescriptor * command,
      const QList< QVariant > &params
    );

  /**
   * Creates an instance of the RuleList Action.
   * The other components of the filtering framework will
   * call this function instead of directly instantiating the class.
   *
   * May be reimplemented by subclasses in order to return their
   * own customized implementation of the Condition object.
   *
   * Returns the RuleList pointer in case of success and NULL
   * in case of error. In the latter case lastError() may be used
   * to obtain additional informations about the cause of failure.
   *
   * The default implementation fails only in case of out-of-memory condition.
   *
   * A failure in a subclass reimplementation should be signaled by a NULL return
   * value and should cause a call to setLastError().
   */
  virtual Action::RuleList * createRuleList( Component * parent );

  /**
   * Creates an instance of the standard Stop Action.
   * The other components of the filtering framework will
   * call this function instead of directly instantiating the class.
   *
   * May be reimplemented by subclasses in order to return their
   * own customized implementation of the Condition object.
   *
   * Returns the Stop pointer in case of success and NULL
   * in case of error. In the latter case lastError() may be used
   * to obtain additional informations about the cause of failure.
   *
   * The default implementation fails only in case of out-of-memory condition.
   *
   * A failure in a subclass reimplementation should be signaled by a NULL return
   * value and should cause a call to setLastError().
   *
   * In fact it's very unlikely that you'll ever need to reimplement this
   * function since the default Stop action is what you want most of the times.
   */
  virtual Action::Stop * createStopAction( Component * parent );

  /**
   * Returns the last error occured in this component factory.
   */
  const QString & lastError() const
  {
    return mLastError;
  }

protected:

  /**
   * Sets the last error occured on this engine.
   */
  void setLastError( const QString &error )
  {
    mLastError = error;
  }

}; // class ComponentFactory

} // namespace Filter

} // namespace Akonadi

#endif //!_AKONADI_FILTER_COMPONENTFACTORY_H_
