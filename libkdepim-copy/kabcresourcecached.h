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

namespace KABC {

class ResourceCached : public Resource
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
      Stores the remote uid for the given local uid.
     */
    void setRemoteUid( const QString &localUid, const QString &remoteUid );

    /**
      Removes the remote uid.
     */
    void removeRemoteUid( const QString &remoteUid );

    /**
      Returns the remote uid of the given local uid.
     */
    QString remoteUid( const QString &localUid ) const;

    /**
      Returns the local uid for the given remote uid.
     */
    QString localUid( const QString &remoteUid ) const;

    bool hasChanges() const;
    void clearChanges();
    void clearChange( const KABC::Addressee& );
    void clearChange( const QString& );

    KABC::Addressee::List addedAddressees() const;
    KABC::Addressee::List changedAddressees() const;
    KABC::Addressee::List deletedAddressees() const;

  protected:
    virtual QString cacheFile() const;
    virtual QString uidMapFile() const;

  private:
    QMap<QString, QVariant> mUidMap;

    QMap<QString, KABC::Addressee> mAddedAddressees;
    QMap<QString, KABC::Addressee> mChangedAddressees;
    QMap<QString, KABC::Addressee> mDeletedAddressees;

    class ResourceCachedPrivate;
    ResourceCachedPrivate *d;
};

}

#endif
