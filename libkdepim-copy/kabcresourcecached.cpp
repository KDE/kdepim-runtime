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

#include <qfile.h>

#include <kabc/vcardconverter.h>
#include <kdebug.h>
#include <klocale.h>
#include <kstandarddirs.h>

#include "kabcresourcecached.h"

using namespace KABC;

ResourceCached::ResourceCached( const KConfig *config )
  : KABC::Resource( config )
{
}

ResourceCached::~ResourceCached()
{
}

void ResourceCached::writeConfig( KConfig *config )
{
  KABC::Resource::writeConfig( config );
}

void ResourceCached::insertAddressee( const Addressee &addr )
{
  if ( !mAddrMap.contains( addr.uid() ) ) { // new contact
    if ( mDeletedAddressees.contains( addr.uid() ) )
      mDeletedAddressees.remove( addr.uid() );

    mAddrMap.insert( addr.uid(), addr );
    mAddedAddressees.insert( addr.uid(), addr );
  } else {
    KABC::Addressee oldAddressee = mAddrMap.find( addr.uid() ).data();
    if ( oldAddressee != addr ) {
      mAddrMap.remove( addr.uid() );
      mAddrMap.insert( addr.uid(), addr );
      mChangedAddressees.insert( addr.uid(), addr );
    }
  }
}

void ResourceCached::removeAddressee( const Addressee &addr )
{
  if ( mAddedAddressees.contains( addr.uid() ) )
    mAddedAddressees.remove( addr.uid() );

  if ( mDeletedAddressees.find( addr.uid() ) == mDeletedAddressees.end() )
    mDeletedAddressees.insert( addr.uid(), addr );

  mAddrMap.remove( addr.uid() );
}

void ResourceCached::loadCache()
{
  mUidMap.clear();
  mAddrMap.clear();

  // load uid map
  QFile mapFile( uidMapFile() );
  if ( mapFile.open( IO_ReadOnly ) ) {
    QDataStream stream( &mapFile );
    stream >> mUidMap;
    mapFile.close();
  } else {
    kdError() << "Can't open uid map file '" << mapFile.name() << "'" << endl;
  }

  // load cache
  QFile file( cacheFile() );
  if ( !file.open( IO_ReadOnly ) )
    return;


  KABC::VCardConverter converter;
  KABC::Addressee::List list = converter.parseVCards( QString::fromUtf8( file.readAll() ) );
  KABC::Addressee::List::Iterator it;

  for ( it = list.begin(); it != list.end(); ++it ) {
    (*it).setResource( this );
    (*it).setChanged( false );
    mAddrMap.insert( (*it).uid(), *it );
  }

  file.close();
}

void ResourceCached::saveCache()
{
  // save uid map
  QFile mapFile( uidMapFile() );
  if ( mapFile.open( IO_WriteOnly ) ) {
    QDataStream stream( &mapFile );
    stream << mUidMap;
    mapFile.close();
  } else {
    kdError() << "Can't open uid map file '" << mapFile.name() << "'" << endl;
  }

  // save cache
  QFile file( cacheFile() );
  if ( !file.open( IO_WriteOnly ) )
    return;

  KABC::Addressee::List list = mAddrMap.values();

  KABC::VCardConverter converter;
  QString vCard = converter.createVCards( list );
  file.writeBlock( vCard.utf8() );
  file.close();
}

void ResourceCached::cleanUpCache( const KABC::Addressee::List &addrList )
{
  // load uid map
  QFile mapFile( uidMapFile() );
  if ( mapFile.open( IO_ReadOnly ) ) {
    QDataStream stream( &mapFile );
    stream >> mUidMap;
    mapFile.close();
  } else {
    kdError() << "Can't open uid map file '" << mapFile.name() << "'" << endl;
  }

  // load cache
  QFile file( cacheFile() );
  if ( !file.open( IO_ReadOnly ) )
    return;


  KABC::VCardConverter converter;
  KABC::Addressee::List list = converter.parseVCards( QString::fromUtf8( file.readAll() ) );
  KABC::Addressee::List::Iterator it;

  for ( it = list.begin(); it != list.end(); ++it ) {
    if ( !addrList.contains( *it ) )
      mAddrMap.remove( (*it).uid() );
  }

  file.close();
}

void ResourceCached::setRemoteUid( const QString &localUid, const QString &remoteUid )
{
  mUidMap.insert( localUid, remoteUid );
}

void ResourceCached::removeRemoteUid( const QString &remoteUid )
{
  QMap<QString, QVariant>::Iterator it;
  for ( it = mUidMap.begin(); it != mUidMap.end(); ++it )
    if ( it.data().toString() == remoteUid ) {
      mUidMap.remove( it );
      return;
    }
}

QString ResourceCached::remoteUid( const QString &localUid ) const
{
  QMap<QString, QVariant>::ConstIterator it;
  it = mUidMap.find( localUid );

  if ( it != mUidMap.end() )
    return it.data().toString();
  else
    return QString::null;
}

QString ResourceCached::localUid( const QString &remoteUid ) const
{
  QMap<QString, QVariant>::ConstIterator it;
  for ( it = mUidMap.begin(); it != mUidMap.end(); ++it )
    if ( it.data().toString() == remoteUid )
      return it.key();

  return QString::null;
}

bool ResourceCached::hasChanges() const
{
  return !( mAddedAddressees.isEmpty() &&
            mChangedAddressees.isEmpty() &&
            mDeletedAddressees.isEmpty() );
}

void ResourceCached::clearChanges()
{
  mAddedAddressees.clear();
  mChangedAddressees.clear();
  mDeletedAddressees.clear();
}

void ResourceCached::clearChange( const KABC::Addressee &addr )
{
  mAddedAddressees.remove( addr.uid() );
  mChangedAddressees.remove( addr.uid() );
  mDeletedAddressees.remove( addr.uid() );
}

void ResourceCached::clearChange( const QString &uid )
{
  mAddedAddressees.remove( uid );
  mChangedAddressees.remove( uid );
  mDeletedAddressees.remove( uid );
}

KABC::Addressee::List ResourceCached::addedAddressees() const
{
  return mAddedAddressees.values();
}

KABC::Addressee::List ResourceCached::changedAddressees() const
{
  return mChangedAddressees.values();
}

KABC::Addressee::List ResourceCached::deletedAddressees() const
{
  return mDeletedAddressees.values();
}

QString ResourceCached::cacheFile() const
{
  return locateLocal( "cache", "kabc/kresources/" + identifier() );
}

QString ResourceCached::uidMapFile() const
{
  return locateLocal( "data", "kabc/uidmaps/" + type() + "/" + identifier() );
}

#include "kabcresourcecached.moc"
