/****************************************************************************** *
 *
 *  File : datamemberdescriptor.h
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

#ifndef _AKONADI_FILTER_DATAMEMBERDESCRIPTOR_H_
#define _AKONADI_FILTER_DATAMEMBERDESCRIPTOR_H_

#include <akonadi/filter/config-akonadi-filter.h>

#include <QtCore/QString>

#include <akonadi/filter/datatype.h>

namespace Akonadi
{
namespace Filter
{

enum DataMemberIdentifiers
{
  // custom data members start here
  DataMemberCustomFirst = 1000
};

/**
 * A descriptor of a data member that can be extracted from the Data object.
 *
 * A data member has a numeric identifier which must be unique within a
 * single filter domain (and more specifically, within a single ComponentFactory class).
 * The identifier constants should start at DataMemberCustomFirst.
 *
 * A data member has an unique keyword that is used as textual rappresentation
 * in the storage layer. The keyword must be made of alphanumeric characters only
 * and can't contain spaces. Examples of keywords are "from", "to", "cc", "body" etc...
 *
 * A data member has a (localized) name which is used in the UI filter editors.
 * The name has no restrictions on its composition (however, using a linefeed inside
 * a name is not reccomended). Examples of names are "cc address list", "message body" etc...
 *
 * A data member has a DataType, which is used to match the FunctionDescriptor
 * objects on top of it: only functions that will accept the specific data type
 * will be applicable.
 *
 * A data member, finally, has a feature mask: an application defined
 * mask of bits which is used as additional filter for the functions that can
 * be applied on top of it. Certain functions will require that a specific set
 * of bits is set in the data member in order to apply.
 *
 * For instance, the "from" field of an e-mail message may provide a "ContainsAddresses"
 * bit which can be required by the "AddressIn" function.
 *
 * The default Data object doesn't suppor any default data member: the application
 * is responsible of defining them.
 * However, the Akonadi filtering library contains a DataRfc822 implementation
 * which provides several data members applicable to email messages.
 */
class AKONADI_FILTER_EXPORT DataMemberDescriptor
{
public:
  /**
   * Create a data member.
   *
   * @param id The unique identifier of this data member. This is an application defined constant
   *        which should be greater or equal to DataMemberCustomFirst.
   * @param keyword The unique keyword for this data member. This will be used for storing
   *        the member inside sieve scripts.
   * @param dataType The data type of this member.
   * @param providedFeatureMask An application defined mask of bits which will be used to
   *        match this data member with the functions that can be applied on top of it.
   *        If no features are provided then only functions that match the data type and
   *        that require no features will be applicable to this data member.
   */
  DataMemberDescriptor(
      int id,                          //< The unique id of the data member
      const QString &keyword,          //< Unique data member keyword
      const QString &name,             //< The token that is displayed in the UI editors.
      DataType dataType,               //< The output data type of this function
      int featureMask = 0              //< The user-defined features that this data member provides
    );
  virtual ~DataMemberDescriptor();

protected:

  /**
   * The unique id of the data member descriptor.
   */
  int mId;

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

  /**
   * The mask of application defined features that this data member provides.
   * Certain functions will require that a set of bits is present in a data member
   * in order to be applied on top of it.
   */
  int mFeatureMask;

public:

  /**
   * Returns the data type of this member.
   */
  DataType dataType() const
  {
    return mDataType;
  }

  /**
   * Returns the unique identifier of this data member.
   */
  int id() const
  {
    return mId;
  }

  /**
   * Returns the unique keyword of this data member.
   */
  const QString & keyword() const
  {
    return mKeyword;
  }

  /**
   * Returns the localized name of this data member.
   */
  const QString & name() const
  {
    return mName;
  }

  /**
   * Returns the feature mask of this data member.
   */
  int featureMask() const
  {
    return mFeatureMask;
  }

}; // class DataMemberDescriptor

} // namespace Filter

} // namespace Akonadi

#endif //!_AKONADI_FILTER_DATAMEMBERDESCRIPTOR_H_
