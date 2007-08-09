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
#include <libakonadi/collectionlistjob.h>
#include <libakonadi/collectionmodifyjob.h>
#include <libakonadi/itemappendjob.h>
#include <libakonadi/itemfetchjob.h>
#include <libakonadi/itemstorejob.h>
#include <libakonadi/session.h>

#include <kfiledialog.h>
#include <klocale.h>
#include <kbookmarkmanager.h>
#include <kbookmark.h>

#include <QtCore/QDebug>
#include <QtDBus/QDBusConnection>

using namespace Akonadi;


LocalBookmarksResource::LocalBookmarksResource( const QString &id )
    :ResourceBase( id )
{
      qDebug() << "MIMETYPES:" << KBookmark::List::mimeDataTypes();
}

LocalBookmarksResource::~ LocalBookmarksResource()
{
}

bool LocalBookmarksResource::requestItemDelivery( const Akonadi::DataReference &ref, const QStringList &parts, const QDBusMessage &msg )
{
  Q_UNUSED( parts );
  // TODO use remote id to retrieve the item in the file
  qDebug() << "request item delivery";
}

void LocalBookmarksResource::aboutToQuit()
{
  // TODO save to the backend
}

void LocalBookmarksResource::configure()
{
  QString oldFile = settings()->value( "General/Path" ).toString();
  KUrl url;
  if ( !oldFile.isEmpty() )
    url = KUrl::fromPath( oldFile );
  else
    url = KUrl::fromPath( QDir::homePath() );
  QString newFile = KFileDialog::getOpenFileName( url, "*.xml |" + i18nc("Filedialog filter for *.xml", "XML Bookmark file"), 0, i18n("Select Bookmarks File") );
  if ( newFile.isEmpty() )
    return;
  if ( oldFile == newFile )
    return;
  settings()->setValue( "General/Path", newFile );

  mBookmarkManager = KBookmarkManager::managerForFile( newFile, name() );

  synchronize();
}

void LocalBookmarksResource::itemAdded( const Akonadi::Item & item, const Akonadi::Collection& )
{
  // todo save the bookmark in the xml file
}

void LocalBookmarksResource::itemChanged( const Akonadi::Item&, const QStringList& )
{
  qWarning() << "Implement me!";
}

void LocalBookmarksResource::itemRemoved(const Akonadi::DataReference & ref)
{
  // remove the bookmark
}

Collection::List listRecursive( const KBookmarkGroup& parent, const Collection& parentCol )
{
  Collection::List list;
  const QStringList mimeTypes = QStringList() << "message/rfc822" << Collection::collectionMimeType();
  for ( KBookmark it = parent.first(); !it.isNull(); it = parent.next( it ) ) {
    if ( !it.isGroup() )
      continue;

    KBookmarkGroup bkg = it.toGroup();
    Collection col;
    col.setName( bkg.fullText() + '(' + bkg.address() + ')' ); // has to be unique
    col.setRemoteId( bkg.address() );
    col.setParent( parentCol );
    col.setContentTypes( mimeTypes ); // ###
    list << col;
    list << listRecursive( bkg, col );
  }

  return list;
}

void LocalBookmarksResource::retrieveCollections()
{
  qDebug() << "retrieve collections";
  Collection root;
  root.setParent( Collection::root() );
  root.setRemoteId( settings()->value( "General/Path" ).toString() );
  root.setName( name() );
  QStringList mimeTypes;
  mimeTypes << "application/x-xbel" << Collection::collectionMimeType(); // ###
  root.setContentTypes( mimeTypes );

  Collection::List list;
  list << root;
  list << listRecursive( mBookmarkManager->root(), root );
  collectionsRetrieved( list );
}

void LocalBookmarksResource::synchronizeCollection(const Akonadi::Collection & col)
{
  qDebug() << "synchronize collection";
  if ( !col.isValid() )
  {
    qDebug() << "Collection not valid";
    return;
  }

  KBookmark bk = mBookmarkManager->findByAddress( col.remoteId() );

  if ( bk.isNull() || !bk.isGroup() )
  {
    qDebug() << "bk null or bk not group";
    return;
  }

  KBookmarkGroup bkg = bk.toGroup();

  for ( KBookmark it = bkg.first(); !it.isNull(); it = bkg.next( it ) ) {
    Item item( DataReference( -1, it.address() ) );
    item.setMimeType( "application/x-xbel" );
    item.setPayload<KBookmark>( it );
    ItemAppendJob *job = new ItemAppendJob( item, col, this );
    if ( !job->exec() )
      return;
    qDebug() << "item !!!";
  }

  collectionSynchronized();
}

#include "localbookmarksresource.moc"
