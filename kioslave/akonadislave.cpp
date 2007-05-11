/*
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>

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

#include "akonadislave.h"

#include <libakonadi/itemfetchjob.h>
#include <libakonadi/itemdeletejob.h>
#include <libakonadi/itemserializer.h>
#include <libakonadi/collection.h>
#include <libakonadi/collectionlistjob.h>
#include <libakonadi/collectiondeletejob.h>

#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kdebug.h>
#include <klocale.h>

static const KCmdLineOptions options[] =
{
  { "+protocol", I18N_NOOP( "Protocol name" ), 0 },
  { "+pool", I18N_NOOP( "Socket name" ), 0 },
  { "+app", I18N_NOOP( "Socket name" ), 0 },
  KCmdLineLastOption
};

extern "C" { int KDE_EXPORT kdemain(int argc, char **argv); }

int kdemain(int argc, char **argv) {

  KCmdLineArgs::init(argc, argv, "kio_akonadi", 0, 0, 0);
  KCmdLineArgs::addCmdLineOptions( options );
  KApplication app( false );

  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
  AkonadiSlave slave( args->arg(1), args->arg(2) );
  slave.dispatchLoop();

  return 0;
}

using namespace Akonadi;

AkonadiSlave::AkonadiSlave(const QByteArray & pool_socket, const QByteArray & app_socket) :
    KIO::SlaveBase( "akonadi", pool_socket, app_socket )
{
  kDebug() << k_funcinfo << endl;
}

AkonadiSlave::~ AkonadiSlave()
{
  kDebug() << k_funcinfo << endl;
}

void AkonadiSlave::get(const KUrl & url)
{
  kDebug() << k_funcinfo << url << endl;
  QMap<QString, QString> query = url.queryItems();

  ItemFetchJob *job = new ItemFetchJob( DataReference( query[ QString::fromLatin1("item") ].toInt(), QString() ) );
  if ( !job->exec() ) {
    error( KIO::ERR_INTERNAL, job->errorString() );
    return;
  }
  if ( job->items().count() != 1 ) {
    error( KIO::ERR_DOES_NOT_EXIST, "No such item." );
  } else {
    const Item item = job->items().first();
    QByteArray tmp;
    ItemSerializer::serialize( item, "RFC822", tmp );
    data( tmp );
    data( QByteArray() );
    finished();
  }

  finished();
}

void AkonadiSlave::stat(const KUrl & url)
{
  kDebug() << k_funcinfo << url << endl;

  // Stats for a collection
  if ( Collection::urlIsValid( url ) ) 
  {
      Collection collection = Collection::fromUrl( url );
      // Check that the collection exists.
      CollectionListJob *job = new CollectionListJob( collection, CollectionListJob::Local );
      if ( !job->exec() ) {
        error( KIO::ERR_INTERNAL, job->errorString() );
        return;
      }

      if ( job->collections().count() != 1 ) {
        error( KIO::ERR_DOES_NOT_EXIST, "No such item." );
        return;
      }

      collection = job->collections().first();

      KIO::UDSEntry entry;
      entry.insert( KIO::UDS_NAME, collection.name()  );
      entry.insert( KIO::UDS_MIME_TYPE, QString::fromLatin1("inode/directory") );
      entry.insert( KIO::UDS_FILE_TYPE, S_IFDIR );
      entry.insert( KIO::UDS_URL, url.url() );
      statEntry( entry );
      finished();
  }
  // Stats for an item
  else if ( Item::urlIsValid( url ) ) 
  {
    ItemFetchJob *job = new ItemFetchJob( Item::fromUrl( url ) );

    if ( !job->exec() ) {
      error( KIO::ERR_INTERNAL, job->errorString() );
      return;
    }

    if ( job->items().count() != 1 ) {
      error( KIO::ERR_DOES_NOT_EXIST, "No such item." );
      return;
    }

    const Item item = job->items().first();
    KIO::UDSEntry entry;
    entry.insert( KIO::UDS_NAME, QString::number( item.reference().id() ) );
    entry.insert( KIO::UDS_MIME_TYPE, item.mimeType() );
    entry.insert( KIO::UDS_FILE_TYPE, S_IFREG );

    statEntry( entry );
    finished();
  }
}

void AkonadiSlave::del( const KUrl &url, bool isFile )
{
  kDebug() << k_funcinfo << url << endl;

  if ( !isFile ) // It's a directory
  {
    Collection collection = Collection::fromUrl( url );
    CollectionDeleteJob *job = new CollectionDeleteJob( collection );
    if ( !job->exec() ) {
      error( KIO::ERR_INTERNAL, job->errorString() );
      return;
    } 
    finished();
  } else // It's a file
  {
    ItemDeleteJob* job = new ItemDeleteJob( Item::fromUrl( url ) );
    if ( !job->exec() ) {
      error( KIO::ERR_INTERNAL, job->errorString() );
      return;
    }
    finished();
  }
}

void AkonadiSlave::listDir( const KUrl &url )
{
  kDebug() << k_funcinfo << url << endl;

  if ( !Collection::urlIsValid( url ) )
  {
    error( KIO::ERR_DOES_NOT_EXIST, "No such collection." );
    return;
  }

  // Fetching collections
  Collection collection = Collection::fromUrl( url );
  CollectionListJob *job = new CollectionListJob( collection, CollectionListJob::Flat );
  if ( !job->exec() ) {
    error( KIO::ERR_CANNOT_ENTER_DIRECTORY, job->errorString() );
    return;
  }

  Collection::List collections = job->collections();

  KIO::UDSEntry entry;
  foreach( Collection col, collections )
  {
    kDebug() << "Collection (" << col.id() << ", " << col.name() << ")" << endl;
    entry.clear();
    entry.insert( KIO::UDS_NAME, col.name() );
    entry.insert( KIO::UDS_MIME_TYPE, QString::fromLatin1("inode/directory") );
    entry.insert( KIO::UDS_FILE_TYPE, S_IFDIR );
    entry.insert( KIO::UDS_URL, col.url().url() );
    listEntry( entry, false );
  }

  listEntry( entry, true );

  // Fetching items
  ItemFetchJob* fjob = new ItemFetchJob( collection );

  if ( !fjob->exec() ) {
    error( KIO::ERR_INTERNAL, job->errorString() );
    return;
  }
  Item::List items = fjob->items();

  totalSize( collections.count() + items.count() );
  foreach( Item item, items )
  {
    kDebug() << "Item (" << item.reference().id()  << ")" << endl; 
    entry.clear();
    entry.insert( KIO::UDS_NAME, item.reference().id() );
    entry.insert( KIO::UDS_MIME_TYPE, item.mimeType() );
    entry.insert( KIO::UDS_FILE_TYPE, S_IFREG );
  }

  finished();
}

