/****************************************************************************** *
 *
 *  File : operatordescriptor.h
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

#ifndef _AKONADI_FILTER_OPERATORDESCRIPTOR_H_
#define _AKONADI_FILTER_OPERATORDESCRIPTOR_H_

#include "config-akonadi-filter.h"

#include <QString>

#include "datatype.h"

namespace Akonadi
{
namespace Filter
{

/**
 * The standard operator numeric identifiers.
 */
enum OperatorIdentifiers
{
  StandardOperatorGreaterThan = 1,         // Integer > Integer
  StandardOperatorLowerThan,               // Integer < Integer
  StandardOperatorIntegerIsEqualTo,        // Integer == Integer 
  StandardOperatorStringIsEqualTo,         // String | Address == String
  StandardOperatorContains,                // String | StringList | Address | AddressList contains String
  StandardOperatorIsInAddressbook,         // Address | AddressList is in addressbook
  StandardOperatorStringMatchesRegexp,     // String | StringList | Address | AddressList matches String (regexp)
  StandardOperatorStringMatchesWildcard,   // String | StringList | Address | AddressList matches String (wildcard)
  StandardOperatorDateIsEqualTo,           // DateTime == DateTime
  StandardOperatorDateIsAfter,             // DateTime > DateTime
  StandardOperatorDateIsBefore             // DateTime < DateTime
};

/**
 * The OperatorDescriptor is an essential part of a Condition::PropertyTest object inside a filtering Rule.
 *
 * Examples of operators are "matches", "equals", "above", "below", "inaddressbook" etc...
 *
 * Each operator can accept a set of left data types (obtained as result of a FunctionDescriptor applied
 * to a DataMemberDescriptor which in turn extracts stuff from the item being filtered) and none or exactly
 * one right data type (which is the data type of the constant the operator compares "against").
 *
 * The OperatorDescriptor has a keyword and an id which *must* be unique inside a filtering application set.
 * The keyword is used in filter storage (sieve scripts at the time of writing) while the integer id
 * can be used for fast identification inside the filter implementations.
 *
 * The id is either a member of the OperatorIdentifiers enumeration or a custom id (which should
 * be a number above OperatorIdentifiers::OperatorDescriptorCustomFirst.
 *
 * An operator must always return a boolean result. This is different from a FunctionDescriptor which
 * can return any data type (even multiple ones).
 */
class AKONADI_FILTER_EXPORT OperatorDescriptor
{
public:
  OperatorDescriptor(
      int id,
      const QString &keyword,
      const QString &name,
      int acceptableLeftOperandDataTypeMask,
      DataType rightOperandDataType
    );
  virtual ~OperatorDescriptor();

protected:

  /**
   * The unique id of this operatordescriptor.
   */
  int mId;

  /**
   * The non-localized keyword of the operatordescriptor. Must be unique.
   */
  QString mKeyword;

  /**
   * The localized name of the operator (this is what is shown in the selection combos)
   */
  QString mName;

  /**
   * The left input data types we can accept.
   */
  int mAcceptableLeftOperandDataTypeMask;

  /**
   * The expected data type of the right operand (may be DataTypeNone for no right operand)
   */
  DataType mRightOperandDataType;

public:

  int id() const
  {
    return mId;
  }

  int acceptableLeftOperandDataTypeMask() const
  {
    return mAcceptableLeftOperandDataTypeMask;
  }

  DataType rightOperandDataType() const
  {
    return mRightOperandDataType;
  }

  const QString & keyword() const
  {
    return mKeyword;
  }

  const QString & name() const
  {
    return mName;
  }

}; // class OperatorDescriptor

} // namespace Filter

} // namespace Akonadi

#endif //!_AKONADI_FILTER_OPERATORDESCRIPTOR_H_
