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

#include "kabc/resource.h"

class KJob;

namespace Akonadi {
  class Collection;
  class DataReference;
  class Item;
}

namespace KABC {

class KABC_EXPORT ResourceAkonadi : public Resource
{
  Q_OBJECT

  public:

    ResourceAkonadi();
    explicit ResourceAkonadi( const KConfigGroup &group );
    virtual ~ResourceAkonadi();
    /**
     *  Call this after you used one of the set... methods
     */
    virtual void init();

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

    void setCollection( const Akonadi::Collection& collection );
    Akonadi::Collection collection() const;

  protected Q_SLOTS:
    void loadResult( KJob *job );
    void saveResult( KJob *job );

  private:
    class Private;
    Private *const d;

    Q_PRIVATE_SLOT( d, void itemAdded( const Akonadi::Item&, const Akonadi::Collection& ) )
    Q_PRIVATE_SLOT( d, void itemChanged( const Akonadi::Item&, const QStringList& ) )
    Q_PRIVATE_SLOT( d, void itemRemoved( const Akonadi::DataReference& ) )
};

}

#endif
