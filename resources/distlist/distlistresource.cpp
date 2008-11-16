/*
    Copyright (c) 2008 Bertjan Broeksema <b.broeksema@kdemail.net>
    Copyright (c) 2008 Kevin Krammer <kevin.krammer@gmx.at>

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

#include "distlistresource.h"
#include "settingsadaptor.h"
#include "singlefileresourceconfigdialog.h"

#include <kabc/contactgrouptool.h>

#include <kfiledialog.h>
#include <klocale.h>
#include <KWindowSystem>

#include <QtDBus/QDBusConnection>
using namespace Akonadi;

DistListResource::DistListResource( const QString &id )
  : SingleFileResource<Settings>( id )
{
  setSupportedMimetypes( QStringList() << KABC::ContactGroup::mimeType() );

  new SettingsAdaptor( Settings::self() );
  QDBusConnection::sessionBus().registerObject( QLatin1String( "/Settings" ),
                            Settings::self(), QDBusConnection::ExportAdaptors );
}

DistListResource::~DistListResource()
{
  mContactGroups.clear();
}

bool DistListResource::retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts )
{
  Q_UNUSED( parts );
  const QString rid = item.remoteId();
  if ( !mContactGroups.contains( rid ) ) {
    emit error( QString( "ContactGroup with uid '%1' not found!" ).arg( rid ) );
    return false;
  }
  Item i( item );
  i.setPayload<KABC::ContactGroup>( mContactGroups.value( rid ) );
  itemRetrieved( i );
  return true;
}

void DistListResource::aboutToQuit()
{
  writeFile();
  Settings::self()->writeConfig();
}

void DistListResource::configure( WId windowId )
{
  SingleFileResourceConfigDialog<Settings> dlg( windowId );
  dlg.setFilter( "*.distlist|" + i18nc("Filedialog filter for *.distlist", "Distributionlist File" ) );
  dlg.setCaption( i18n("Select Distributionlist File") );
  if ( dlg.exec() == QDialog::Accepted ) {
    reloadFile();
  }
}

void DistListResource::itemAdded( const Akonadi::Item &item, const Akonadi::Collection& )
{
  KABC::ContactGroup contactGroup;
  if ( item.hasPayload<KABC::ContactGroup>() )
    contactGroup  = item.payload<KABC::ContactGroup>();

  if ( !contactGroup.id().isEmpty() ) {
    mContactGroups.insert( contactGroup.id(), contactGroup );

    Item i( item );
    i.setRemoteId( contactGroup.id() );
    changeCommitted( i );

    fileDirty();
  } else {
    changeProcessed();
  }
}

void DistListResource::itemChanged( const Akonadi::Item &item, const QSet<QByteArray>& )
{
  KABC::ContactGroup contactGroup;
  if ( item.hasPayload<KABC::ContactGroup>() )
    contactGroup  = item.payload<KABC::ContactGroup>();

  if ( !contactGroup.id().isEmpty() ) {
    mContactGroups.insert( contactGroup.id(), contactGroup );

    Item i( item );
    i.setRemoteId( contactGroup.id() );
    changeCommitted( i );

    fileDirty();
  } else {
    changeProcessed();
  }
}

void DistListResource::itemRemoved(const Akonadi::Item & item)
{
  if ( mContactGroups.contains( item.remoteId() ) )
    mContactGroups.remove( item.remoteId() );

  fileDirty();

  changeProcessed();
}

void DistListResource::retrieveItems( const Akonadi::Collection & col )
{
  // distributionlists does not support folders so we can safely ignore the collection
  Q_UNUSED( col );

  Item::List items;

  // FIXME: Check if the KIO::Job is done and was successful, if so send the
  // items, otherwise set a bool and in the result slot of the job send the
  // items if the bool is set.

  foreach ( const KABC::ContactGroup &contactGroup, mContactGroups ) {
    Item item;
    item.setRemoteId( contactGroup.id() );
    item.setMimeType( KABC::ContactGroup::mimeType() );
    item.setPayload( contactGroup );
    items.append( item );
  }

  itemsRetrieved( items );
}

bool DistListResource::readFromFile( const QString &fileName )
{
  mContactGroups.clear();

  QFile file( KUrl( fileName ).path() );
  if ( !file.open( QIODevice::ReadOnly ) ) {
    emit status( Broken, i18n( "Unable to open distributionlist file '%1'.", fileName ) );
    return false;
  }

  // TODO: check for KDE legacy file (KConfig based)

  KABC::ContactGroup::List list;
  // TODO check for error
  KABC::ContactGroupTool::convertFromXml( &file, list );

  file.close();

  foreach( const KABC::ContactGroup &group, list ) {
    mContactGroups.insert( group.id(), group );
  }

  return true;
}

bool DistListResource::writeToFile( const QString &fileName )
{
  QFile file( fileName );
  if ( !file.open( QIODevice::WriteOnly ) ) {
    emit status( Broken, i18n( "Unable to open distributionlist file '%1'.", fileName ) );
    return false;
  }

  // TODO: check for KDE legacy file (KConfig based)

  // TODO check for error
  KABC::ContactGroupTool::convertToXml( mContactGroups.values(), &file );

  file.close();

  return true;
}

AKONADI_RESOURCE_MAIN( DistListResource )

#include "distlistresource.moc"
// kate: space-indent on; indent-width 2; replace-tabs on;
