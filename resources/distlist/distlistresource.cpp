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
#include <kmimetype.h>
#include <KWindowSystem>

#include <QtDBus/QDBusConnection>
using namespace Akonadi;

DistListResource::DistListResource( const QString &id )
  : SingleFileResource<Settings>( id ),
    mLegacyKConfigFormat( false )
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

void DistListResource::configure( WId windowId )
{
  SingleFileResourceConfigDialog<Settings> dlg( windowId );
  dlg.setFilter( "*.distlist|" + i18nc("Filedialog filter for *.distlist", "Distributionlist File" ) );
  dlg.setCaption( i18n("Select Distributionlist File") );
  if ( dlg.exec() == QDialog::Accepted ) {
    reloadFile();
  }
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

bool DistListResource::readFromFile( const QString &fileName )
{
  mContactGroups.clear();

  QFile file( KUrl( fileName ).path() );
  if ( !file.open( QIODevice::ReadOnly ) ) {
    emit status( Broken, i18n( "Unable to open distributionlist file '%1'.", fileName ) );
    return false;
  }

  const QByteArray peekData = file.peek( 100 );
  KMimeType::Ptr contentType = KMimeType::findByContent( peekData );
  if ( contentType->is( QLatin1String( "application/xml" ) ) ) {
    kDebug() << "File format is XML based";
    KABC::ContactGroup::List list;
    // TODO check for error
    KABC::ContactGroupTool::convertFromXml( &file, list );

    file.close();

    foreach( const KABC::ContactGroup &group, list ) {
      mContactGroups.insert( group.id(), group );
    }
  } else {
    kDebug() << "Checking for legacy format";
    file.close();

    KConfig cfg( fileName );
    if ( cfg.hasGroup( "DistributionLists" ) ) {
      mLegacyKConfigFormat = true;

      KConfigGroup cg( &cfg, "DistributionLists" );
      const QStringList entryList = cg.keyList();

      foreach ( const QString &entry, entryList ) {
        const QStringList value = cg.readEntry( entry, QStringList() );

        KABC::ContactGroup contactGroup( entry );
        contactGroup.setId( fileName + entry );

        QStringList::const_iterator entryIt = value.constBegin();
        while ( entryIt != value.constEnd() ) {
          const QString id = *entryIt;
          ++entryIt;

          const QString email = entryIt != value.constEnd() ? *entryIt : QString();

          KABC::ContactGroup::Reference reference( id );
          if ( !email.isEmpty() )
            reference.setPreferredEmail( email );

          contactGroup.append( reference );

          if ( entryIt == value.constEnd() )
            break;

          ++entryIt;
        }

        mContactGroups.insert( contactGroup.id(), contactGroup );
      }
    }
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

  if ( mLegacyKConfigFormat ) {
    file.close();

    KConfig cfg( fileName );
    KConfigGroup cg( &cfg, "DistributionLists" );
    cg.deleteGroup();

    QMap<QString, KABC::ContactGroup>::const_iterator it = mContactGroups.constBegin();
    QMap<QString, KABC::ContactGroup>::const_iterator endIt = mContactGroups.constEnd();
    for ( ; it != endIt; ++it ) {
      const KABC::ContactGroup contactGroup = it.value();

      QStringList value;
      for ( unsigned int index = 0; index < contactGroup.referencesCount(); ++index ) {
        const KABC::ContactGroup::Reference reference = contactGroup.reference( index );

        value.append( reference.uid() );
        value.append( reference.preferredEmail() );
      }

      cg.writeEntry( contactGroup.name(), value );
    }

    cg.sync();
    return true;
  }

  // TODO check for error
  KABC::ContactGroupTool::convertToXml( mContactGroups.values(), &file );

  file.close();

  return true;
}

void DistListResource::aboutToQuit()
{
  writeFile();
  Settings::self()->writeConfig();
}

void DistListResource::itemAdded( const Akonadi::Item &item, const Akonadi::Collection& )
{
  KABC::ContactGroup contactGroup;
  if ( item.hasPayload<KABC::ContactGroup>() )
    contactGroup = item.payload<KABC::ContactGroup>();

  if ( !contactGroup.id().isEmpty() ) {
    if ( mLegacyKConfigFormat ) {
      // legacy format only supports ContactGroup::Reference style data
      if ( contactGroup.dataCount() > 0 ) {
        error( i18nc( "@info:status",
                      "Current storage format cannot handle distribution list entries for which no address book entry exists" ) );
        changeProcessed();
        return;
      }
    }
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
    contactGroup = item.payload<KABC::ContactGroup>();

  if ( !contactGroup.id().isEmpty() ) {
    if ( mLegacyKConfigFormat ) {
      // legacy format only supports ContactGroup::Reference style data
      if ( contactGroup.dataCount() > 0 ) {
        error( i18nc( "@info:status",
                      "Current storage format cannot handle distribution list entries for which no address book entry exists" ) );
        changeProcessed();
        return;
      }
    }
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

AKONADI_RESOURCE_MAIN( DistListResource )

#include "distlistresource.moc"
// kate: space-indent on; indent-width 2; replace-tabs on;
