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

#include <akonadi/collectionfetchjob.h>
#include <akonadi/collectionmodifyjob.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/session.h>

#include <kdebug.h>
#include <kfiledialog.h>
#include <klocale.h>
#include <kbookmarkmanager.h>
#include <kbookmark.h>

#include <QtDBus/QDBusConnection>

using namespace Akonadi;


LocalBookmarksResource::LocalBookmarksResource( const QString &id )
    :ResourceBase( id )
{
  new SettingsAdaptor( Settings::self() );
}

LocalBookmarksResource::~ LocalBookmarksResource()
{
}

bool LocalBookmarksResource::retrieveItem( const Akonadi::Item &item, const QList<QByteArray> &parts )
{
  Q_UNUSED( parts );
  // TODO use remote id to retrieve the item in the file
  return true;
}

void LocalBookmarksResource::aboutToQuit()
{
  // TODO save to the backend
}

void LocalBookmarksResource::configure( WId windowId )
{
  QString oldFile = Settings::self()->path();
  KUrl url;
  if ( !oldFile.isEmpty() )
    url = KUrl::fromPath( oldFile );
  else
    url = KUrl::fromPath( QDir::homePath() );
  QString newFile = KFileDialog::getOpenFileNameWId( url, "*.xml |" + i18nc("Filedialog filter for *.xml", "XML Bookmark file"), windowId, i18n("Select Bookmarks File") );
  if ( newFile.isEmpty() )
    return;
  if ( oldFile == newFile )
    return;
  Settings::self()->setPath( newFile );

  mBookmarkManager = KBookmarkManager::managerForFile( newFile, name() );

  synchronize();
}

void LocalBookmarksResource::itemAdded( const Akonadi::Item & item, const Akonadi::Collection& col )
{
  if ( item.mimeType() != QLatin1String( "application/x-xbel" ) )
    return;

  KBookmark bk = item.payload<KBookmark>();
  KBookmark bkg = mBookmarkManager->findByAddress( col.remoteId() );
  if ( !bkg.isGroup() )
    return;

  KBookmarkGroup group = bkg.toGroup();
  group.addBookmark( bk );

  // saves the file
  mBookmarkManager->emitChanged( group );
}

void LocalBookmarksResource::itemChanged( const Akonadi::Item& item, const QSet<QByteArray>& )
{
  KBookmark bk = item.payload<KBookmark>();

  // saves the file
  mBookmarkManager->emitChanged( bk.parentGroup() );
}

void LocalBookmarksResource::itemRemoved(const Akonadi::Item & item)
{
  KBookmark bk = mBookmarkManager->findByAddress( item.remoteId() );
  KBookmarkGroup bkg = bk.parentGroup();

  if ( !bk.isNull() )
    return;

  bkg.deleteBookmark( bk );

  // saves the file
  mBookmarkManager->emitChanged( bkg );
}

Collection::List listRecursive( const KBookmarkGroup& parent, const Collection& parentCol )
{
  Collection::List list;
  const QStringList mimeTypes = QStringList() << "message/rfc822" << Collection::mimeType();

  for ( KBookmark it = parent.first(); !it.isNull(); it = parent.next( it ) ) {
    if ( !it.isGroup() )
      continue;

    KBookmarkGroup bkg = it.toGroup();
    Collection col;
    col.setName( bkg.fullText() + '(' + bkg.address() + ')' ); // has to be unique
    col.setRemoteId( bkg.address() );
    col.setParent( parentCol );
    col.setContentMimeTypes( mimeTypes ); // ###
    list << col;
    list << listRecursive( bkg, col );
  }

  return list;
}

void LocalBookmarksResource::retrieveCollections()
{
  Collection root;
  root.setParent( Collection::root() );
  root.setRemoteId( Settings::self()->path() );
  root.setName( name() );
  QStringList mimeTypes;
  mimeTypes << "application/x-xbel" << Collection::mimeType(); // ###
  root.setContentMimeTypes( mimeTypes );

  Collection::List list;
  list << root;
  list << listRecursive( mBookmarkManager->root(), root );

  collectionsRetrieved( list );
}

void LocalBookmarksResource::retrieveItems( const Akonadi::Collection & col )
{
  if ( !col.isValid() )
  {
    kDebug( 5253 ) << "Collection not valid";
    return;
  }

  KBookmarkGroup bkg;
  if ( col.remoteId() == Settings::self()->path() ) {
    bkg = mBookmarkManager->root();
  } else {

    KBookmark bk = mBookmarkManager->findByAddress( col.remoteId() );
    if ( bk.isNull() || !bk.isGroup() )
      return;

    bkg = bk.toGroup();
  }

  Item::List itemList;
  for ( KBookmark it = bkg.first(); !it.isNull(); it = bkg.next( it ) ) {

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
