/*
    Copyright (c) 2009 Grégory Oestreicher <greg@kamago.net>

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

#ifndef DAVPROTOCOLBASE_H
#define DAVPROTOCOLBASE_H

#include "davcollection.h"

#include <QtCore/QList>
#include <QtXml/QDomDocument>

class QStringList;

class DavProtocolBase
{
  public:
    virtual ~DavProtocolBase();
    virtual bool useReport() const = 0;
    virtual bool useMultiget() const = 0;
    virtual QDomDocument collectionsQuery() const = 0;
    virtual QString collectionsXQuery() const = 0;
    virtual QList<QDomDocument> itemsQueries() const = 0;
    virtual QDomDocument itemsReportQuery( const QStringList &urls ) const;

    /**
     * Returns the possible content types for the collection that
     * is described by the passed PROPFIND @response.
     */
    virtual DavCollection::ContentTypes collectionContentTypes( const QDomElement &response ) const = 0;
};

#endif
