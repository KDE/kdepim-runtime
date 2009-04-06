/*
    This file is part of libkabc.
    Copyright (c) 2008 Kevin Krammer <kevin.krammer@gmx.at>

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

#ifndef KABC_RESOURCEAKONADI_H
#define KABC_RESOURCEAKONADI_H

#include "kabc/resourceabc.h"

#include "sharedresourceiface.h"

class KJob;
class QModelIndex;

namespace Akonadi {
  class Collection;
  class Item;
}

namespace KABC {

class ResourceAkonadi : public ResourceABC, public SharedResourceIface
{
  Q_OBJECT

  public:

    ResourceAkonadi();
    explicit ResourceAkonadi( const KConfigGroup &group );
    virtual ~ResourceAkonadi();

    virtual void clear();

    virtual void writeConfig( KConfigGroup &group );

    virtual bool doOpen();
    virtual void doClose();

    virtual Ticket *requestSaveTicket();
    virtual void releaseSaveTicket( Ticket *ticket );

    virtual bool load();
    virtual bool asyncLoad();
    virtual bool save( Ticket *ticket );
    virtual bool asyncSave( Ticket *ticket );

    virtual void insertAddressee( const Addressee &addr );
    virtual void removeAddressee( const Addressee &addr );

    virtual void insertDistributionList( DistributionList *list );
    virtual void removeDistributionList( DistributionList *list );

    virtual bool subresourceActive( const QString &subResource ) const;
    virtual bool subresourceWritable( const QString &subResource ) const;
    virtual QString subresourceLabel( const QString &subResource ) const;
    virtual int subresourceCompletionWeight( const QString &subResource ) const;
    virtual QStringList subresources() const;
    virtual QMap<QString, QString> uidToResourceMap() const;

    void setStoreCollection( const Akonadi::Collection& collection );
    Akonadi::Collection storeCollection() const;

  public Q_SLOTS:
    virtual void setSubresourceActive( const QString &subResource, bool active );
    virtual void setSubresourceCompletionWeight( const QString &subResource, int weight );

  private:
    class Private;
    Private *const d;
};

}

#endif
// kate: space-indent on; indent-width 2; replace-tabs on;
