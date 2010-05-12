/*  This file is part of the KDE project
    Copyright (C) 2010 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.net
    Author: Kevin Krammer, krake@kdab.com

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef AKONADI_FILESTORE_ABSTRACTLOCALSTORE_H
#define AKONADI_FILESTORE_ABSTRACTLOCALSTORE_H

#include "storeinterface.h"

#include <Akonadi/Collection>
#include <Akonadi/Item>

#include <QObject>

template <typename T> class QList;

namespace Akonadi
{

namespace FileStore
{

/**
 */
class AKONADI_FILESTORE_EXPORT AbstractLocalStore : public QObject, public StoreInterface
{
  Q_OBJECT

  public:
    AbstractLocalStore();
    ~AbstractLocalStore();

    virtual void setPath( const QString &path );
    QString path() const;

    Collection topLevelCollection() const;

    CollectionCreateJob *createCollection( const Collection &collection, const Collection &targetParent );

    CollectionFetchJob *fetchCollections( const Collection &collection, CollectionFetchJob::Type type = CollectionFetchJob::FirstLevel ) const;

    CollectionDeleteJob *deleteCollection( const Collection &collection );

    CollectionModifyJob *modifyCollection( const Collection &collection );

    CollectionMoveJob *moveCollection( const Collection &collection, const Collection &targetParent );

    ItemFetchJob *fetchItems( const Collection &collection ) const;

    ItemFetchJob *fetchItem( const Item &item ) const;

    ItemCreateJob *createItem( const Item &item, const Collection &collection );

    ItemModifyJob *modifyItem( const Item &item );

    ItemDeleteJob *deleteItem( const Item &item );

    ItemMoveJob *moveItem( const Item &item, const Collection &targetParent );

    StoreCompactJob *compactStore();

  protected: // job processing
    virtual void processJob( Job *job ) = 0;

    Job *currentJob() const;

    void notifyError( int errorCode, const QString &errorText ) const;

    void notifyCollectionsProcessed( const Collection::List &collections ) const;

    void notifyItemsProcessed( const Item::List &items ) const;

  protected: // template methods
    void setTopLevelCollection( const Collection &collection );

    virtual void checkCollectionCreate( CollectionCreateJob *job, int &errorCode, QString &errorText ) const;

    virtual void checkCollectionDelete( CollectionDeleteJob *job, int &errorCode, QString &errorText ) const;

    virtual void checkCollectionFetch( CollectionFetchJob *job, int &errorCode, QString &errorText ) const;

    virtual void checkCollectionModify( CollectionModifyJob *job, int &errorCode, QString &errorText ) const;

    virtual void checkCollectionMove( CollectionMoveJob *job, int &errorCode, QString &errorText ) const;

    virtual void checkItemCreate( ItemCreateJob *job, int &errorCode, QString &errorText ) const;

    virtual void checkItemDelete( ItemDeleteJob *job, int &errorCode, QString &errorText ) const;

    virtual void checkItemFetch( ItemFetchJob *job, int &errorCode, QString &errorText ) const;

    virtual void checkItemModify( ItemModifyJob *job, int &errorCode, QString &errorText ) const;

    virtual void checkItemMove( ItemMoveJob *job, int &errorCode, QString &errorText ) const;

    virtual void checkStoreCompact( StoreCompactJob *job, int &errorCode, QString &errorText ) const;

  private:
    class Private;
    Private *const d;

    Q_PRIVATE_SLOT( d, void processJobs( const QList<Job*> &jobs ) )
};

}
}

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
