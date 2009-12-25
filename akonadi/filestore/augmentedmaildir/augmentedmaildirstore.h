/*  This file is part of the KDE project
    Copyright (C) 2009 Kevin Krammer <kevin.krammer@gmx.at>

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

#ifndef AKONADI_AUGMENTEDMAILDIRSTORE_H
#define AKONADI_AUGMENTEDMAILDIRSTORE_H

#include "akonadi-filestore_export.h"

#include <akonadi/filestore/directaccessstore.h>

namespace Akonadi
{

namespace FileStore
{

/**
 */
class AKONADI_FILESTORE_EXPORT AugmentedMailDirStore : public AbstractDirectAccessStore
{
  Q_OBJECT

  public:
    explicit AugmentedMailDirStore( QObject *parent = 0 );

    virtual ~AugmentedMailDirStore();

    virtual void setFileName( const QString &fileName );

    virtual CollectionFetchJob *fetchCollections( const Collection &collection, CollectionFetchJob::Type type = CollectionFetchJob::FirstLevel, const CollectionFetchScope *fetchScope = 0 ) const;

    virtual ItemFetchJob *fetchItems( const Collection &collection, const ItemFetchScope *fetchScope = 0 ) const;

    virtual ItemFetchJob *fetchItem( const Item &item, const ItemFetchScope *fetchScope = 0 ) const;

    virtual ItemCreateJob *createItem( const Item &item, const Collection &collection );

    virtual ItemModifyJob *modifyItem( const Item &item, bool ignorePayload = false );

    virtual ItemDeleteJob *deleteItem( const Item &item );

    virtual StoreCompactJob *compactStore();

  private:
    class Private;
    Private* d;

    Q_PRIVATE_SLOT( d, void onJobsReady( const QList<Job*>& ) )
};

}
}

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
