/*
 * Copyright (c) 2009  Igor Trindade Oliveira <igor_trindade@yahoo.com.br>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <akonadi/item.h>
#include <akonadi/collection.h>

#include <akonadi/xml/xmldocument.h>

using namespace Akonadi;

class DAO : public QObject
{
  public:
    DAO();
  
  private:
    XmlDocument *doc;

  public Q_SLOTS:
    void openXmlDocument(const QString &path);
    Collection getCollectionByRemoteId(const QString &rid);
    Item getItemByRemoteId(const QString &rid);
};
