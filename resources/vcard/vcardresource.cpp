/*
    Copyright (c) 2007 Tobias Koenig <tokoe@kde.org>
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

#include "vcardresource.h"
#include "configdialog.h"
#include "settingsadaptor.h"

#include <akonadi/changerecorder.h>
#include <akonadi/itemfetchscope.h>

#include <kfiledialog.h>
#include <klocale.h>
#include <KWindowSystem>

#include <QtDBus/QDBusConnection>
using namespace Akonadi;

VCardResource::VCardResource( const QString &id )
  : SingleFileResource<Settings>( id )
{
  new SettingsAdaptor( Settings::self() );
  QDBusConnection::sessionBus().registerObject( QLatin1String( "/Settings" ),
                            Settings::self(), QDBusConnection::ExportAdaptors );
  changeRecorder()->itemFetchScope().fetchFullPayload();
}

VCardResource::~VCardResource()
{
  mAddressees.clear();
}

bool VCardResource::retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts )
{
  Q_UNUSED( parts );
  const QString rid = item.remoteId();
  if ( !mAddressees.contains( rid ) ) {
    emit error( QString( "Contact with uid '%1' not found!" ).arg( rid ) );
    return false;
  }
  Item i( item );
  i.setPayload<KABC::Addressee>( mAddressees.value( rid ) );
  itemRetrieved( i );
  return true;
}

void VCardResource::aboutToQuit()
{
  // TODO: we need to delay termination whehn writeToFile() becomes async!
  writeFile();
  Settings::self()->writeConfig();
}

void VCardResource::configure( WId windowId )
{
  ConfigDialog dlg;
  if ( windowId )
    KWindowSystem::setMainWindow( &dlg, windowId );
  dlg.exec();

  readFile();
  synchronize();
}

void VCardResource::itemAdded( const Akonadi::Item &item, const Akonadi::Collection& )
{
  KABC::Addressee addressee;
  if ( item.hasPayload<KABC::Addressee>() )
    addressee  = item.payload<KABC::Addressee>();

  if ( !addressee.isEmpty() ) {
    mAddressees.insert( addressee.uid(), addressee );

    Item i( item );
    i.setRemoteId( addressee.uid() );
    changeCommitted( i );

    fileDirty();
  } else {
    changeProcessed();
  }
}

void VCardResource::itemChanged( const Akonadi::Item &item, const QSet<QByteArray>& )
{
  KABC::Addressee addressee;
  if ( item.hasPayload<KABC::Addressee>() )
    addressee  = item.payload<KABC::Addressee>();

  if ( !addressee.isEmpty() ) {
    mAddressees.insert( addressee.uid(), addressee );

    Item i( item );
    i.setRemoteId( addressee.uid() );
    changeCommitted( i );

    fileDirty();
  } else {
    changeProcessed();
  }
}

void VCardResource::itemRemoved(const Akonadi::Item & item)
{
  if ( mAddressees.contains( item.remoteId() ) )
    mAddressees.remove( item.remoteId() );

  fileDirty();

  changeProcessed();
}

void VCardResource::retrieveCollections()
{
  Collection c;
  c.setParent( Collection::root() );
  c.setRemoteId( Settings::self()->path() );
  c.setName( name() );
  QStringList mimeTypes;
  mimeTypes << "text/directory";
  c.setContentMimeTypes( mimeTypes );
  Collection::List list;
  list << c;
  collectionsRetrieved( list );
}

void VCardResource::retrieveItems( const Akonadi::Collection & col )
{
  // VCard does not support folders so we can savely ignore the collection
  Q_UNUSED( col );

  Item::List items;

  // FIXME: Check if the KIO::Job is done and was successfull, if so send the
  // items, otherwise set a bool and in the result slot of the job send the
  // items if the bool is set.

  foreach ( const KABC::Addressee &addressee, mAddressees ) {
    Item item;
    item.setRemoteId( addressee.uid() );
    item.setMimeType( "text/directory" );
    items.append( item );
  }

  itemsRetrieved( items );
}

bool VCardResource::readFromFile( const QString &fileName )
{
  mAddressees.clear();

  QFile file( KUrl( fileName ).path() );
  if ( !file.open( QIODevice::ReadOnly ) ) {
    emit status( Broken, i18n( "Unable to open vCard file '%1'.", fileName ) );
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

bool VCardResource::writeToFile( const QString &fileName )
{
  QFile file( fileName );
  if ( !file.open( QIODevice::WriteOnly ) ) {
    emit status( Broken, i18n( "Unable to open vCard file '%1'.", fileName ) );
    return false;
  }

  const QByteArray data = mConverter.createVCards( mAddressees.values() );

  file.write( data );
  file.close();

  return true;
}

AKONADI_RESOURCE_MAIN( VCardResource )

#include "vcardresource.moc"
