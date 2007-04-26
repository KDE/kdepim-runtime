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

#include <libakonadi/collectionlistjob.h>
#include <libakonadi/collectionmodifyjob.h>
#include <libakonadi/itemappendjob.h>
#include <libakonadi/itemfetchjob.h>
#include <libakonadi/itemstorejob.h>
#include <libakonadi/session.h>

#include <kfiledialog.h>
#include <klocale.h>

#include <QtDBus/QDBusConnection>

#include "vcardresource.h"

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

bool VCardResource::requestItemDelivery( const Akonadi::DataReference &ref, int, const QDBusMessage &msg )
{
  if ( !mAddressees.contains( ref.remoteId() ) ) {
    error( QString( "Contact with uid '%1' not found!" ).arg( ref.remoteId() ) );
    return false;
  } else {
    const QByteArray data = mConverter.createVCard( mAddressees.value( ref.remoteId() ) );

    ItemStoreJob *job = new ItemStoreJob( ref, session() );
    job->setData( data );

    return deliverItem( job, msg );
  }
}

void VCardResource::aboutToQuit()
{
  QString fileName = settings()->value( "General/Path" ).toString();
  if ( fileName.isEmpty() )
    error( i18n( "No filename specified." ) );
  else if ( storeAddressees() )
    error( i18n( "Failed to save address book file to %1", fileName ) );
}

void VCardResource::configure()
{
  QString oldFile = settings()->value( "General/Path" ).toString();
  KUrl url;
  if ( !oldFile.isEmpty() )
    url = KUrl::fromPath( oldFile );
  else
    url = KUrl::fromPath( QDir::homePath() );

  QString newFile = KFileDialog::getOpenFileName( url, "*.vcf |vCard Contact File", 0, i18n( "Select Address Book" ) );

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

    DataReference r( item.reference().id(), addressee.uid() );
    changesCommitted( r );
  }
}

void VCardResource::itemChanged( const Akonadi::Item &item, const QStringList& )
{
  KABC::Addressee addressee = item.payload<KABC::Addressee>();
  if ( !addressee.isEmpty() ) {
    mAddressees.insert( addressee.uid(), addressee );

    DataReference r( item.reference().id(), addressee.uid() );
    changesCommitted( r );
  }
}

void VCardResource::itemRemoved(const Akonadi::DataReference & ref)
{
  if ( mAddressees.contains( ref.remoteId() ) )
    mAddressees.remove( ref.remoteId() );
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

void VCardResource::synchronizeCollection( const Akonadi::Collection & col )
{
  ItemFetchJob *fetch = new ItemFetchJob( col, session() );
  if ( !fetch->exec() ) {
    changeStatus( Error, i18n( "Unable to fetch listing of collection '%1': %2", col.name(), fetch->errorString() ) );
    return;
  }

  changeProgress( 0 );

  Item::List items = fetch->items();

  int counter = 0;
  foreach ( KABC::Addressee addressee, mAddressees ) {
    QString uid = addressee.uid();

    bool found = false;
    foreach ( Item item, items ) {
      if ( item.reference().remoteId() == uid ) {
        found = true;
        break;
      }
    }

    if ( found )
      continue;

    Item item( DataReference( -1, uid ) );
    item.setMimeType( "text/vcard" );
    ItemAppendJob *append = new ItemAppendJob( item, col, session() );
    if ( !append->exec() ) {
      changeProgress( 0 );
      changeStatus( Error, i18n( "Appending new contact failed: %1", append->errorString() ) );
      return;
    }

    counter++;
    int percentage = (counter * 100) / mAddressees.count();
    changeProgress( percentage );
  }

  collectionSynchronized();
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
  for ( int i = 0; i < list.count(); ++i )
    mAddressees.insert( list[ i ].uid(), list[ i ] );

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
