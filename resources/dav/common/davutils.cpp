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

#include "davutils.h"

QDomElement DavUtils::firstChildElementNS( const QDomElement &parent, const QString &namespaceUri, const QString &tagName )
{
  for ( QDomNode child = parent.firstChild(); !child.isNull(); child = child.nextSibling() ) {
    if ( child.isElement() ) {
      const QDomElement elt = child.toElement();
      if ( tagName.isEmpty() || (elt.tagName() == tagName && elt.namespaceURI() == namespaceUri) )
        return elt;
    }
  }

  return QDomElement();
}

QDomElement DavUtils::nextSiblingElementNS( const QDomElement &element, const QString &namespaceUri, const QString &tagName )
{
  for ( QDomNode sib = element.nextSibling(); !sib.isNull(); sib = sib.nextSibling() ) {
    if ( sib.isElement() ) {
      const QDomElement elt = sib.toElement();
      if ( tagName.isEmpty() || (elt.tagName() == tagName && elt.namespaceURI() == namespaceUri) )
        return elt;
    }
  }

  return QDomElement();
}
