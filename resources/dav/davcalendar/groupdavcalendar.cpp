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

#include <QDomDocument>

#include <kio/davjob.h>
#include <kio/jobclasses.h>
#include <kio/job.h>
#include <klocalizedstring.h>
#include <kdebug.h>

#include "groupdavcalendar.h"

groupdavCalendar::groupdavCalendar()
{
  QDomDocument props;
  QDomElement root = props.createElementNS( "DAV:", "propfind" );
  props.appendChild( root );
  QDomElement e1 = props.createElementNS( "DAV:", "prop" );
  root.appendChild( e1 );
  QDomElement e2 = props.createElementNS( "DAV:", "displayname" );
  e1.appendChild( e2 );
  e2 = props.createElementNS( "DAV:", "resourcetype" );
  e1.appendChild( e2 );
  e2 = props.createElementNS( "DAV:", "getetag" );
  e1.appendChild( e2 );
  
  itemsQueries_ << props;
}

bool groupdavCalendar::useReport() const
{
  return false;
}

bool groupdavCalendar::useMultiget() const
{
  return false;
}

QDomDocument groupdavCalendar::collectionsQuery() const
{
  QDomDocument props;
  QDomElement root = props.createElementNS( "DAV:", "propfind" );
  props.appendChild( root );
  QDomElement e1 = props.createElementNS( "DAV:", "prop" );
  root.appendChild( e1 );
  QDomElement e2 = props.createElementNS( "DAV:", "displayname" );
  e1.appendChild( e2 );
  e2 = props.createElementNS( "DAV:", "resourcetype" );
  e1.appendChild( e2 );
  
  return props;
}

QString groupdavCalendar::collectionsXQuery() const
{
  QString xquery( "//*[(local-name()='vevent-collection' or local-name()='vtodo-collection') and namespace-uri()='http://groupdav.org/']/ancestor::*[local-name()='response' and namespace-uri()='DAV:']" );
  return xquery;
}

const QList<QDomDocument>& groupdavCalendar::itemsQueries() const
{
  return itemsQueries_;
}
