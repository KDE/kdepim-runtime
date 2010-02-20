/*
    Copyright (c) 2010 Tobias Koenig <tokoe@kde.org>

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

#ifndef DAVUTILS_H
#define DAVUTILS_H

#include <QtCore/QList>
#include <QtXml/QDomElement>

#include <KUrl>

namespace DavUtils
{
  enum Protocol
  {
    CalDav = 0,
    CardDav,
    GroupDav
  };
  
  QString protocolName( Protocol protocol );
  
  class DavUrl
  {
    public:
      typedef QList<DavUrl> List;
      
      DavUrl();
      DavUrl( const KUrl &url, Protocol protocol );
      
      void setUrl( const KUrl &url );
      KUrl url() const;
      
      void setProtocol( Protocol protocol );
      Protocol protocol() const;
      
    private:
      KUrl mUrl;
      Protocol mProtocol;
  };
  
  QDomElement firstChildElementNS( const QDomElement &parent, const QString &namespaceUri, const QString &tagName );

  QDomElement nextSiblingElementNS( const QDomElement &element, const QString &namespaceUri, const QString &tagName );
}

#endif