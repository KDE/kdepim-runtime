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
#include "mbox.h"
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
  MBox mbox(Settings::self()->file());

  QString errMsg;
  if ( !mbox.isValid( errMsg ) ) {
    emit error( errMsg );
    collectionsRetrieved( Collection::List() );
  }

  Collection col;
  col.setParent(Collection::root());
  col.setRemoteId(Settings::self()->file());
  col.setName(name());

  QStringList mimeTypes;
  mimeTypes << "message/rfc822" << Collection::mimeType();
  col.setContentMimeTypes( mimeTypes );

  collectionsRetrieved(Collection::List() << col);
}

void MboxResource::retrieveItems( const Akonadi::Collection &col )
{
  MBox mbox( col.remoteId() );

  if (Settings::self()->lockfileMethod() == Settings::procmail)
    mbox.setProcmailLockFile(Settings::self()->lockfile());

  if (!mbox.isValid()) {
    emit error( i18n("Invalid mbox file: %1", col.remoteId() ) );
    itemsRetrieved(Item::List());
    return;
  }

  if (int rc = mbox.open() != 0) { // This can happen for example when the lock fails.
    emit error(i18n("Error while opening mbox file %1: %2", col.remoteId(), rc));
    itemsRetrieved(Item::List());
    return;
  }

  /*
  QStringList entryList = mbox.entryList();
  mbox.close();

  Item::List items;
  foreach ( const QString &entry, entryList ) {
    const QString rid = col.remoteId() + QDir::separator() + entry;
    Item item;
    item.setRemoteId( rid );
    item.setMimeType( "message/rfc822" );
    item.setSize( md.size( entry ) );
    KMime::Message *msg = new KMime::Message;
    msg->setHead( KMime::CRLFtoLF( md.readEntryHeaders( entry ) ) );
    msg->parse();
    item.setPayload( MessagePtr( msg ) );
    items << item;
  }
  itemsRetrieved( items );
  */
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
