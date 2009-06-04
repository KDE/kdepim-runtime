/*
    Copyright 2009 Constantin Berzan <exit3219@gmail.com>

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU Library General Public License as published
    by the Free Software Foundation; either version 2 of the License or
    ( at your option ) version 3 or, at the discretion of KDE e.V.
    ( which shall act as a proxy as in section 14 of the GPLv3 ), any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "mtdummyresource.h"

#include "configdialog.h"
#include "settings.h"
#include "settingsadaptor.h"

#include <KWindowSystem>

#include <QtDBus/QDBusConnection>

#include <Akonadi/ItemCopyJob>

using namespace Akonadi;

MTDummyResource::MTDummyResource( const QString &id )
  : TransportResource( id )
{
  new SettingsAdaptor( Settings::self() );
  QDBusConnection::sessionBus().registerObject( QLatin1String( "/Settings" ),
                            Settings::self(), QDBusConnection::ExportAdaptors );
}

MTDummyResource::~MTDummyResource()
{
}

void MTDummyResource::retrieveCollections()
{
  // we have no collections of our own
  collectionsRetrieved( Collection::List() );
}

void MTDummyResource::retrieveItems( const Akonadi::Collection &collection )
{
  Q_UNUSED( collection );
  // we have no collections of our own
  Q_ASSERT( false );
}

bool MTDummyResource::retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts )
{
  Q_UNUSED( item );
  Q_UNUSED( parts );
  // we have no collections of our own
  Q_ASSERT( false );
  return false;
}

void MTDummyResource::aboutToQuit()
{
}

void MTDummyResource::configure( WId windowId )
{
  ConfigDialog dlg;
  if ( windowId )
    KWindowSystem::setMainWindow( &dlg, windowId );
  dlg.exec();
}

void MTDummyResource::send( Item::Id message )
{
  ItemCopyJob *job = new ItemCopyJob( Item( message ), Collection( Settings::self()->sink() ) );
  connect( job, SIGNAL( result( KJob* ) ), this, SLOT( emitResult( KJob * ) ) );
}

AKONADI_RESOURCE_MAIN( MTDummyResource )

#include "mtdummyresource.moc"
