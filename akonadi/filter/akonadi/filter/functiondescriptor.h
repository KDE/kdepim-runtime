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

#include <akonadi/filter/config-akonadi-filter.h>

#include <akonadi/filter/datatype.h>

#include <QtCore/QString>

namespace Akonadi
{
namespace Filter
{

enum FunctionIdentifiers
{
  // standard functions
  FunctionValueOf = 1,
  FunctionSizeOf,
  FunctionCountOf,
  FunctionExists,
  FunctionDateIn,

  // custom functions
  FunctionCustomFirst = 1000
};


/**
 * This class describes a function to be applied on the result of the extraction of a data member
 * (via DataMemberDescriptor) from a Data object.
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
      DataType outputDataType,         //< The output data type of this function
      int outputFeatureMask,           //< The features provided by this function
      int acceptableInputDataTypeMask, //< The acceptable input data types of this function
      int requiredInputFeatureMask = 0 //< The required input data member feature mask, 0 means none
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
   * The output type of this function
   */
  DataType mOutputDataType;

  /**
   * The mask of features provided by this function.
   */
  int mOutputFeatureMask;

  /**
   * The acceptable input data types
   */
  int mAcceptableInputDataTypeMask;

  /**
   * The mask of required features (in input data)
   */
  int mRequiredInputFeatureMask;

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

  DataType outputDataType() const
  {
    return mOutputDataType;
  }

  int outputFeatureMask() const
  {
    return mOutputFeatureMask;
  }

  int acceptableInputDataTypeMask() const
  {
    return mAcceptableInputDataTypeMask;
  }

  int requiredInputFeatureMask() const
  {
    return mRequiredInputFeatureMask;
  }


}; // class FunctionDescriptor

} // namespace Filter

} // namespace Akonadi

#endif //!_AKONADI_FILTER_FUNCTIONDESCRIPTOR_H_
