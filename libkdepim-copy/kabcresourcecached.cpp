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
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include <qfile.h>

#include <kabc/vcardconverter.h>
#include <kdebug.h>
#include <klocale.h>
#include <kstandarddirs.h>

#include "kabcresourcecached.h"

using namespace KABC;

ResourceCached::ResourceCached( const KConfig *config )
  : KABC::Resource( config ), mIdMapper( "kabc/uidmaps/" )
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
    if ( mDeletedAddressees.contains( addr.uid() ) ) {
      // it was first removed, then added, so it's an update...
      mDeletedAddressees.remove( addr.uid() );

      mAddrMap.insert( addr.uid(), addr );
      mChangedAddressees.insert( addr.uid(), addr );
      return;
    }

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
  if ( mAddedAddressees.contains( addr.uid() ) ) {
    mAddedAddressees.remove( addr.uid() );
    return;
  }

  if ( mDeletedAddressees.find( addr.uid() ) == mDeletedAddressees.end() )
    mDeletedAddressees.insert( addr.uid(), addr );

  mAddrMap.remove( addr.uid() );
}

void ResourceCached::loadCache()
{
  mAddrMap.clear();

  setIdMapperIdentifier();
  mIdMapper.load();

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
  setIdMapperIdentifier();
  mIdMapper.save();

  // save cache
  QFile file( cacheFile() );
  if ( !file.open( IO_WriteOnly ) )
    return;

  KABC::Addressee::List list = mAddrMap.values();

  KABC::VCardConverter converter;
  QString vCard = converter.createVCards( list );
  file.writeBlock( vCard.utf8(), vCard.utf8().length() );
  file.close();
}

void ResourceCached::cleanUpCache( const KABC::Addressee::List &addrList )
{
  // load cache
  QFile file( cacheFile() );
  if ( !file.open( IO_ReadOnly ) )
    return;


  KABC::VCardConverter converter;
  KABC::Addressee::List list = converter.parseVCards( QString::fromUtf8( file.readAll() ) );
  KABC::Addressee::List::Iterator cacheIt;
  KABC::Addressee::List::ConstIterator it;

  for ( cacheIt = list.begin(); cacheIt != list.end(); ++cacheIt ) {
    bool found = false;
    for ( it = addrList.begin(); it != addrList.end(); ++it ) {
      if ( (*it).uid() == (*cacheIt).uid() )
        found = true;
    }

    if ( !found ) {
      mIdMapper.removeRemoteId( mIdMapper.remoteId( (*cacheIt).uid() ) );
      mAddrMap.remove( (*cacheIt).uid() );
    }
  }

  file.close();
}

KPIM::IdMapper& ResourceCached::idMapper()
{
  return mIdMapper;
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

QString ResourceCached::changesCacheFile( const QString &type ) const
{
  return locateLocal( "cache", "kabc/changescache/" + identifier() + "_" + type );
}

void ResourceCached::saveChangesCache( const QMap<QString, KABC::Addressee> &map, const QString &type )
{
  QFile file( changesCacheFile( type ) );

  const KABC::Addressee::List list = map.values();
  if ( list.isEmpty() ) {
    file.remove();
  } else {
    if ( !file.open( IO_WriteOnly ) ) {
      kdError() << "Can't open changes cache file '" << file.name() << "' for saving." << endl;
      return;
    }

    KABC::VCardConverter converter;
    const QString vCards = converter.createVCards( list );
    QCString content = vCards.utf8();
    file.writeBlock( content, content.length() );
  }
}

void ResourceCached::saveChangesCache()
{
  saveChangesCache( mAddedAddressees, "added" );
  saveChangesCache( mDeletedAddressees, "deleted" );
  saveChangesCache( mChangedAddressees, "changed" );
}

void ResourceCached::loadChangesCache( QMap<QString, KABC::Addressee> &map, const QString &type )
{
  QFile file( changesCacheFile( type ) );
  if ( !file.open( IO_ReadOnly ) )
    return;

  KABC::VCardConverter converter;

  const KABC::Addressee::List list = converter.parseVCards( QString::fromUtf8( file.readAll() ) );
  KABC::Addressee::List::ConstIterator it;
  for ( it = list.begin(); it != list.end(); ++it )
    map.insert( (*it).uid(), *it );

  file.close();
}

void ResourceCached::loadChangesCache()
{
  loadChangesCache( mAddedAddressees, "added" );
  loadChangesCache( mDeletedAddressees, "deleted" );
  loadChangesCache( mChangedAddressees, "changed" );
}

void ResourceCached::setIdMapperIdentifier()
{
  mIdMapper.setIdentifier( type() + "_" + identifier() );
}

#include "kabcresourcecached.moc"
