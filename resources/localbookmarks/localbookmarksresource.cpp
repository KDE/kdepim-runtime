/*
    Copyright (c) 2006 Till Adam <adam@kde.org>

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

#include "localbookmarksresource.h"

#include "settings.h"
#include "settingsadaptor.h"

#include <kbookmark.h>
#include <kbookmarkmanager.h>
#include <kfiledialog.h>
#include <klocale.h>

using namespace Akonadi;

LocalBookmarksResource::LocalBookmarksResource( const QString &id )
  : ResourceBase( id ), mBookmarkManager( 0 )
{
  new SettingsAdaptor( Settings::self() );
  QDBusConnection::sessionBus().registerObject( QLatin1String( "/Settings" ),
                            Settings::self(), QDBusConnection::ExportAdaptors );

  const QString fileName = Settings::self()->path();
  if (!fileName.isEmpty() ) {
     mBookmarkManager = KBookmarkManager::managerForFile( fileName, name() );
  }
}

LocalBookmarksResource::~LocalBookmarksResource()
{
}

bool LocalBookmarksResource::retrieveItem( const Akonadi::Item &item, const QSet<QByteArray>& )
{
  itemRetrieved( item );

  return true;
}

void LocalBookmarksResource::configure( WId windowId )
{
  const QString oldFile = Settings::self()->path();

  KUrl url;
  if ( !oldFile.isEmpty() )
    url = KUrl::fromPath( oldFile );
  else
    url = KUrl::fromPath( QDir::homePath() );

  const QString newFile = KFileDialog::getOpenFileNameWId( url, "*.xml |" + i18nc( "Filedialog filter for *.xml",
                                                                                   "XML Bookmark file"),
                                                           windowId, i18n( "Select Bookmarks File" ) );

  if ( newFile.isEmpty() ) {
    emit configurationDialogRejected();
    return;
  }

  if ( oldFile == newFile ) {
    emit configurationDialogAccepted();
    return;
  }

  Settings::self()->setPath( newFile );

  mBookmarkManager = KBookmarkManager::managerForFile( newFile, name() );

  Settings::self()->writeConfig();
  synchronize();

  emit configurationDialogAccepted();
}

void LocalBookmarksResource::itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection )
{
  if ( item.mimeType() != QLatin1String( "application/x-xbel" ) ) {
    cancelTask( i18n( "Item is not a bookmark" ) );
    return;
  }

  KBookmark bookmark = item.payload<KBookmark>();
  KBookmark bookmarkGroup = mBookmarkManager->findByAddress( collection.remoteId() );
  if ( !bookmarkGroup.isGroup() ) {
    cancelTask( i18n( "Bookmark group not found" ) );
    return;
  }

  KBookmarkGroup group = bookmarkGroup.toGroup();
  group.addBookmark( bookmark );

  // saves the file
  mBookmarkManager->emitChanged( group );

  changeCommitted( item );
}

void LocalBookmarksResource::itemChanged( const Akonadi::Item& item, const QSet<QByteArray>& )
{
  KBookmark bookmark = item.payload<KBookmark>();

  // saves the file
  mBookmarkManager->emitChanged( bookmark.parentGroup() );

  changeCommitted( item );
}

void LocalBookmarksResource::itemRemoved( const Akonadi::Item &item )
{
  const KBookmark bookmark = mBookmarkManager->findByAddress( item.remoteId() );
  KBookmarkGroup bookmarkGroup = bookmark.parentGroup();

  bookmarkGroup.deleteBookmark( bookmark );

  // saves the file
  mBookmarkManager->emitChanged( bookmarkGroup );

  changeCommitted( item );
}

static Collection::List listRecursive( const KBookmarkGroup &parentGroup, const Collection &parentCollection )
{
  const QStringList mimeTypes = QStringList() << "application/x-xbel" << Collection::mimeType();

  Collection::List collections;

  for ( KBookmark it = parentGroup.first(); !it.isNull(); it = parentGroup.next( it ) ) {

    if ( !it.isGroup() )
      continue;

    KBookmarkGroup bookmarkGroup = it.toGroup();
    Collection collection;
    collection.setName( bookmarkGroup.fullText() + '(' + bookmarkGroup.address() + ')' ); // has to be unique
    collection.setRemoteId( bookmarkGroup.address() );
    collection.setParentCollection( parentCollection );
    collection.setContentMimeTypes( mimeTypes ); // ###

    collections << collection;
    collections << listRecursive( bookmarkGroup, collection );
  }

  return collections;
}

void LocalBookmarksResource::retrieveCollections()
{
  Collection root;
  root.setParentCollection( Collection::root() );
  root.setRemoteId( Settings::self()->path() );
  root.setName( name() );
  QStringList mimeTypes;
  mimeTypes << "application/x-xbel" << Collection::mimeType();
  root.setContentMimeTypes( mimeTypes );


  if (!mBookmarkManager) {
     mBookmarkManager = KBookmarkManager::managerForFile( Settings::self()->path(), name() );
  }

  Collection::List list;
  list << root;
  list << listRecursive( mBookmarkManager->root(), root );

  collectionsRetrieved( list );
}

void LocalBookmarksResource::retrieveItems( const Akonadi::Collection &collection )
{
  if ( !collection.isValid() ) {
    cancelTask( i18n( "Bookmark collection is invalid" ) );
    return;
  }

  KBookmarkGroup bookmarkGroup;
  if ( collection.remoteId() == Settings::self()->path() ) {
    bookmarkGroup = mBookmarkManager->root();
  } else {

    const KBookmark bookmark = mBookmarkManager->findByAddress( collection.remoteId() );
    if ( bookmark.isNull() || !bookmark.isGroup() ) {
      cancelTask( i18n( "Bookmark collection is invalid" ) );
      return;
    }

    bookmarkGroup = bookmark.toGroup();
  }

  Item::List itemList;
  for ( KBookmark it = bookmarkGroup.first(); !it.isNull(); it = bookmarkGroup.next( it ) ) {

    if ( it.isGroup() || it.isSeparator() || it.isNull() )
      continue;

    Item item;
    item.setRemoteId( it.address() );
    item.setMimeType( "application/x-xbel" );
    item.setPayload<KBookmark>( it );
    itemList.append( item );
  }

  itemsRetrieved( itemList );
}

AKONADI_RESOURCE_MAIN( LocalBookmarksResource )

#include "localbookmarksresource.moc"
