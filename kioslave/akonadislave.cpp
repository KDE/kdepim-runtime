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

#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/itemdeletejob.h>
#include <akonadi/collection.h>
#include <akonadi/collectionfetchjob.h>
#include <akonadi/collectiondeletejob.h>

#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kdebug.h>
#include <klocale.h>

extern "C" { int KDE_EXPORT kdemain(int argc, char **argv); }

int kdemain(int argc, char **argv) {

  KCmdLineArgs::init(argc, argv, "kio_akonadi", 0, KLocalizedString(), 0);

  KCmdLineOptions options;
  options.add("+protocol", ki18n( "Protocol name" ));
  options.add("+pool", ki18n( "Socket name" ));
  options.add("+app", ki18n( "Socket name" ));
  KCmdLineArgs::addCmdLineOptions( options );
  KApplication app( false );

  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
  AkonadiSlave slave( args->arg(1).toLocal8Bit(), args->arg(2).toLocal8Bit() );
  slave.dispatchLoop();

  return 0;
}

using namespace Akonadi;

AkonadiSlave::AkonadiSlave(const QByteArray & pool_socket, const QByteArray & app_socket) :
    KIO::SlaveBase( "akonadi", pool_socket, app_socket )
{
  kDebug( 7129 ) << "kio_akonadi starting up";
}

AkonadiSlave::~ AkonadiSlave()
{
  kDebug( 7129 ) << "kio_akonadi shutting down";
}

void AkonadiSlave::get(const KUrl & url)
{
  const Item item = Item::fromUrl( url );
  ItemFetchJob *job = new ItemFetchJob( item );
  job->fetchScope().fetchFullPayload();

  if ( !job->exec() ) {
    error( KIO::ERR_INTERNAL, job->errorString() );
    return;
  }

  if ( job->items().count() != 1 ) {
    error( KIO::ERR_DOES_NOT_EXIST, "No such item." );
  } else {
    const Item item = job->items().first();
    QByteArray tmp = item.payloadData();
    data( tmp );
    data( QByteArray() );
    finished();
  }

  finished();
}

void AkonadiSlave::stat(const KUrl & url)
{
  kDebug( 7129 ) << url;

  // Stats for a collection
  if ( Collection::fromUrl( url ).isValid() )
  {
      Collection collection = Collection::fromUrl( url );

      if ( collection != Collection::root() ) {
        // Check that the collection exists.
        CollectionFetchJob *job = new CollectionFetchJob( collection, CollectionFetchJob::Base );
        if ( !job->exec() ) {
          error( KIO::ERR_INTERNAL, job->errorString() );
          return;
        }

        if ( job->collections().count() != 1 ) {
          error( KIO::ERR_DOES_NOT_EXIST, "No such item." );
          return;
        }

        collection = job->collections().first();
      }

      KIO::UDSEntry entry;
      entry.insert( KIO::UDSEntry::UDS_NAME, collection.name()  );
      entry.insert( KIO::UDSEntry::UDS_MIME_TYPE, Collection::mimeType() );
      entry.insert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR );
      entry.insert( KIO::UDSEntry::UDS_URL, url.url() );
      statEntry( entry );
      finished();
  }
  // Stats for an item
  else if ( Item::fromUrl( url ).isValid() )
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
    entry.insert( KIO::UDSEntry::UDS_NAME, QString::number( item.id() ) );
    entry.insert( KIO::UDSEntry::UDS_MIME_TYPE, item.mimeType() );
    entry.insert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFREG );

    statEntry( entry );
    finished();
  }
}

void AkonadiSlave::del( const KUrl &url, bool isFile )
{
  kDebug( 7129 ) << url;

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
  kDebug( 7129 ) << url;

  if ( !Collection::fromUrl( url ).isValid() )
  {
    error( KIO::ERR_DOES_NOT_EXIST, "No such collection." );
    return;
  }

  // Fetching collections
  Collection collection = Collection::fromUrl( url );
  if ( !collection.isValid() ) {
    error( KIO::ERR_DOES_NOT_EXIST, "No such collection." );
    return;
  }
  CollectionFetchJob *job = new CollectionFetchJob( collection, CollectionFetchJob::FirstLevel );
  if ( !job->exec() ) {
    error( KIO::ERR_CANNOT_ENTER_DIRECTORY, job->errorString() );
    return;
  }

  Collection::List collections = job->collections();

  KIO::UDSEntry entry;
  foreach( Collection col, collections )
  {
    kDebug( 7129 ) <<"Collection (" << col.id() <<"," << col.name() <<")";
    entry.clear();
    entry.insert( KIO::UDSEntry::UDS_NAME, col.name() );
    entry.insert( KIO::UDSEntry::UDS_MIME_TYPE, Collection::mimeType() );
    entry.insert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR );
    entry.insert( KIO::UDSEntry::UDS_URL, col.url().url() );
    listEntry( entry, false );
  }

  // Fetching items
  if ( collection != Collection::root() ) {
    ItemFetchJob* fjob = new ItemFetchJob( collection );
    if ( !fjob->exec() ) {
      error( KIO::ERR_INTERNAL, job->errorString() );
      return;
    }
    Item::List items = fjob->items();
    totalSize( collections.count() + items.count() );
    foreach( Item item, items )
    {
      kDebug( 7129 ) <<"Item (" << item.id()  <<")";
      entry.clear();
      entry.insert( KIO::UDSEntry::UDS_NAME, QString::number( item.id() ) );
      entry.insert( KIO::UDSEntry::UDS_MIME_TYPE, item.mimeType() );
      entry.insert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFREG );
      entry.insert( KIO::UDSEntry::UDS_URL, item.url().url() );
      listEntry( entry, false );
    }
  }

  listEntry( entry, true );
  finished();
}

