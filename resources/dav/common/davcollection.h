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

#ifndef DAVCOLLECTION_H
#define DAVCOLLECTION_H

#include <QtCore/QList>
#include <QtCore/QString>

class DavCollection
{
  public:
    typedef QList<DavCollection> List;

    enum ContentType
    {
      Events = 1,
      Todos = 2,
      Contacts = 4
    };
    Q_DECLARE_FLAGS( ContentTypes, ContentType )

    DavCollection();
    DavCollection( const QString &url, const QString &displayName, ContentTypes contentTypes );

    void setUrl( const QString &url );
    QString url() const;

    void setDisplayName( const QString &displayName );
    QString displayName() const;

    void setContentTypes( ContentTypes contentTypes );
    ContentTypes contentTypes() const;

  private:
    QString mUrl;
    QString mDisplayName;
    ContentTypes mContentTypes;
};

Q_DECLARE_OPERATORS_FOR_FLAGS( DavCollection::ContentTypes )

#endif
