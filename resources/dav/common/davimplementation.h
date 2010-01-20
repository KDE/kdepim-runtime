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

#ifndef DAVIMPLEMENTATION_H
#define DAVIMPLEMENTATION_H

#include <QtCore/QList>
#include <QtXml/QDomDocument>

class QStringList;

class DavImplementation
{
  public:
    virtual ~DavImplementation();
    virtual bool useReport() const = 0;
    virtual bool useMultiget() const = 0;
    virtual QDomDocument collectionsQuery() const = 0;
    virtual QString collectionsXQuery() const = 0;
    virtual QList<QDomDocument> itemsQueries() const = 0;
    virtual QDomDocument itemsReportQuery( const QStringList &urls ) const;
};

#endif
