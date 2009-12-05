/*
    This file is part of oxaccess.

    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

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

#include "davutils.h"

using namespace OXA;

QDomElement DAVUtils::addDavElement( QDomDocument &document, QDomNode &parentNode, const QString &tag )
{
  const QDomElement element = document.createElementNS( QLatin1String( "DAV:" ), QLatin1String( "D:" ) + tag );
  parentNode.appendChild( element );

  return element;
}

QDomElement DAVUtils::addOxElement( QDomDocument &document, QDomNode &parentNode, const QString &tag, const QString &text )
{
  QDomElement element = document.createElementNS( QLatin1String( "http://www.open-xchange.org" ), QLatin1String( "ox:" ) + tag );

  if ( !text.isEmpty() ) {
    const QDomText textNode = document.createTextNode( text );
    element.appendChild( textNode );
  }

  parentNode.appendChild( element );

  return element;
}

void DAVUtils::setOxAttribute( QDomElement &element, const QString &name, const QString &value )
{
  element.setAttributeNS( QLatin1String( "http://www.open-xchange.org" ), QLatin1String( "ox:" ) + name, value );
}

bool DAVUtils::davErrorOccurred( const QDomDocument &document, QString &errorText )
{
  const QDomElement documentElement = document.documentElement();
  const QDomNodeList propStats = documentElement.elementsByTagNameNS( QLatin1String( "DAV:" ),
                                                                      QLatin1String( "propstat" ) );

  for ( int i = 0; i < propStats.count(); ++i ) {
    const QDomElement propStat = propStats.at( i ).toElement();
    const QDomElement status = propStat.firstChildElement( QLatin1String( "status" ) );
    const QDomElement description = propStat.firstChildElement( QLatin1String( "responsedescription" ) );

    if ( status.text() != QLatin1String( "200" ) ) {
      errorText = description.text();
      return true;
    }
  }

  return false;
}
