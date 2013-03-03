/*
    Copyright (c) 2008 Tobias Koenig <tokoe@kde.org>
    Copyright (c) 2008 Bertjan Broeksema <broeksema@kde.org>
    Copyright (c) 2012 Sérgio Martins <iamsergio@gmail.com>

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

#include "icaldirresource.h"

#include "settingsadaptor.h"
#include "../shared/dirsettingsdialog.h"

#include <akonadi/changerecorder.h>
#include <akonadi/entitydisplayattribute.h>
#include <akonadi/itemfetchscope.h>

#include <KCalCore/MemoryCalendar>
#include <KCalCore/FileStorage>
#include <KCalCore/ICalFormat>

#include <QtCore/QDir>
#include <QtCore/QDirIterator>
#include <QtCore/QFile>

using namespace Akonadi;
using namespace KCalCore;

static Incidence::Ptr readFromFile( const QString &fileName, const QString &expectedUid )
{
  MemoryCalendar::Ptr calendar = MemoryCalendar::Ptr( new MemoryCalendar( QLatin1String( "UTC" ) ) );
  FileStorage::Ptr fileStorage = FileStorage::Ptr( new FileStorage( calendar, fileName, new ICalFormat() ) );

  Incidence::Ptr incidence;
  if ( fileStorage->load() ) {
    Incidence::List incidences = calendar->incidences();
    if ( incidences.count() == 1 && incidences.first()->uid() == expectedUid )
      incidence = incidences.first();
  } else {
    kError() << "Error loading file " << fileName;
  }

  return incidence;
}

static bool writeToFile( const QString &fileName, Incidence::Ptr &incidence )
{
  if ( !incidence ) {
    kError() << "incidence is 0!";
    return false;
  }

  MemoryCalendar::Ptr calendar = MemoryCalendar::Ptr( new MemoryCalendar( QLatin1String( "UTC" ) ) );
  FileStorage::Ptr fileStorage = FileStorage::Ptr( new FileStorage( calendar, fileName, new ICalFormat() ) );
  calendar->addIncidence( incidence );
  Q_ASSERT( calendar->incidences().count() == 1 );

  const bool success = fileStorage->save();
  if ( !success ) {
    kError() << "Failed to save calendar to file " + fileName;
  }

  return success;
}

ICalDirResource::ICalDirResource( const QString &id )
  : ResourceBase( id )
{
  // setup the resource
  new SettingsAdaptor( Settings::self() );
  QDBusConnection::sessionBus().registerObject( QLatin1String( "/Settings" ),
                            Settings::self(), QDBusConnection::ExportAdaptors );

  changeRecorder()->itemFetchScope().fetchFullPayload();
}

ICalDirResource::~ICalDirResource()
{
}

void ICalDirResource::aboutToQuit()
{
  Settings::self()->writeConfig();
}

void ICalDirResource::configure( WId windowId )
{
  SettingsDialog dlg( windowId );
  dlg.setWindowIcon( KIcon( "text-calendar" ) );
  if ( dlg.exec() ) {
    initializeICalDirectory();
    loadIncidences();

    synchronize();

    emit configurationDialogAccepted();
  } else {
    emit configurationDialogRejected();
  }
}

bool ICalDirResource::loadIncidences()
{
  mIncidences.clear();

  QDirIterator it( iCalDirectoryName() );
  while ( it.hasNext() ) {
    it.next();
    if ( it.fileName() != "." && it.fileName() != ".." && it.fileName() != "WARNING_README.txt" ) {
      const KCalCore::Incidence::Ptr incidence = readFromFile( it.filePath(), it.fileName() );
      if ( incidence ) {
        mIncidences.insert( incidence->uid(), incidence );
      }
    }
  }

  emit status( Idle );
  return true;
}

bool ICalDirResource::retrieveItem( const Akonadi::Item &item, const QSet<QByteArray>& )
{
  const QString remoteId = item.remoteId();
  if ( !mIncidences.contains( remoteId ) ) {
    emit error( i18n( "Incidence with uid '%1' not found.", remoteId ) );
    return false;
  }

  Item newItem( item );
  newItem.setPayload<KCalCore::Incidence::Ptr>( mIncidences.value( remoteId ) );
  itemRetrieved( newItem );

  return true;
}

void ICalDirResource::itemAdded( const Akonadi::Item &item, const Akonadi::Collection& )
{
  if ( Settings::self()->readOnly() ) {
    emit error( i18n( "Trying to write to a read-only directory: '%1'", iCalDirectoryName() ) );
    cancelTask();
    return;
  }

  KCalCore::Incidence::Ptr incidence;
  if ( item.hasPayload<KCalCore::Incidence::Ptr>() )
    incidence = item.payload<KCalCore::Incidence::Ptr>();

  if ( incidence ) {
    // add it to the cache...
    mIncidences.insert( incidence->uid(), incidence );

    // ... and write it through to the file system
    const bool success = writeToFile( iCalDirectoryFileName( incidence->uid() ), incidence );

    if ( success ) {
      // report everything ok
      Item newItem( item );
      newItem.setRemoteId( incidence->uid() );
      changeCommitted( newItem );
    } else {
      cancelTask();
    }
  } else {
    changeProcessed();
  }
}

void ICalDirResource::itemChanged( const Akonadi::Item &item, const QSet<QByteArray>& )
{
  if ( Settings::self()->readOnly() ) {
    emit error( i18n( "Trying to write to a read-only directory: '%1'", iCalDirectoryName() ) );
    cancelTask();
    return;
  }

  KCalCore::Incidence::Ptr incidence;
  if ( item.hasPayload<KCalCore::Incidence::Ptr>() )
    incidence  = item.payload<KCalCore::Incidence::Ptr>();

  if ( incidence ) {
    // change it in the cache...
    mIncidences.insert( incidence->uid(), incidence );

    // ... and write it through to the file system
    const bool success = writeToFile( iCalDirectoryFileName( incidence->uid() ), incidence );

    if ( success ) {
      Item newItem( item );
      newItem.setRemoteId( incidence->uid() );
      changeCommitted( newItem );
    } else {
      cancelTask();
    }
  } else {
    changeProcessed();
  }
}

void ICalDirResource::itemRemoved( const Akonadi::Item &item )
{
  if ( Settings::self()->readOnly() ) {
    emit error( i18n( "Trying to write to a read-only directory: '%1'", iCalDirectoryName() ) );
    cancelTask();
    return;
  }

  // remove it from the cache...
  if ( mIncidences.contains( item.remoteId() ) )
    mIncidences.remove( item.remoteId() );

  // ... and remove it from the file system
  QFile::remove( iCalDirectoryFileName( item.remoteId() ) );

  changeProcessed();
}

void ICalDirResource::retrieveCollections()
{
  Collection c;
  c.setParentCollection( Collection::root() );
  c.setRemoteId( iCalDirectoryName() );
  c.setName( name() );
  c.setContentMimeTypes( QStringList() << "text/calendar" );
  if ( Settings::self()->readOnly() ) {
    c.setRights( Collection::CanChangeCollection );
  } else {
    Collection::Rights rights = Collection::ReadOnly;
    rights |= Collection::CanChangeItem;
    rights |= Collection::CanCreateItem;
    rights |= Collection::CanDeleteItem;
    rights |= Collection::CanChangeCollection;
    c.setRights( rights );
  }

  EntityDisplayAttribute* attr = c.attribute<EntityDisplayAttribute>( Collection::AddIfMissing );
  attr->setDisplayName( i18n( "Calendar Folder" ) );
  attr->setIconName( "office-calendar" );

  Collection::List list;
  list << c;
  collectionsRetrieved( list );
}

void ICalDirResource::retrieveItems( const Akonadi::Collection& )
{
  loadIncidences();
  Item::List items;

  foreach ( const KCalCore::Incidence::Ptr &incidence, mIncidences ) {
    Item item;
    item.setRemoteId( incidence->uid() );
    item.setMimeType( incidence->mimeType() );
    items.append( item );
  }

  itemsRetrieved( items );
}

QString ICalDirResource::iCalDirectoryName() const
{
  return Settings::self()->path();
}

QString ICalDirResource::iCalDirectoryFileName( const QString &file ) const
{
  return Settings::self()->path() + QDir::separator() + file;
}

void ICalDirResource::initializeICalDirectory() const
{
  QDir dir( iCalDirectoryName() );

  // if folder does not exists, create it
  if ( !dir.exists() )
    QDir::root().mkpath( dir.absolutePath() );

  // check whether warning file is in place...
  QFile file( dir.absolutePath() + QDir::separator() + "WARNING_README.txt" );
  if ( !file.exists() ) {
    // ... if not, create it
    file.open( QIODevice::WriteOnly );
    file.write( "Important Warning!!!\n\n"
                "Don't create or copy files inside this folder manually, they are managed by the Akonadi framework!\n" );
    file.close();
  }
}

AKONADI_RESOURCE_MAIN( ICalDirResource )

#include "icaldirresource.moc"
