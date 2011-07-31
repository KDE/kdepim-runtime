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

#include "davcollection.h"

DavCollection::DavCollection()
{
}

DavCollection::DavCollection( DavUtils::Protocol protocol, const QString &url, const QString &displayName, ContentTypes contentTypes )
  : mProtocol( protocol ), mUrl( url ), mDisplayName( displayName ), mContentTypes( contentTypes ), mPrivileges( All )
{
}

void DavCollection::setProtocol( DavUtils::Protocol protocol )
{
  mProtocol = protocol;
}

DavUtils::Protocol DavCollection::protocol() const
{
  return mProtocol;
}

void DavCollection::setUrl( const QString &url )
{
  mUrl = url;
}

QString DavCollection::url() const
{
  return mUrl;
}

void DavCollection::setDisplayName( const QString &displayName )
{
  mDisplayName = displayName;
}

QString DavCollection::displayName() const
{
  return mDisplayName;
}

void DavCollection::setColor( const QColor &color )
{
  mColor = color;
}

QColor DavCollection::color() const
{
  return mColor;
}

void DavCollection::setContentTypes( ContentTypes contentTypes )
{
  mContentTypes = contentTypes;
}

DavCollection::ContentTypes DavCollection::contentTypes() const
{
  return mContentTypes;
}

void DavCollection::setPrivileges( Privileges privs )
{
  mPrivileges = privs;
}

DavCollection::Privileges DavCollection::privileges() const
{
  return mPrivileges;
}

