/*
    This file is part of libkdepim.
    Copyright (c) 2004 Tobias Koenig <tokoe@kde.org>

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
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

#ifndef KABC_RESOURCECACHED_H
#define KABC_RESOURCECACHED_H

#include <kabc/resource.h>
#include <kdepimmacros.h>

#include "libemailfunctions/idmapper.h"

namespace KABC {

class KDE_EXPORT ResourceCached : public Resource
{
  Q_OBJECT

  public:
    ResourceCached( const KConfig* );
    ~ResourceCached();

    /**
      Writes the resource specific config to file.
     */
    virtual void writeConfig( KConfig *config );

    /**
      Insert an addressee into the resource.
     */
    virtual void insertAddressee( const Addressee& );

    /**
      Removes an addressee from resource.
     */
    virtual void removeAddressee( const Addressee& addr );

    void loadCache();
    void saveCache();
    void cleanUpCache( const KABC::Addressee::List &list );

    /**
      Returns a reference to the id mapper.
     */
    KPIM::IdMapper& idMapper();

    bool hasChanges() const;
    void clearChanges();
    void clearChange( const KABC::Addressee& );
    void clearChange( const QString& );

    KABC::Addressee::List addedAddressees() const;
    KABC::Addressee::List changedAddressees() const;
    KABC::Addressee::List deletedAddressees() const;

  protected:
    virtual QString cacheFile() const;

    /**
      Functions for keeping the changes persistent.
     */
    virtual QString changesCacheFile( const QString& ) const;
    void loadChangesCache( QMap<QString, KABC::Addressee>&, const QString& );
    void loadChangesCache();
    void saveChangesCache( const QMap<QString, KABC::Addressee>&, const QString& );
    void saveChangesCache();

    void setIdMapperIdentifier();

  private:
    KPIM::IdMapper mIdMapper;

    QMap<QString, KABC::Addressee> mAddedAddressees;
    QMap<QString, KABC::Addressee> mChangedAddressees;
    QMap<QString, KABC::Addressee> mDeletedAddressees;

    class ResourceCachedPrivate;
    ResourceCachedPrivate *d;
};

}

#endif
