/****************************************************************************** *
 *
 *  File : function.h
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

#ifndef _AKONADI_FILTER_FUNCTION_H_
#define _AKONADI_FILTER_FUNCTION_H_

#include "config-akonadi-filter.h"

#include "datatype.h"

#include <QString>

namespace Akonadi
{
namespace Filter
{



/**
 * 
 *
 */
class AKONADI_FILTER_EXPORT Function
{
public:
  /**
   * Create a function with the specified identifier
   *
   */
  Function(
      const QString &id,               //< Unique function identifier: it matches the keyword used in Sieve scripts.
      const QString &name,             //< The token that is displayed in the UI editors.
      DataType outputDataType,         //< The output data type of this function
      int acceptableInputDataTypeMask  //< The acceptable input data types of this function
    );
  virtual ~Function();

protected:

  /**
   * The internal, non-localized identifier of the function.
   */
  QString mId;

  /**
   * The localized name of the function (this is what is shown in rule selection combos)
   */
  QString mName;

  /**
   * The type of this function
   */
  DataType mOutputDataType;

  /**
   * The acceptable input data types
   */
  int mAcceptableInputDataTypeMask;

public:

  DataType outputDataType() const
  {
    return mOutputDataType;
  }

  const QString & id() const
  {
    return mId;
  }

  const QString & name() const
  {
    return mName;
  }

  int acceptableInputDataTypeMask() const
  {
    return mAcceptableInputDataTypeMask;
  }

}; // class Function

} // namespace Filter

} // namespace Akonadi

#endif //!_AKONADI_FILTER_FUNCTION_H_
