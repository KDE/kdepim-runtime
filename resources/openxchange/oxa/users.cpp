/*
    This file is part of oxaccess.

    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "users.h"

#include "usersrequestjob.h"

#include <QtCore/QFile>

#include <kstandarddirs.h>

#include <QtCore/QDebug>

using namespace OXA;

Users* Users::mSelf = 0;

Users::Users()
  : mCurrentUserId( -1 )
{
}

Users::~Users()
{
}

Users* Users::self()
{
  if ( !mSelf )
    mSelf = new Users();

  return mSelf;
}

void Users::init( const QString &identifier )
{
  mIdentifier = identifier;

  loadFromCache();
}

qlonglong Users::currentUserId() const
{
  return mCurrentUserId;
}

User Users::lookupUid( qlonglong uid ) const
{
  return mUsers.value( uid );
}

User Users::lookupEmail( const QString &email ) const
{
  QMapIterator<qlonglong, User> it( mUsers );
  while ( it.hasNext() ) {
    it.next();

    if ( it.value().email() == email )
      return it.value();
  }

  return User();
}

QString Users::cacheFilePath() const
{
  return KStandardDirs::locateLocal( "data", "akonadi/openxchangeresource_" + mIdentifier );
}

void Users::setCurrentUserId( qlonglong id )
{
  mCurrentUserId = id;

  saveToCache();
}

void Users::setUsers( const User::List &users )
{
  mUsers.clear();

  foreach ( const User &user, users )
    mUsers.insert( user.uid(), user );

  saveToCache();
}

void Users::loadFromCache()
{
  QFile cacheFile( cacheFilePath() );
  if ( !cacheFile.open( QIODevice::ReadOnly ) )
    return;

  QDataStream stream( &cacheFile );
  stream.setVersion( QDataStream::Qt_4_6 );

  mUsers.clear();

  stream >> mCurrentUserId;

  qulonglong count;
  stream >> count;

  qlonglong uid;
  QString name;
  QString email;
  for ( qulonglong i = 0; i < count; ++i ) {
    stream >> uid >> name >> email;

    User user;
    user.setUid( uid );
    user.setName( name );
    user.setEmail( email );
    mUsers.insert( user.uid(), user );
  }
}

void Users::saveToCache()
{
  QFile cacheFile( cacheFilePath() );
  if ( !cacheFile.open( QIODevice::WriteOnly ) )
    return;

  QDataStream stream( &cacheFile );
  stream.setVersion( QDataStream::Qt_4_6 );

  // write current user id
  stream << mCurrentUserId;

  // write number of users
  stream << (qulonglong)mUsers.count();

  // write uid, name and email address for each user
  QMapIterator<qlonglong, User> it( mUsers );
  while ( it.hasNext() ) {
    it.next();

    stream << it.value().uid() << it.value().name() << it.value().email();
  }
}

#include "users.moc"
