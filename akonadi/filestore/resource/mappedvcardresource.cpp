/*  This file is part of the KDE project
    Copyright (C) 2009 Kevin Krammer <kevin.krammer@gmx.at>

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

#include "mappedvcardresource.h"

#include "mappedvcardresourcesettings.h"
#include "mappedvcardsettingsadaptor.h"

#include <akonadi/filestore/storecompactjob.h>

#include <akonadi/kabc/contactparts.h>

#include <KFileDialog>
#include <KLocale>

using namespace Akonadi::FileStore;

MappedVCardResource::MappedVCardResource( const QString &id )
  : StoreResource<MappedVCardStore>( id )
{
  mPayloadParts << Akonadi::ContactPart::Lookup
                << Akonadi::ContactPart::Standard;

  new SettingsAdaptor( Settings::self() );
  QDBusConnection::sessionBus().registerObject( QLatin1String( "/Settings" ),
                            Settings::self(), QDBusConnection::ExportAdaptors );
}

void MappedVCardResource::configure( WId windowId )
{
  Q_UNUSED( windowId );

  const QString oldFileName = Settings::self()->fileName();
  KUrl url;
  if ( !oldFileName.isEmpty() )
    url = KUrl::fromPath( oldFileName );
  else
    url = KUrl::fromPath( QDir::homePath() );

  const QString title = i18nc( "@title:window", "Select a vCard file" );
  const QString filter = QLatin1String( "text/directory" );
  const QString newFileName = KFileDialog::getOpenFileName( url, filter, 0, title );

  if ( newFileName.isEmpty() )
    return;

  if ( oldFileName == newFileName )
    return;

  Settings::self()->setFileName( newFileName );

  Settings::self()->writeConfig();

  static_cast<MappedVCardStore*>( mStore )->setFileName( newFileName );

  clearCache();
  synchronizeCollectionTree();
}

void MappedVCardResource::aboutToQuit()
{
  if ( mStore != 0 ) {
    StoreCompactJob *job = mStore->compactStore();
    job->exec();
  }

  StoreResource<MappedVCardStore>::aboutToQuit();
}

AKONADI_RESOURCE_MAIN( MappedVCardResource )

#include "mappedvcardresource.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
