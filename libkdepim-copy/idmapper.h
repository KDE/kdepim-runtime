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

#ifndef IDMAPPER_H
#define IDMAPPER_H

#include <qmap.h>
#include <qvariant.h>

namespace KPIM {

class IdMapper
{
  public:
    IdMapper();
    ~IdMapper();

    /**
      Loads the map from a file.
     */
    bool load( const QString &fileName );

    /**
      Saves the map to a file.
     */
    bool save( const QString &fileName );

    /**
      Clears the map.
     */
    void clear();

    /**
      Stores the remote id for the given local id.
     */
    void setRemoteId( const QString &localId, const QString &remoteId );

    /**
      Removes the remote id.
     */
    void removeRemoteId( const QString &remoteId );

    /**
      Returns the remote id of the given local id.
     */
    QString remoteId( const QString &localId ) const;

    /**
      Returns the local id for the given remote id.
     */
    QString localId( const QString &remoteId ) const;

    /**
      Returns a string representation of the id pairs, that's usefull
      for debugging.
     */
    QString asString() const;

  private:
    QMap<QString, QVariant> mIdMap;
};

}

#endif
