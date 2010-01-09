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

#ifndef DAV_IMPLEMENTATION_H
#define DAV_IMPLEMENTATION_H

#include <QDomDocument>
#include <QString>
#include <QList>

class davImplementation
{
  public:
    virtual bool useReport() const = 0;
    virtual bool useMultiget() const = 0;
    virtual QDomDocument collectionsQuery() const = 0;
    virtual QString collectionsXQuery() const = 0;
    virtual const QList<QDomDocument>& itemsQueries() const = 0;
    virtual QDomDocument itemsReportQuery( const QStringList &urls ) const
      { return QDomDocument(); }
};

#endif