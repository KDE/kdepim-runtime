/*
    Copyright (c) 2009 Bertjan Broeksem <b.broeksema@kdemail.net>

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

#include "mboxresource.h"

#include <akonadi/changerecorder.h>
#include <akonadi/itemfetchscope.h>
#include <KWindowSystem>
#include <QtDBus/QDBusConnection>

#include "configdialog.h"
#include "settings.h"
#include "settingsadaptor.h"

using namespace Akonadi;

MboxResource::MboxResource( const QString &id ) : ResourceBase( id )
{
  new SettingsAdaptor( Settings::self() );
  QDBusConnection::sessionBus().registerObject( QLatin1String( "/Settings" ),
                              Settings::self(), QDBusConnection::ExportAdaptors );

  changeRecorder()->fetchCollection( true );
  changeRecorder()->itemFetchScope().fetchFullPayload( true );
}

MboxResource::~MboxResource()
{
}

void MboxResource::configure( WId windowId )
{
  ConfigDialog dlg;
  if ( windowId )
    KWindowSystem::setMainWindow( &dlg, windowId );
  dlg.exec();

  synchronizeCollectionTree();
}

void MboxResource::retrieveCollections()
{
}

void MboxResource::retrieveItems( const Akonadi::Collection &col )
{
  Q_UNUSED(col);
}

bool MboxResource::retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts )
{
  Q_UNUSED(item);
  Q_UNUSED(parts);
  return false;
}

void MboxResource::aboutToQuit()
{
}

void MboxResource::itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection )
{
  Q_UNUSED(item);
  Q_UNUSED(collection);
}

void MboxResource::itemChanged( const Akonadi::Item &item, const QSet<QByteArray> &parts )
{
  Q_UNUSED(item);
  Q_UNUSED(parts);
}

void MboxResource::itemRemoved( const Akonadi::Item &item )
{
  Q_UNUSED(item);
}

void MboxResource::collectionAdded( const Akonadi::Collection &collection
                                  , const Akonadi::Collection &parent )
{
  Q_UNUSED(collection);
  Q_UNUSED(parent);
}

void MboxResource::collectionChanged( const Akonadi::Collection &collection )
{
  Q_UNUSED(collection);
}

void MboxResource::collectionRemoved( const Akonadi::Collection &collection )
{
  Q_UNUSED(collection);
}


AKONADI_RESOURCE_MAIN( MboxResource )

#include "mboxresource.moc"
