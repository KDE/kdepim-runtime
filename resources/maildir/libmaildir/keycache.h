/*
    Copyright (C) 2012  Andras Mantia <amantia@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#ifndef KEYCACHE_H
#define KEYCACHE_H

/** @brief a cache for the maildir keys (file names in cur/new folders).
 *  It is used to find if a file is in cur or new
 */

#include <QSet>
#include <QHash>

class KeyCache {

public:
  static KeyCache *self()
  {
    if ( !mSelf )
            mSelf = new KeyCache();
    return mSelf;
  }

  /** Find the new and cur keys on the disk for @param dir and add them to the cache */
  void addKeys( const QString& dir );

  /** Refresh the new and cur keys for @param dir */
  void refreshKeys( const QString& dir );

  /** Add a "new" key for @param dir. */
  void addNewKey( const QString& dir, const QString& key );

  /** Add a "cur" key for @param dir. */
  void addCurKey( const QString& dir, const QString& key );

  /** Remove all keys associated with @param dir. */
  void removeKey( const QString& dir, const QString& key );

  /** Check if the @param key is a "cur" key in @param dir */
  bool isCurKey( const QString& dir, const QString& key ) const;

  /** Check if the @param key is a "new" key in @param dir */
  bool isNewKey( const QString& dir, const QString& key ) const;

private:
  KeyCache() {
  }

  QSet<QString> listNew( const QString& dir ) const;

  QSet<QString> listCurrent( const QString& dir ) const;

  QHash< QString, QSet<QString> > mNewKeys;
  QHash< QString, QSet<QString> > mCurKeys;

  static KeyCache* mSelf;

};


#endif // KEYCACHE_H
