/*
    Copyright (c) 2007 Tobias Koenig <tokoe@kde.org>

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

#include "vcardresource.h"

#include <libakonadi/collectionlistjob.h>
#include <libakonadi/collectionmodifyjob.h>
#include <libakonadi/itemappendjob.h>
#include <libakonadi/itemfetchjob.h>
#include <libakonadi/itemstorejob.h>
#include <libakonadi/session.h>

#include <kfiledialog.h>
#include <klocale.h>

#include <QtDBus/QDBusConnection>
using namespace Akonadi;

VCardResource::VCardResource( const QString &id )
  : ResourceBase( id )
{
  loadAddressees();
}

VCardResource::~VCardResource()
{
  mAddressees.clear();
}

bool VCardResource::retrieveItem( const Akonadi::Item &item, const QStringList &parts )
{
  Q_UNUSED( parts );
  const QString rid = item.reference().remoteId();
  if ( !mAddressees.contains( rid ) ) {
    error( QString( "Contact with uid '%1' not found!" ).arg( rid ) );
    return false;
  }
  Item i( item );
  i.setMimeType( "text/directory" );
  i.setPayload<KABC::Addressee>( mAddressees.value( rid ) );
  itemRetrieved( i );
  return true;
}

void VCardResource::aboutToQuit()
{
  QString fileName = settings()->value( "General/Path" ).toString();
  if ( fileName.isEmpty() )
    error( i18n( "No filename specified." ) );
  else if ( storeAddressees() )
    error( i18n( "Failed to save address book file to %1", fileName ) );
}

void VCardResource::configure( WId windowId )
{
  QString oldFile = settings()->value( "General/Path" ).toString();
  KUrl url;
  if ( !oldFile.isEmpty() )
    url = KUrl::fromPath( oldFile );
  else
    url = KUrl::fromPath( QDir::homePath() );

  QString newFile = KFileDialog::getOpenFileNameWId( url, "*.vcf |" + i18nc( "Filedialog filter for *.vcf", "vCard Contact File" ), windowId, i18n( "Select Address Book" ) );

  if ( newFile.isEmpty() )
    return;

  if ( oldFile == newFile )
    return;

  settings()->setValue( "General/Path", newFile );
  loadAddressees();
  synchronize();
}

void VCardResource::itemAdded( const Akonadi::Item &item, const Akonadi::Collection& )
{
  KABC::Addressee addressee = item.payload<KABC::Addressee>();
  if ( !addressee.isEmpty() ) {
    mAddressees.insert( addressee.uid(), addressee );

    Item i( item );
    i.setRemoteId( addressee.uid() );
    changesCommitted( i );
  } else {
    changeProcessed();
  }
}

void VCardResource::itemChanged( const Akonadi::Item &item, const QStringList& )
{
  KABC::Addressee addressee = item.payload<KABC::Addressee>();
  if ( !addressee.isEmpty() ) {
    mAddressees.insert( addressee.uid(), addressee );

    Item i( item );
    i.setRemoteId( addressee.uid() );
    changesCommitted( i );
  } else {
    changeProcessed();
  }
}

void VCardResource::itemRemoved(const Akonadi::DataReference & ref)
{
  if ( mAddressees.contains( ref.remoteId() ) )
    mAddressees.remove( ref.remoteId() );
  changeProcessed();
}

void VCardResource::retrieveCollections()
{
  Collection c;
  c.setParent( Collection::root() );
  c.setRemoteId( settings()->value( "General/Path" ).toString() );
  c.setName( name() );
  QStringList mimeTypes;
  mimeTypes << "text/directory";
  c.setContentTypes( mimeTypes );
  Collection::List list;
  list << c;
  collectionsRetrieved( list );
}

void VCardResource::retrieveItems( const Akonadi::Collection & col, const QStringList &parts )
{
  Q_UNUSED( parts );
  Item::List items;

  foreach ( KABC::Addressee addressee, mAddressees ) {
    Item item( DataReference( -1, addressee.uid() ) );
    item.setMimeType( "text/directory" );
    items.append( item );
  }

  itemsRetrieved( items );
}

bool VCardResource::loadAddressees()
{
  mAddressees.clear();

  QString fileName = settings()->value( "General/Path" ).toString();
  if ( fileName.isEmpty() ) {
    changeStatus( Error, i18n( "No vCard file specified." ) );
    return false;
  }

  QFile file( fileName );
  if ( !file.open( QIODevice::ReadOnly ) ) {
    changeStatus( Error, i18n( "Unable to open vCard file '%1'.", fileName ) );
    return false;
  }

  const QByteArray data = file.readAll();
  file.close();

  KABC::Addressee::List list = mConverter.parseVCards( data );
  for ( int i = 0; i < list.count(); ++i ) {
    mAddressees.insert( list[ i ].uid(), list[ i ] );
  }

  return true;
}

bool VCardResource::storeAddressees()
{
  QString fileName = settings()->value( "General/Path" ).toString();
  if ( fileName.isEmpty() ) {
    changeStatus( Error, i18n( "No vCard file specified." ) );
    return false;
  }

  QFile file( fileName );
  if ( !file.open( QIODevice::WriteOnly ) ) {
    changeStatus( Error, i18n( "Unable to open vCard file '%1'.", fileName ) );
    return false;
  }

  const QByteArray data = mConverter.createVCards( mAddressees.values() );

  file.write( data );
  file.close();

  return true;
}

#include "vcardresource.moc"
