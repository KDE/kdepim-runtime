/****************************************************************************** *
 *
 *  File : property.h
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

#ifndef _AKONADI_FILTER_PROPERTY_H_
#define _AKONADI_FILTER_PROPERTY_H_

#include "config-akonadi-filter.h"

#include "datatype.h"

#include <QString>

namespace Akonadi
{
namespace Filter
{

class AKONADI_FILTER_EXPORT Property
{
public:
  Property(
      const QString &id,
      const QString &shortName,
      const QString &longName,
      DataType dataType
    );
  virtual ~Property();

protected:

  /**
   * The internal, non-localized identifier of the property.
   */
  QString mId;

  /**
   * The localized name of the property (this is what is shown in rule descriptions)
   */
  QString mShortName;

  /**
   * The localized long name of the property (this is what is shown in the selection combos)
   */
  QString mLongName;

  /**
   * The type of this property
   */
  DataType mDataType;

public:

  DataType dataType() const
  {
    return mDataType;
  }

  const QString & id() const
  {
    return mId;
  }

  const QString & shortName() const
  {
    return mShortName;
  }

  const QString & longName() const
  {
    return mLongName;
  }

}; // class Property

} // namespace Filter

} // namespace Akonadi

#endif //!_AKONADI_FILTER_PROPERTY_H_
