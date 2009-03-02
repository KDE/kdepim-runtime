/*
    This file is part of KJots.

    Copyright (c) 2008 Stephen Kelly <steveire@gmail.com>

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

#ifndef KJOTSBOOK_H
#define KJOTSBOOK_H

#include <QLatin1String>
#include <QStringList>

#include <KUrl>

#include <akonadi/collection.h>
#include <akonadi/item.h>

#include "kjotsakonadipage.h"

class QDomDocument;

using namespace Akonadi;

/**
This is a placeholder class.
*/
class KJotsBook
{
public:

  enum DataRead {
    ReadFirstLevel,
    ReadCollectionsRecursive
  };

  KJotsBook( const QString &remoteId = QString() );
  ~KJotsBook();

  void setDisplayName( const QString &name );
  QString displayName();

  QString getRemoteId();

  /**
  Returns a list of items in the collection.
  */
  Item::List retrieveItems();

  Collection getCollection();

  Collection::List getCollections( int readType = ReadFirstLevel );

  static KJotsBook getBookshelf();
  void setCollection( Collection col );

  Item getItem( Item item );

  Item addItem( Item item );
  Item updateItem( Item item );
  Item removeItem( Item item );
  Collection addCollection( Collection collection );
  Collection updateCollection( Collection collection );
  Collection removeCollection( Collection collection );

  void removeEntity( const QString &rId );
  void addEntity( const QString &rId );

  // Used by plasma when dropping?
  static QLatin1String mimeType() {
    return QLatin1String( "application/x-vnd.kde-app-kjots-book" );
  }

  static QString bookshelfRemoteId() {
    return QString( "kjots.bookshelf" );
  };

private:
  void removeChildElement( const QString &remoteId );
  void setDomDocument( QDomDocument doc );
  void updateDomNodeOrder();
  Collection writeOut();

  Collection m_collection;
  QString m_rootDataPath;
  QDomDocument m_domDocument;
  Collection::List m_collections;
  Item::List m_items;
  QStringList m_collectionChildOrder;
  QStringList m_itemremoteIds;
  QStringList m_childCollections;

};

#endif


