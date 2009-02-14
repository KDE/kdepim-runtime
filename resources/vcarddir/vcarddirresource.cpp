/*
    Copyright (c) 2008 Tobias Koenig <tokoe@kde.org>
    Copyright (c) 2008 Bertjan Broeksema <b.broeksema@kdemail.net>

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

#include "vcarddirresource.h"

#include "settingsadaptor.h"
#include "settingsdialog.h"

#include <QtCore/QDir>
#include <QtCore/QDirIterator>
#include <QtCore/QFile>

#include <akonadi/changerecorder.h>
#include <akonadi/itemfetchscope.h>

using namespace Akonadi;

VCardDirResource::VCardDirResource( const QString &id )
  : ResourceBase( id )
{
  // setup the resource
  new SettingsAdaptor( Settings::self() );
  QDBusConnection::sessionBus().registerObject( QLatin1String( "/Settings" ),
                            Settings::self(), QDBusConnection::ExportAdaptors );

  changeRecorder()->itemFetchScope().fetchFullPayload();
}

VCardDirResource::~VCardDirResource()
{
  // clear cache
  mAddressees.clear();
}

void VCardDirResource::aboutToQuit()
{
  Settings::self()->writeConfig();
}

void VCardDirResource::configure( WId windowId )
{
  SettingsDialog dlg( windowId );
  if ( dlg.exec() ) {
    clearCache();
    initializeVCardDirectory();
    loadAddressees();
  }
}

bool VCardDirResource::loadAddressees()
{
  mAddressees.clear();

  QDirIterator it( vCardDirectoryName() );
  while ( it.hasNext() ) {
    it.next();
    if ( it.fileName() != "." && it.fileName() != ".." && it.fileName() != "WARNING_README.txt" ) {
      QFile file( it.filePath() );
      file.open( QIODevice::ReadOnly );
      const QByteArray data = file.readAll();
      file.close();

      const KABC::Addressee addr = mConverter.parseVCard( data );
      if ( !addr.isEmpty() ) {
        mAddressees.insert( addr.uid(), addr );
      }
    }
  }

  emit status( Idle );

  return true;
}

bool VCardDirResource::retrieveItem( const Akonadi::Item &item, const QSet<QByteArray>& )
{
  const QString remoteId = item.remoteId();
  if ( !mAddressees.contains( remoteId ) ) {
    emit error( QString( "Contact with uid '%1' not found!" ).arg( remoteId ) );
    return false;
  }

  Item newItem( item );
  newItem.setPayload<KABC::Addressee>( mAddressees.value( remoteId ) );
  itemRetrieved( newItem );

  return true;
}

void VCardDirResource::itemAdded( const Akonadi::Item &item, const Akonadi::Collection& )
{
  if ( Settings::self()->readOnly() ) {
    emit error( i18n( "Trying to write to a read-only directory: '%1'", vCardDirectoryName() ) );
    cancelTask();
    return;
  }

  KABC::Addressee addressee;
  if ( item.hasPayload<KABC::Addressee>() )
    addressee  = item.payload<KABC::Addressee>();

  if ( !addressee.isEmpty() ) {
    // add it to the cache...
    mAddressees.insert( addressee.uid(), addressee );

    // ... and write it through to the file system
    const QByteArray data = mConverter.createVCard( addressee );

    QFile file( vCardDirectoryFileName( addressee.uid() ) );
    file.open( QIODevice::WriteOnly );
    file.write( data );
    file.close();

    // report everything ok
    Item newItem( item );
    newItem.setRemoteId( addressee.uid() );
    changeCommitted( newItem );

  } else {
    changeProcessed();
  }
}

void VCardDirResource::itemChanged( const Akonadi::Item &item, const QSet<QByteArray>& )
{
  if ( Settings::self()->readOnly() ) {
    emit error( i18n( "Trying to write to a read-only directory: '%1'", vCardDirectoryName() ) );
    cancelTask();
    return;
  }

  KABC::Addressee addressee;
  if ( item.hasPayload<KABC::Addressee>() )
    addressee  = item.payload<KABC::Addressee>();

  if ( !addressee.isEmpty() ) {
    // change it in the cache...
    mAddressees.insert( addressee.uid(), addressee );

    // ... and write it through to the file system
    const QByteArray data = mConverter.createVCard( addressee );

    QFile file( vCardDirectoryFileName( addressee.uid() ) );
    file.open( QIODevice::WriteOnly );
    file.write( data );
    file.close();

    Item newItem( item );
    newItem.setRemoteId( addressee.uid() );
    changeCommitted( newItem );

  } else {
    changeProcessed();
  }
}

void VCardDirResource::itemRemoved( const Akonadi::Item &item )
{
  if ( Settings::self()->readOnly() ) {
    emit error( i18n( "Trying to write to a read-only directory: '%1'", vCardDirectoryName() ) );
    cancelTask();
    return;
  }

  // remove it from the cache...
  if ( mAddressees.contains( item.remoteId() ) )
    mAddressees.remove( item.remoteId() );

  // ... and remove it from the file system
  QFile::remove( vCardDirectoryFileName( item.remoteId() ) );

  changeProcessed();
}

void VCardDirResource::retrieveCollections()
{
  Collection c;
  c.setParent( Collection::root() );
  c.setRemoteId( vCardDirectoryName() );
  c.setName( name() );
  QStringList mimeTypes;
  mimeTypes << KABC::Addressee::mimeType();
  c.setContentMimeTypes( mimeTypes );
  if ( Settings::self()->readOnly() ) {
    c.setRights( Collection::CanChangeCollection );
  } else {
    Collection::Rights rights;
    rights |= Collection::CanChangeItem;
    rights |= Collection::CanCreateItem;
    rights |= Collection::CanDeleteItem;
    rights |= Collection::CanChangeCollection;
    c.setRights( rights );
  }
  Collection::List list;
  list << c;
  collectionsRetrieved( list );
}

void VCardDirResource::retrieveItems( const Akonadi::Collection& )
{
  Item::List items;

  foreach ( const KABC::Addressee &addressee, mAddressees ) {
    Item item;
    item.setRemoteId( addressee.uid() );
    item.setMimeType( KABC::Addressee::mimeType() );
    items.append( item );
  }

  itemsRetrieved( items );
}

QString VCardDirResource::vCardDirectoryName() const
{
  return Settings::self()->path().mid( 7 );
}

QString VCardDirResource::vCardDirectoryFileName( const QString &file ) const
{
  return Settings::self()->path().mid( 7 ) + QDir::separator() + file;
}

void VCardDirResource::initializeVCardDirectory() const
{
  QDir dir( vCardDirectoryName() );

  // if folder does not exists, create it
  if ( !dir.exists() )
    QDir::root().mkpath( dir.absolutePath() );

  // check whether warning file is in place...
  QFile file( dir.absolutePath() + QDir::separator() + "WARNING_README.txt" );
  if ( !file.exists() ) {
    // ... if not, create it
    file.open( QIODevice::WriteOnly );
    file.write( "Important Warning!!!\n\n"
                "Don't create or copy vCards inside this folder manually, they are managed by the Akonadi framework!\n" );
    file.close();
  }
}

AKONADI_RESOURCE_MAIN( VCardDirResource )

#include "vcarddirresource.moc"
