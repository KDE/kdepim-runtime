/****************************************************************************** *
 *
 *  File : datamember.h
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

#ifndef _AKONADI_FILTER_DATAMEMBER_H_
#define _AKONADI_FILTER_DATAMEMBER_H_

#include "config-akonadi-filter.h"

#include <QString>

#include "datatype.h"

namespace Akonadi
{
namespace Filter
{

/**
 *
 */
class AKONADI_FILTER_EXPORT DataMember
{
public:
  /**
   * Create a function with the specified identifier
   *
   */
  DataMember(
      const QString &keyword,          //< Unique data member keyword
      const QString &name,             //< The token that is displayed in the UI editors.
      DataType dataType                //< The output data type of this function
    );
  virtual ~DataMember();

protected:

  /**
   * The non-localized keyword of the data member. Must be unique
   */
  QString mKeyword;

  /**
   * The localized name of the function (this is what is shown in rule combos)
   */
  QString mName;

  /**
   * The type of this function
   */
  DataType mDataType;

public:

  DataType dataType() const
  {
    return mDataType;
  }

  const QString & keyword() const
  {
    return mKeyword;
  }

  const QString & name() const
  {
    return mName;
  }

}; // class DataMember

} // namespace Filter

} // namespace Akonadi

#endif //!_AKONADI_FILTER_DATAMEMBER_H_
