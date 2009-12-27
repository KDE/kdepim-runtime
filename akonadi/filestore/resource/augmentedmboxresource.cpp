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

#include "augmentedmboxresource.h"

#include "augmentedmboxresourcesettings.h"
#include "augmentedmboxsettingsadaptor.h"

#include <akonadi/filestore/itemmodifyjob.h>

#include <akonadi/kmime/messageparts.h>

#include <KFileDialog>
#include <KLocale>

using namespace Akonadi::FileStore;

AugmentedMBoxResource::AugmentedMBoxResource( const QString &id )
  : StoreResource<AugmentedMBoxStore>( id )
{
  mPayloadParts << Akonadi::MessagePart::Envelope
                << Akonadi::MessagePart::Body
                << Akonadi::MessagePart::Header;

  new SettingsAdaptor( Settings::self());
  QDBusConnection::sessionBus().registerObject( QLatin1String( "/Settings" ),
                            Settings::self(), QDBusConnection::ExportAdaptors );
}

void AugmentedMBoxResource::configure( WId windowId )
{
  Q_UNUSED( windowId );

  const QString oldFileName = Settings::self()->fileName();
  KUrl url;
  if ( !oldFileName.isEmpty() )
    url = KUrl::fromPath( oldFileName );
  else
    url = KUrl::fromPath( QDir::homePath() );

  const QString title = i18nc( "@title:window", "Select an MBox file" );
  const QString filter = QLatin1String( "application/mbox" );
  const QString newFileName = KFileDialog::getOpenFileName( url, filter, 0, title );

  if ( newFileName.isEmpty() )
    return;

  if ( oldFileName == newFileName )
    return;

  Settings::self()->setFileName( newFileName );

  Settings::self()->writeConfig();

  static_cast<AugmentedMBoxStore*>( mStore )->setFileName( newFileName );

  clearCache();
  synchronizeCollectionTree();
}

void AugmentedMBoxResource::itemModifyDone( KJob *job )
{
  StoreResource<AugmentedMBoxStore>::itemModifyDone( job );
  if ( job->error() == 0 ) {
    ItemModifyJob *itemModify = dynamic_cast<ItemModifyJob*>( job );
    Q_ASSERT( itemModify != 0 );

    if ( !itemModify->ignorePayload() ) {
      scheduleCustomTask( this, "compactStore", QVariant() );
    }
  }
}

void AugmentedMBoxResource::itemDeleteDone( KJob *job )
{
  StoreResource<AugmentedMBoxStore>::itemDeleteDone( job );
  if ( job->error() == 0 ) {
    scheduleCustomTask( this, "compactStore", QVariant() );
  }
}

AKONADI_RESOURCE_MAIN( AugmentedMBoxResource )

#include "augmentedmboxresource.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
