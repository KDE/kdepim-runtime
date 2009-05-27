/****************************************************************************** *
 *
 *  File : functiondescriptor.h
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

#ifndef _AKONADI_FILTER_FUNCTIONDESCRIPTOR_H_
#define _AKONADI_FILTER_FUNCTIONDESCRIPTOR_H_

#include "config-akonadi-filter.h"

#include "datatype.h"

#include <QString>

namespace Akonadi
{
namespace Filter
{

enum FunctionIdentifiers
{
  // standard functions
  StandardFunctionValueOf = 1,
  StandardFunctionSizeOf = 2,
  StandardFunctionCountOf = 3,
  StandardFunctionExists = 4,
  StandardFunctionDateIn = 5,

  // e-mail related functions
  StandardFunctionAnyEMailAddressIn = 101,
  StandardFunctionAnyEMailAddressDomainIn = 102,
  StandardFunctionAnyEMailAddressLocalPartIn = 103,

  // custom functions
  FunctionDescriptorCustomFirst = 10000
};


/**
 * 
 *
 */
class AKONADI_FILTER_EXPORT FunctionDescriptor
{
public:
  /**
   * Create a function with the specified keywordentifier
   *
   */
  FunctionDescriptor(
      int id,                          //< The id of the function: it should be unique within an application
      const QString &keyword,          //< Unique function keyword: it matches the keyword used in Sieve scripts.
      const QString &name,             //< The token that is displayed in the UI editors.
      int outputDataTypeMask,          //< The mask of possible output data types of this function
      int acceptableInputDataTypeMask  //< The acceptable input data types of this function
    );
  virtual ~FunctionDescriptor();

protected:

  /**
   * The unique id of the functiondescriptor.
   */
  int mId;

  /**
   * The non-localized keywordentifier of the functiondescriptor.
   */
  QString mKeyword;

  /**
   * The localized name of the function (this is what is shown in rule selection combos)
   */
  QString mName;

  /**
   * The possible output types of this function
   */
  int mOutputDataTypeMask;

  /**
   * The acceptable input data types
   */
  int mAcceptableInputDataTypeMask;

public:

  int id() const
  {
    return mId;
  }

  const QString & keyword() const
  {
    return mKeyword;
  }

  const QString & name() const
  {
    return mName;
  }

  int outputDataTypeMask() const
  {
    return mOutputDataTypeMask;
  }

  int acceptableInputDataTypeMask() const
  {
    return mAcceptableInputDataTypeMask;
  }

}; // class FunctionDescriptor

} // namespace Filter

} // namespace Akonadi

#endif //!_AKONADI_FILTER_FUNCTIONDESCRIPTOR_H_
