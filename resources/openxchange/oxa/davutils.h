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

#ifndef OXA_DAVUTILS_H
#define OXA_DAVUTILS_H

#include <QtCore/QString>
#include <QtXml/QDomDocument>
#include <QtXml/QDomElement>
#include <QtXml/QDomNode>

namespace OXA {

/**
 * Namespace that contains methods for creating or modifying DAV XML documents.
 *
 * @author Tobias Koenig <tokoe@kde.org>
 */
namespace DAVUtils
{
  /**
   * Adds a new element with the given @p tag inside the DAV namespace under @p parentNode
   * to the @p document.
   *
   * @return The newly added element.
   */
  QDomElement addDavElement( QDomDocument &document, QDomNode &parentNode, const QString &tag );

  /**
   * Adds a new element with the given @p tag and @p value inside the OX namespace under @p parentNode
   * to the @p document.
   *
   * @return The newly added element.
   */
  QDomElement addOxElement( QDomDocument &document, QDomNode &parentNode, const QString &tag, const QString &text = QString() );

  /**
   * Sets the attribute of @p element inside the OX namespace with the given @p name to @p value.
   */
  void setOxAttribute( QDomElement &element, const QString &name, const QString &value );

  /**
   * Checks whether the response @p document contains an error message.
   * If so, @c true is returned and @p errorText set to the error message.
   */
  bool davErrorOccurred( const QDomDocument &document, QString &errorText );
}

}

#endif
