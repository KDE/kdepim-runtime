/****************************************************************************** *
 *
 *  File : attribute.h
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

#ifndef _AKONADI_FILTER_ATTRIBUTE_H_
#define _AKONADI_FILTER_ATTRIBUTE_H_

#include "config-akonadi-filter.h"

#include <QString>

namespace Akonadi
{
namespace Filter
{

class AKONADI_FILTER_EXPORT Attribute
{
public:
  /**
   * The resulting type of the attribute.
   */
  enum DataType
  {
    DataTypeString,
    DataTypeInteger
  };
public:
  Attribute(
      const QString &id,
      const QString &name,
      const QString &description,
      DataType dataType
    );
  virtual ~Attribute();

protected:

  /**
   * The internal, non-localized identifier of the attribute.
   */
  QString mId;

  /**
   * The localized name of the attribute (this is what is shown in the selection combos)
   */
  QString mName;

  /**
   * The localized description of the attribute (if needed)
   */
  QString mDescription;

  /**
   * The type of this attribute
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

  const QString & name() const
  {
    return mName;
  }

  const QString & description() const
  {
    return mDescription;
  }

}; // class Attribute

} // namespace Filter

} // namespace Akonadi

#endif //!_AKONADI_FILTER_ATTRIBUTE_H_
