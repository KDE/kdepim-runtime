/*
    Copyright (c) 2008 Kevin Krammer <kevin.krammer@gmx.at>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#ifndef KDEPIM_DISTRIBUTIONLISTCONVERTER_H
#define KDEPIM_DISTRIBUTIONLISTCONVERTER_H

#include "kdepim_export.h"

namespace KABC {
  class DistributionList;
  class Resource;
}

namespace KPIM {

class DistributionList;

/**
 * Helper for converting between KPIM::DistributionList and KABC::DistributionList.
 *
 * @author Kevin Krammer
 *
 * @since 4.2
 */
class KDEPIM_EXPORT DistributionListConverter
{
  public:
    /**
     * Creates a converter instance for a given @p resource
     *
     * @param resource the resource which holds the addressees
     */
    explicit DistributionListConverter( KABC::Resource *resource );

    /**
     * Destroys the instance
     */
    ~DistributionListConverter();

    /**
     * Returns a KABC list filled with the entries of the given KPIM list
     *
     * @param pimList the KPIM::DistributionList to convert from
     * @return a new instance of a KABC::DistributionList, its resource set to
     *         the one passed to the converter's constructor
     */
    KABC::DistributionList *convertToKABC( const KPIM::DistributionList &pimList );

    /**
     * Returns a KPIM list filled with the entries of the given KABC list
     *
     * @param kabcList the KABC::DistributionList to convert from
     * @return a new instance of a KPIM::DistributionList
     */
    KPIM::DistributionList convertFromKABC( const KABC::DistributionList *kabcList );

    /**
     * Updates a KABC list using a matching KPIM list
     *
     * @param pimList the KPIM::Distribution list to take the data from
     * @param kabcList the KABC::Distribution list to updateKABC
     * @return @c false if the identifiers don't match, otherwise @c true
     */
    bool updateKABC( const KPIM::DistributionList &pimList, KABC::DistributionList *kabcList );

  private:
    //@cond PRIVATE
    class Private;
    Private * const d;
    //@endcond
};

}

#endif
// kate: space-indent on; indent-width 2; replace-tabs on;
