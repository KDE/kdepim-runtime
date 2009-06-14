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
  class Stop;
  class RuleList;
} // namespace Action

/**
 * The Akonadi::Filter::ComponentFactory class plays a very central role in the filter management and customisation.
 *
 * - It's responsable for the creation of the filter tree components, either standard ones (the Program,
 *   the Rule, the RuleList, the And/Or/Not condition etc...) or customized ones.
 *
 * - It acts as repository of the FunctionDescriptors that can be tested in filter conditions
 *
 * - It acts as repository of the Actions that the filter can perform
 */
class AKONADI_FILTER_EXPORT ComponentFactory
{
public:
  ComponentFactory();
  virtual ~ComponentFactory();

private:
  QList< const FunctionDescriptor * > mFunctionDescriptorList;
  QHash< QString, FunctionDescriptor * > mFunctionDescriptorHash;

  QList< OperatorDescriptor * > mOperatorDescriptorList;
  QMultiHash< QString, OperatorDescriptor * > mOperatorDescriptorMultiHash;

  QList< const CommandDescriptor * > mCommandDescriptorList;
  QHash< QString, CommandDescriptor * > mCommandDescriptorHash;

  QHash< QString, DataMemberDescriptor * > mDataMemberDescriptorHash;

  QString mLastError;

  QString mDefaultActionDescription;
public:

  void setDefaultActionDescription( const QString &defaultActionDescription )
  {
    mDefaultActionDescription = defaultActionDescription;
  }

  const QString & defaultActionDescription()
  {
    return mDefaultActionDescription;
  }

  void setLastError( const QString &error )
  {
    mLastError = error;
  }

  const QString & lastError() const
  {
    return mLastError;
  }

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
   * specified data type mask.
   *
   * This is used primairly by the UI filter editors to find the
   * DataMemberDescriptor objects that can be "used by" a FunctionDescriptor with a
   * given allowed input DataType set.
   */
  virtual QList< const DataMemberDescriptor * > enumerateDataMembers( int acceptableDataTypeMask );

  /**
   * Registers an OperatorDescriptor to be used by the filtering conditions.
   */
  void registerOperator( OperatorDescriptor * op );

  /**
   * Finds a registered OperatorDescriptor object which has the specified keyword
   * and supports all the DataTypes specified by leftOperandDataTypeMask.
   * This is used by the Decoder subclasses.
   *
   * Returns 0 if no register OperatorDescriptor matches the specified conditions.
   *
   * Please note that the keywords are case insensitive.
   */
  virtual const OperatorDescriptor * findOperator( const QString &keyword, int leftOperandDataTypeMask );

  /**
   * Enumerates the OperatorDescriptor objects that are matched by the specified
   * data type mask.
   *
   * This is used primairly by the UI filter editors to find the
   * OperatorDescriptor objects that can be applied to the result of a FunctionDescriptor
   * with a given possible ouptut DataType set.
   */
  virtual QList< const OperatorDescriptor * > enumerateOperators( int leftOperandDataTypeMask );


  virtual Program * createProgram();

  virtual Rule * createRule( Component * parent );

  virtual Condition::And * createAndCondition( Component * parent );
  virtual Condition::Or * createOrCondition( Component * parent );
  virtual Condition::Not * createNotCondition( Component * parent );
  virtual Condition::True * createTrueCondition( Component * parent );
  virtual Condition::False * createFalseCondition( Component * parent );
  virtual Condition::Base * createPropertyTestCondition(
      Component * parent,
      const FunctionDescriptor * function,
      const DataMemberDescriptor * dataMember,
      const OperatorDescriptor * op,
      const QVariant &operand
    );

  virtual Action::Base * createCommandAction( Component * parent, const CommandDescriptor * command, const QList< QVariant > &params );
  virtual Action::RuleList * createRuleList( Component * parent );
  virtual Action::Stop * createStopAction( Component * parent );
}; // class ComponentFactory

} // namespace Filter

} // namespace Akonadi

#endif //!_AKONADI_FILTER_COMPONENTFACTORY_H_
