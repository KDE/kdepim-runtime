/****************************************************************************** *
 *
 *  File : propertybag.h
 *  Created on Wed 05 Aug 2009 01:13:34 by Szymon Stefanek
 *
 *  This file is part of the Akonadi Filtering Framework
 *
 *  Copyright 2009 Szymon Tomasz Stefanek <s.stefanek at gmail dot com>
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

#ifndef _AKONADI_FILTER_PROPERTYBAG_H_
#define _AKONADI_FILTER_PROPERTYBAG_H_

#include <akonadi/filter/config-akonadi-filter.h>

#include <QtCore/QHash>
#include <QtCore/QString>
#include <QtCore/QVariant>


class AKONADI_FILTER_EXPORT PropertyBag
{
public:
  PropertyBag();

  virtual ~PropertyBag();

protected:

  /**
   * A hash table of properties that this component carries.
   * The properties can be arbitrary name=value pairs
   * set by the filtering application.
   */
  QHash< QString, QVariant > mProperties;

public:

  const QHash< QString, QVariant > & allProperties() const
  {
    return mProperties;
  }

  void setAllProperties( const QHash< QString, QVariant > &all );

  QVariant property( const QString &name ) const;
  void setProperty( const QString &name, const QVariant &value );

}; // class PropertyBag

#endif //!_AKONADI_FILTER_PROPERTYBAG_H_
