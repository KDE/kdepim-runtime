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

#ifndef KJOTSRESOURCE_H
#define KJOTSRESOURCE_H

#include <QStringList>

#include <akonadi/collection.h>
#include <akonadi/item.h>
#include <akonadi/resourcebase.h>

#include "kjotspage.h"


class KJotsResource : public Akonadi::ResourceBase,
      public Akonadi::AgentBase::Observer2
{
  Q_OBJECT

public:
  KJotsResource( const QString &id );
  ~KJotsResource();

public Q_SLOTS:
  virtual void configure( WId windowId );

protected Q_SLOTS:
  void retrieveCollections();
  void retrieveItems( const Akonadi::Collection &col );
  bool retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts );

protected:
  virtual void aboutToQuit();

  /**
  Creates a new .kjpage file. Updates the .kjbook file to contain a reference to it.
  */
  virtual void itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection );

  /**
  Changes a .kjpage file. If the change is a change to the ItemAbove,
  updates the .kjbook file to reflect that.
  */
  virtual void itemChanged( const Akonadi::Item &item, const QSet<QByteArray> &parts );

  /**
  Removes a .kjpage file. Updates the .kjbook file to reflect that.
  */
  virtual void itemRemoved( const Akonadi::Item &item );

  /**
  */
  virtual void itemMoved( const Akonadi::Item &item,
                          const Akonadi::Collection &source,
                          const Akonadi::Collection &destination );

  /**
  Creates a new .kjbook file.
  */
  virtual void collectionAdded( const Akonadi::Collection &collection, const Akonadi::Collection &parent );

  /**
  Deletes a .kjbook file. Also removes the reference to @p collection from its parent.
  */
  virtual void collectionRemoved( const Akonadi::Collection &collection );

  /**
  Updates a .kjbook file when the @p collections name is changed. Also reorganises the parent collection
  .kjbook if the entityAbove attribute is changed.
  */
  virtual void collectionChanged( const Akonadi::Collection &collection );

  /**
  */
  virtual void collectionMoved( const Akonadi::Collection &collection,
                          const Akonadi::Collection &source,
                          const Akonadi::Collection &destination );

  Akonadi::Collection::List addToParentRecursive( Akonadi::Collection &book );

  Akonadi::Collection::List getDescendantCollections( Akonadi::Collection &col ) const;
  Akonadi::Item::List getContainedItems( const Akonadi::Collection &col ) const;
  QString getFileUrl( const Akonadi::Collection &col ) const;
  QString getFileUrl( const Akonadi::Item &item ) const;
  KJotsPage getPage( const Akonadi::Item &item, QSet<QByteArray> parts ) const;

private:
  QString m_rootDataPath;

  /**
    @returns the Collection at
  */
  QDomDocument getBook( const Akonadi::Collection &collection ) const;
  QDomDocument getDomDocument( const KJotsPage &page ) const;
  bool removeContentEntry( const Akonadi::Collection &parent, const QString &contentFilename ) const;
  bool addContentEntry( const Akonadi::Collection &parent, const QString &contentFilename ) const;

  bool entityMoved( const Akonadi::Entity &entity,
                    const Akonadi::Collection &source,
                    const Akonadi::Collection &destination );

};

#endif
