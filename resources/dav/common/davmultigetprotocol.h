/*
    Copyright (c) 2010 Grégory Oestreicher <greg@kamago.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#ifndef DAVMULTIGETPROTOCOL_H
#define DAVMULTIGETPROTOCOL_H

#include "davprotocolbase.h"

/**
 * @short Base class for protocols that implement multiget capabilities
 */
class DavMultigetProtocol : public DavProtocolBase
{
  public:
    /**
     * Destroys the DAV protocol
     */
    virtual ~DavMultigetProtocol();

    /**
     * Returns the XML document that represents a MULTIGET DAV query to
     * list all DAV resources with the given @p urls.
     */
    virtual QDomDocument itemsReportQuery( const QStringList &urls ) const = 0;

    /**
     * Returns the namespace used by protocol-specific elements found in responses.
     */
    virtual QString responseNamespace() const = 0;

    /**
     * Returns the tag name of data elements found in responses.
     */
    virtual QString dataTagName() const = 0;
};

#endif
