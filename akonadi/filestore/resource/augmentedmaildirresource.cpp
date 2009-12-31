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

#include "augmentedmaildirresource.h"

#include "augmentedmaildirresourcesettings.h"
#include "augmentedmaildirsettingsadaptor.h"

#include <akonadi/kmime/messageparts.h>

#include <KFileDialog>
#include <KLocale>

using namespace Akonadi::FileStore;

AugmentedMailDirResource::AugmentedMailDirResource( const QString &id )
  : StoreResource<AugmentedMailDirStore>( id )
{
  mPayloadParts << Akonadi::MessagePart::Envelope
                << Akonadi::MessagePart::Body
                << Akonadi::MessagePart::Header;

  new SettingsAdaptor( Settings::self());
  QDBusConnection::sessionBus().registerObject( QLatin1String( "/Settings" ),
                            Settings::self(), QDBusConnection::ExportAdaptors );
}

void AugmentedMailDirResource::configure( WId windowId )
{
  Q_UNUSED( windowId );

  const QString oldPath = Settings::self()->path();
  KUrl url;
  if ( !oldPath.isEmpty() )
    url = KUrl::fromPath( oldPath );
  else
    url = KUrl::fromPath( QDir::homePath() );

  const QString title = i18nc( "@title:window", "Select a MailDir directory" );
  const QString newPath = KFileDialog::getExistingDirectory( url, 0, title );

  if ( newPath.isEmpty() )
    return;

  if ( oldPath == newPath )
    return;

  Settings::self()->setPath( newPath );

  Settings::self()->writeConfig();

  static_cast<AugmentedMailDirStore*>( mStore )->setFileName( newPath );

  clearCache();
  synchronizeCollectionTree();
}

AKONADI_RESOURCE_MAIN( AugmentedMailDirResource )

#include "augmentedmaildirresource.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
