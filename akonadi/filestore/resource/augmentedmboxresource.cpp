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

using namespace Akonadi::FileStore;

AugmentedMBoxResource::AugmentedMBoxResource( const QString &id )
  : StoreResource<AugmentedMBoxStore>( id )
{
  new SettingsAdaptor( Settings::self() );
  QDBusConnection::sessionBus().registerObject( QLatin1String( "/Settings" ),
                            Settings::self(), QDBusConnection::ExportAdaptors );
}

void AugmentedMBoxResource::configure( WId windowId )
{
  Q_UNUSED( windowId );
}

void AugmentedMBoxResource::itemModifyDone( KJob *job )
{
  StoreResource<AugmentedMBoxStore>::itemModifyDone( job );
  if ( job->error() == 0 ) {
    scheduleCustomTask( this, "compactStore", QVariant() );
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
