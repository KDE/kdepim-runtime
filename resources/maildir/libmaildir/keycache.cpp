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


#include "keycache.h"

#include <QDir>

KeyCache* KeyCache::mSelf = 0;

void KeyCache::addKeys( const QString& dir )
{
  if ( !mNewKeys.contains( dir ) ) {
    mNewKeys.insert( dir, listNew( dir ) );
    //kDebug() << "Added new keys for: " << dir;
  }

  if ( !mCurKeys.contains( dir ) ) {
    mCurKeys.insert( dir, listCurrent( dir ) );
    //kDebug() << "Added cur keys for: " << dir;
  }
}

void KeyCache::refreshKeys( const QString& dir )
{
    mNewKeys.remove( dir );
    mCurKeys.remove( dir );
    addKeys( dir );
}

void KeyCache::addNewKey( const QString& dir, const QString& key )
{
    mNewKeys[dir].insert( key );
  // kDebug() << "Added new key for : " << dir << " key: " << key;
}

void KeyCache::addCurKey( const QString& dir, const QString& key )
{
    mCurKeys[dir].insert( key );
  // kDebug() << "Added cur key for : " << dir << " key:" << key;
}

void KeyCache::removeKey( const QString& dir, const QString& key )
{
  //kDebug() << "Removed new and cur key for: " << dir << " key:" << key;
    mNewKeys[dir].remove( key );
    mCurKeys[dir].remove( key );
}

bool KeyCache::isCurKey( const QString& dir, const QString& key ) const
{
    return mCurKeys.value( dir ).contains( key  );
}

bool KeyCache::isNewKey( const QString& dir, const QString& key ) const
{
    return mNewKeys.value( dir ).contains( key );
}

QSet< QString > KeyCache::listNew( const QString& dir ) const
{
    QDir d( dir + QString::fromLatin1( "/new" ) );
    d.setSorting(QDir::NoSort);
    return d.entryList( QDir::Files ).toSet();
}

QSet< QString > KeyCache::listCurrent( const QString& dir ) const
{
    QDir d( dir + QString::fromLatin1( "/cur"  ) );
    d.setSorting(QDir::NoSort);
    return d.entryList( QDir::Files ).toSet();
}

