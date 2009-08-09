/****************************************************************************** *
 *
 *  File : data.h
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

#ifndef _AKONADI_FILTER_DATA_H_
#define _AKONADI_FILTER_DATA_H_

#include <akonadi/filter/config-akonadi-filter.h>

#include <QtCore/QList>
#include <QtCore/QVariant>

#include <akonadi/filter/datatype.h>
#include <akonadi/filter/errorstack.h>

namespace Akonadi
{
namespace Filter
{

class CommandDescriptor;
class DataMemberDescriptor;
class FunctionDescriptor;
class OperatorDescriptor;

/**
 * This is the base class for the Data object that you "throw" through a filter tree
 * for execution. In fact you must use a subclass since some of the functions contained
 * in this interface are pure virtual.
 */
class AKONADI_FILTER_EXPORT Data : public ErrorStack
{
public:
  Data();
  virtual ~Data();

public:

  /**
   * The result of a performPropertyTest() call.
   */
  enum PropertyTestResult
  {
    /**
     * The property test is verified (condition matched).
     */
    PropertyTestVerified,

    /**
     * The property test is not verified (condition didn't match).
     */
    PropertyTestNotVerified,

    /**
     * An error has been encountered during the evaluation of the test.
     */
    PropertyTestError
  };

  /**
   * Perform a property test on the data.
   *
   * The function/dataMember pair define the property you should retrieve via
   * a call to getPropertyValue(). The op is the descriptor of the operator
   * you should apply to the property and the right operand.
   *
   * If the operator right data type is DataTypeNone then operand is a null
   * variant and it should be ignored by the implementation.
   *
   * Thedefault implementation is something like:
   *
   *   evaluateOperator( getPropertyValue( function, dataMember ), op, operand );
   *
   * If you use only the standard functions and operators provided by this library
   * then the current implementation of this function will work out of the box.
   * If you use additional functions, additional operators or you want to provide
   * a super-efficient implementation of some data member/function/operator combination
   * then you can override this function.
   */
  virtual PropertyTestResult performPropertyTest(
      const FunctionDescriptor * function,
      const DataMemberDescriptor * dataMember,
      const OperatorDescriptor * op,
      const QVariant &operand
    );

  /**
   * Retrieve a value of a property which is defined to be the application
   * of a function to a data member. Think of something like:
   *
   *    length( From field )
   *
   * where the "From" field is a data member extracted from your real data
   * (for instance, the "From" header of an e-mail message) and length() is
   * a function applied on this data member to return its length in bytes.
   *
   * The value of the data member is retrieved by calling getDataMemberValue().
   *
   * If you use only the standard functions provided by this library
   * then the current implementation of this function will work out of the box.
   * If you use additional functions or you want to provide a super-efficient
   * implementation of some data member/function combination then you
   * can override this function.
   *
   * Efficiency can be improved greatly for some of the functions. For instance
   * the default implementation of a property like
   *
   *    exists( Body )
   *
   * will first retrieve the whole body of the message and then return
   * a boolean bit of information: the body either exists or not.
   * Retrieving the whole body can be expensive and if you can
   * determine its existence without actually fetching the whole data
   * then reimplementing this function might be a great performance improvement.
   *
   * Upon succesfull execution you should return the value of the function
   * applied to the data member. If the execution fails then you should
   * return a null QVariant and push some errors on the stack (see the ErrorStack
   * base classe).
   */
  virtual QVariant getPropertyValue( const FunctionDescriptor * function, const DataMemberDescriptor * dataMember );

  /**
   * You must implement this function in order to execute your Command type actions.
   *
   * Upon succesfull execution you should return true while on failure you should
   * return false and push some errors on the stack (see the ErrorStack base classe).
   */
  virtual bool executeCommand( const CommandDescriptor * command, const QList< QVariant > &params ) = 0;

  /**
   * You must implement this function in order to retrieve the data member
   * specified by the DataMemberDescriptor.
   * 
   * Upon succesfull execution you should return the value of the data member.
   * If the execution fails then you should return a null QVariant and push some errors
   * on the stack (see the ErrorStack base classe).
   */
  virtual QVariant getDataMemberValue( const DataMemberDescriptor * dataMember ) = 0;
};

} // namespace Filter

} // namespace Akonadi

#endif //!_AKONADI_FILTER_DATA_H_
