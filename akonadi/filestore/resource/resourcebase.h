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

#ifndef AKONADI_FILESTORE_RESOURCEBASE_H
#define AKONADI_FILESTORE_RESOURCEBASE_H

#include "akonadi-filestore_export.h"

#include <akonadi/collection.h>
#include <akonadi/item.h>
#include <akonadi/resourcebase.h>

class KJob;

namespace Akonadi
{
namespace FileStore
{
class StoreInterface;

class AKONADI_FILESTORE_EXPORT ResourceBase : public Akonadi::ResourceBase, public Akonadi::AgentBase::Observer
{
  Q_OBJECT

  public:
    ResourceBase( const QString &id );
    ~ResourceBase();

  protected:
    StoreInterface *mStore;

    QSet<QByteArray> mPayloadParts;

  protected:
    virtual void itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection );
    virtual void itemChanged( const Akonadi::Item &item, const QSet<QByteArray> &parts );
    virtual void itemRemoved( const Akonadi::Item &item );

  protected Q_SLOTS:
    void retrieveCollections();
    void retrieveItems( const Akonadi::Collection &col );
    bool retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts );

    void collectionsReceived( const Akonadi::Collection::List &collections );
    virtual void collectionFetchDone( KJob *job );

    void itemsReceived( const Akonadi::Item::List &items );
    virtual void itemFetchDone( KJob *job );

    virtual void itemCreateDone( KJob *job );

    virtual void itemModifyDone( KJob *job );

    virtual void itemDeleteDone( KJob *job );

    void compactStore( const QVariant &parameter );
    virtual void storeCompactDone( KJob *job );
};

}
}

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
