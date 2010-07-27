/*  This file is part of the KDE project
    Copyright (C) 2010 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.net
    Author: Kevin Krammer, krake@kdab.com

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

#include "mixedmaildirstore.h"

#include "testdatautil.h"

#include "filestore/collectionmovejob.h"
#include "filestore/itemfetchjob.h"

#include "libmaildir/maildir.h"

#include <KTempDir>

#include <qtest_kde.h>

using namespace Akonadi;

class CollectionMoveTest : public QObject
{
  Q_OBJECT

  public:
    CollectionMoveTest() : QObject(), mStore( 0 ), mDir( 0 ) {}

    ~CollectionMoveTest() {
      delete mStore;
      delete mDir;
    }

  private:
    MixedMaildirStore *mStore;
    KTempDir *mDir;

  private Q_SLOTS:
    void init();
    void cleanup();
    void testMoveToTopLevel();
    void testMoveToMaildir();
    void testMoveToMBox();
};

void CollectionMoveTest::init()
{
  mStore = new MixedMaildirStore;

  mDir = new KTempDir;
  QVERIFY( mDir->exists() );
}

void CollectionMoveTest::cleanup()
{
  delete mStore;
  mStore = 0;
  delete mDir;
  mDir = 0;
}

void CollectionMoveTest::testMoveToTopLevel()
{
}

void CollectionMoveTest::testMoveToMaildir()
{
  QDir topDir( mDir->name() );

  // top level dir
  QVERIFY( TestDataUtil::installFolder( QLatin1String( "maildir" ), topDir.path(), QLatin1String( "collection1" ) ) );
  QFileInfo fileInfo1( topDir, QLatin1String( "collection1" ) );
  QVERIFY( TestDataUtil::installFolder( QLatin1String( "maildir" ), topDir.path(), QLatin1String( "collection2" ) ) );
  QFileInfo fileInfo2( topDir, QLatin1String( "collection2" ) );
  QVERIFY( TestDataUtil::installFolder( QLatin1String( "mbox" ), topDir.path(), QLatin1String( "collection3" ) ) );
  QFileInfo fileInfo3( topDir, QLatin1String( "collection3" ) );
  QVERIFY( TestDataUtil::installFolder( QLatin1String( "mbox" ), topDir.path(), QLatin1String( "collection4" ) ) );
  QFileInfo fileInfo4( topDir, QLatin1String( "collection4" ) );

  // first level maildir parent
  QDir subDir1 = topDir;
  QVERIFY( subDir1.mkdir( QLatin1String( ".collection1.directory" ) ) );
  QVERIFY( subDir1.cd( QLatin1String( ".collection1.directory" ) ) );
  QVERIFY( TestDataUtil::installFolder( QLatin1String( "maildir" ), subDir1.path(), QLatin1String( "collection1_1" ) ) );
  QVERIFY( TestDataUtil::installFolder( QLatin1String( "mbox" ), subDir1.path(), QLatin1String( "collection1_2" ) ) );

  // first level mbox parent
  QDir subDir4 = topDir;
  QVERIFY( subDir4.mkdir( QLatin1String( ".collection4.directory" ) ) );
  QVERIFY( subDir4.cd( QLatin1String( ".collection4.directory" ) ) );
  QVERIFY( TestDataUtil::installFolder( QLatin1String( "mbox" ), subDir4.path(), QLatin1String( "collection4_1" ) ) );
  QVERIFY( TestDataUtil::installFolder( QLatin1String( "maildir" ), subDir4.path(), QLatin1String( "collection4_2" ) ) );
  QVERIFY( TestDataUtil::installFolder( QLatin1String( "mbox" ), subDir4.path(), QLatin1String( "collection4_3" ) ) );
  QVERIFY( TestDataUtil::installFolder( QLatin1String( "maildir" ), subDir4.path(), QLatin1String( "collection4_4" ) ) );

  // target maildir
  KPIM::Maildir topLevelMd( topDir.path(), true );
  KPIM::Maildir targetMd( topLevelMd.addSubFolder( QLatin1String( "target" ) ), false );
  QVERIFY( targetMd.isValid() );
  QDir subDirTarget;

  mStore->setPath( topDir.path() );

  // common variables
  FileStore::CollectionMoveJob *job = 0;
  FileStore::ItemFetchJob *itemFetch = 0;
  Collection collection;
  const QVariant colListVar = QVariant::fromValue<Collection::List>( Collection::List() );
  QVariant var;
  Collection::List collections;
  Item::List items;
  QMap<QByteArray, int> flagCounts;

  Collection target;
  target.setName( QLatin1String( "target" ) );
  target.setRemoteId( QLatin1String( "target" ) );
  target.setParentCollection( mStore->topLevelCollection() );

  // test move leaf maildir into sibling
  Collection collection2;
  collection2.setName( QLatin1String( "collection2" ) );
  collection2.setRemoteId( QLatin1String( "collection2" ) );
  collection2.setParentCollection( mStore->topLevelCollection() );

  job = mStore->moveCollection( collection2, target );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  subDirTarget = topDir;
  QVERIFY( subDirTarget.cd( QLatin1String( ".target.directory" ) ) );

  collection = job->collection();
  QCOMPARE( collection.remoteId(), collection2.remoteId() );
  QCOMPARE( collection.parentCollection(), target );

  fileInfo2.refresh();
  QVERIFY( !fileInfo2.exists() );
  fileInfo2 = QFileInfo( subDirTarget, collection.remoteId() );
  QVERIFY( fileInfo2.exists() );

  // check for index preservation
  var = job->property( "onDiskIndexInvalidated" );
  QVERIFY( var.isValid() );
  QCOMPARE( var.userType(), colListVar.userType() );

  collections = var.value<Collection::List>();
  QCOMPARE( (int)collections.count(), 1 );
  QCOMPARE( collections.first(), collection );

  // get the items and check the flags (see data/README)
  itemFetch = mStore->fetchItems( collection );
  QVERIFY( itemFetch->exec() );
  QCOMPARE( itemFetch->error(), 0 );

  items = itemFetch->items();
  QCOMPARE( (int)items.count(), 4 );
  Q_FOREACH( const Item &item, items ) {
    Q_FOREACH( const QByteArray &flag, item.flags() ) {
      ++flagCounts[ flag ];
    }
  }

  QCOMPARE( flagCounts[ "\\SEEN" ], 2 );
  QCOMPARE( flagCounts[ "\\FLAGGED" ], 1 );
  QCOMPARE( flagCounts[ "$TODO" ], 1 );
  flagCounts.clear();

  // test move leaf mbox into sibling
  Collection collection3;
  collection3.setName( QLatin1String( "collection3" ) );
  collection3.setRemoteId( QLatin1String( "collection3" ) );
  collection3.setParentCollection( mStore->topLevelCollection() );

  job = mStore->moveCollection( collection3, target );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  collection = job->collection();
  QCOMPARE( collection.remoteId(), collection3.remoteId() );
  QCOMPARE( collection.parentCollection(), target );

  fileInfo3.refresh();
  QVERIFY( !fileInfo3.exists() );
  fileInfo3 = QFileInfo( subDirTarget, collection.remoteId() );
  QVERIFY( fileInfo3.exists() );

  // check for index preservation
  var = job->property( "onDiskIndexInvalidated" );
  QVERIFY( var.isValid() );
  QCOMPARE( var.userType(), colListVar.userType() );

  collections = var.value<Collection::List>();
  QCOMPARE( (int)collections.count(), 1 );
  QCOMPARE( collections.first(), collection );

  // get the items and check the flags (see data/README)
  itemFetch = mStore->fetchItems( collection );
  QVERIFY( itemFetch->exec() );
  QCOMPARE( itemFetch->error(), 0 );

  items = itemFetch->items();
  QCOMPARE( (int)items.count(), 4 );
  Q_FOREACH( const Item &item, items ) {
    Q_FOREACH( const QByteArray &flag, item.flags() ) {
      ++flagCounts[ flag ];
    }
  }

  QCOMPARE( flagCounts[ "\\SEEN" ], 2 );
  QCOMPARE( flagCounts[ "\\FLAGGED" ], 1 );
  QCOMPARE( flagCounts[ "$TODO" ], 1 );
  flagCounts.clear();

  // test move maildir with subtree into sibling
  Collection collection1;
  collection1.setName( QLatin1String( "collection1" ) );
  collection1.setRemoteId( QLatin1String( "collection1" ) );
  collection1.setParentCollection( mStore->topLevelCollection() );

  // load sub collection index data to check for correct cache updates
  Collection collection1_1;
  collection1_1.setName( QLatin1String( "collection1_1" ) );
  collection1_1.setRemoteId( QLatin1String( "collection1_1" ) );
  collection1_1.setParentCollection( collection1 );
  itemFetch = mStore->fetchItems( collection1_1 );
  QVERIFY( itemFetch->exec() );

  Collection collection1_2;
  collection1_2.setName( QLatin1String( "collection1_2" ) );
  collection1_2.setRemoteId( QLatin1String( "collection1_2" ) );
  collection1_2.setParentCollection( collection1 );
  itemFetch = mStore->fetchItems( collection1_2 );
  QVERIFY( itemFetch->exec() );

  job = mStore->moveCollection( collection1, target );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  collection = job->collection();
  QCOMPARE( collection.remoteId(), collection1.remoteId() );
  QCOMPARE( collection.parentCollection(), target );

  fileInfo1.refresh();
  QVERIFY( !fileInfo1.exists() );
  fileInfo1 = QFileInfo( subDirTarget, collection.remoteId() );
  QVERIFY( fileInfo1.exists() );
  QVERIFY( !subDir1.exists() );
  subDir1 = subDirTarget;
  QVERIFY( subDir1.cd( QLatin1String( ".collection1.directory" ) ) );
  QCOMPARE( subDir1.entryList( QStringList() << QLatin1String( "collection*" ) ),
            QStringList() << QLatin1String( "collection1_1" ) << QLatin1String( "collection1_2" ) );

  // check for index preservation
  var = job->property( "onDiskIndexInvalidated" );
  QVERIFY( var.isValid() );
  QCOMPARE( var.userType(), colListVar.userType() );

  collections = var.value<Collection::List>();
  QCOMPARE( (int)collections.count(), 1 );
  QCOMPARE( collections.first(), collection );

  // get the items and check the flags (see data/README)
  itemFetch = mStore->fetchItems( collection );
  QVERIFY( itemFetch->exec() );
  QCOMPARE( itemFetch->error(), 0 );

  items = itemFetch->items();
  QCOMPARE( (int)items.count(), 4 );
  Q_FOREACH( const Item &item, items ) {
    Q_FOREACH( const QByteArray &flag, item.flags() ) {
      ++flagCounts[ flag ];
    }
  }

  QCOMPARE( flagCounts[ "\\SEEN" ], 2 );
  QCOMPARE( flagCounts[ "\\FLAGGED" ], 1 );
  QCOMPARE( flagCounts[ "$TODO" ], 1 );
  flagCounts.clear();

  // check for children cache path updates
  collection1.setParentCollection( target );
  collection1_1.setParentCollection( collection1 );
  collection1_2.setParentCollection( collection1 );

  itemFetch = mStore->fetchItems( collection1_1 );
  QVERIFY( itemFetch->exec() );
  QCOMPARE( itemFetch->error(), 0 );

  items = itemFetch->items();
  QCOMPARE( (int)items.count(), 4 );
  Q_FOREACH( const Item &item, items ) {
    Q_FOREACH( const QByteArray &flag, item.flags() ) {
      ++flagCounts[ flag ];
    }
  }

  QCOMPARE( flagCounts[ "\\SEEN" ], 2 );
  QCOMPARE( flagCounts[ "\\FLAGGED" ], 1 );
  QCOMPARE( flagCounts[ "$TODO" ], 1 );
  flagCounts.clear();

  itemFetch = mStore->fetchItems( collection1_2 );
  QVERIFY( itemFetch->exec() );
  QCOMPARE( itemFetch->error(), 0 );

  items = itemFetch->items();
  QCOMPARE( (int)items.count(), 4 );
  Q_FOREACH( const Item &item, items ) {
    Q_FOREACH( const QByteArray &flag, item.flags() ) {
      ++flagCounts[ flag ];
    }
  }

  QCOMPARE( flagCounts[ "\\SEEN" ], 2 );
  QCOMPARE( flagCounts[ "\\FLAGGED" ], 1 );
  QCOMPARE( flagCounts[ "$TODO" ], 1 );
  flagCounts.clear();

  // test move mbox with subtree into sibling
  Collection collection4;
  collection4.setName( QLatin1String( "collection4" ) );
  collection4.setRemoteId( QLatin1String( "collection4" ) );
  collection4.setParentCollection( mStore->topLevelCollection() );

  // load sub collection index data to check for correct cache updates
  Collection collection4_1;
  collection4_1.setName( QLatin1String( "collection4_1" ) );
  collection4_1.setRemoteId( QLatin1String( "collection4_1" ) );
  collection4_1.setParentCollection( collection4 );
  itemFetch = mStore->fetchItems( collection4_1 );
  QVERIFY( itemFetch->exec() );

  Collection collection4_2;
  collection4_2.setName( QLatin1String( "collection4_2" ) );
  collection4_2.setRemoteId( QLatin1String( "collection4_2" ) );
  collection4_2.setParentCollection( collection4 );
  itemFetch = mStore->fetchItems( collection4_2 );
  QVERIFY( itemFetch->exec() );

  job = mStore->moveCollection( collection4, target );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  collection = job->collection();
  QCOMPARE( collection.remoteId(), collection4.remoteId() );
  QCOMPARE( collection.parentCollection(), target );

  fileInfo4.refresh();
  QVERIFY( !fileInfo4.exists() );
  fileInfo4 = QFileInfo( subDirTarget, collection.remoteId() );
  QVERIFY( fileInfo4.exists() );
  QVERIFY( !subDir4.exists() );
  subDir4 = subDirTarget;
  QVERIFY( subDir4.cd( QLatin1String( ".collection4.directory" ) ) );
  QCOMPARE( subDir4.entryList( QStringList() << QLatin1String( "collection*" ) ),
            QStringList() << QLatin1String( "collection4_1" ) << QLatin1String( "collection4_2" )
                          << QLatin1String( "collection4_3" ) << QLatin1String( "collection4_4" )
          );

  // check for index preservation
  var = job->property( "onDiskIndexInvalidated" );
  QVERIFY( var.isValid() );
  QCOMPARE( var.userType(), colListVar.userType() );

  collections = var.value<Collection::List>();
  QCOMPARE( (int)collections.count(), 1 );
  QCOMPARE( collections.first(), collection );

  // get the items and check the flags (see data/README)
  itemFetch = mStore->fetchItems( collection );
  QVERIFY( itemFetch->exec() );
  QCOMPARE( itemFetch->error(), 0 );

  items = itemFetch->items();
  QCOMPARE( (int)items.count(), 4 );
  Q_FOREACH( const Item &item, items ) {
    Q_FOREACH( const QByteArray &flag, item.flags() ) {
      ++flagCounts[ flag ];
    }
  }

  QCOMPARE( flagCounts[ "\\SEEN" ], 2 );
  QCOMPARE( flagCounts[ "\\FLAGGED" ], 1 );
  QCOMPARE( flagCounts[ "$TODO" ], 1 );
  flagCounts.clear();

  // check for children cache path updates
  collection4.setParentCollection( target );
  collection4_1.setParentCollection( collection4 );
  collection4_2.setParentCollection( collection4 );

  itemFetch = mStore->fetchItems( collection4_1 );
  QVERIFY( itemFetch->exec() );
  QCOMPARE( itemFetch->error(), 0 );

  items = itemFetch->items();
  QCOMPARE( (int)items.count(), 4 );
  Q_FOREACH( const Item &item, items ) {
    Q_FOREACH( const QByteArray &flag, item.flags() ) {
      ++flagCounts[ flag ];
    }
  }

  QCOMPARE( flagCounts[ "\\SEEN" ], 2 );
  QCOMPARE( flagCounts[ "\\FLAGGED" ], 1 );
  QCOMPARE( flagCounts[ "$TODO" ], 1 );
  flagCounts.clear();

  itemFetch = mStore->fetchItems( collection4_2 );
  QVERIFY( itemFetch->exec() );
  QCOMPARE( itemFetch->error(), 0 );

  items = itemFetch->items();
  QCOMPARE( (int)items.count(), 4 );
  Q_FOREACH( const Item &item, items ) {
    Q_FOREACH( const QByteArray &flag, item.flags() ) {
      ++flagCounts[ flag ];
    }
  }

  QCOMPARE( flagCounts[ "\\SEEN" ], 2 );
  QCOMPARE( flagCounts[ "\\FLAGGED" ], 1 );
  QCOMPARE( flagCounts[ "$TODO" ], 1 );
  flagCounts.clear();

  // move from parent maildir to parent's sibling
  collection2.setParentCollection( target );

  job = mStore->moveCollection( collection1_1, collection2 );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  QDir subDir2 = subDirTarget;
  QVERIFY( subDir2.cd( QLatin1String( ".collection2.directory" ) ) );

  collection = job->collection();
  QCOMPARE( collection.remoteId(), collection1_1.remoteId() );
  QCOMPARE( collection.parentCollection(), collection2 );

  QFileInfo fileInfo1_1( subDir1, collection.remoteId() );
  QVERIFY( !fileInfo1_1.exists() );
  fileInfo1_1 = QFileInfo( subDir2, collection.remoteId() );
  QVERIFY( fileInfo1_1.exists() );

  // check for index preservation
  var = job->property( "onDiskIndexInvalidated" );
  QVERIFY( var.isValid() );
  QCOMPARE( var.userType(), colListVar.userType() );

  collections = var.value<Collection::List>();
  QCOMPARE( (int)collections.count(), 1 );
  QCOMPARE( collections.first(), collection );

  // get the items and check the flags (see data/README)
  itemFetch = mStore->fetchItems( collection );
  QVERIFY( itemFetch->exec() );
  QCOMPARE( itemFetch->error(), 0 );

  items = itemFetch->items();
  QCOMPARE( (int)items.count(), 4 );
  Q_FOREACH( const Item &item, items ) {
    Q_FOREACH( const QByteArray &flag, item.flags() ) {
      ++flagCounts[ flag ];
    }
  }

  QCOMPARE( flagCounts[ "\\SEEN" ], 2 );
  QCOMPARE( flagCounts[ "\\FLAGGED" ], 1 );
  QCOMPARE( flagCounts[ "$TODO" ], 1 );
  flagCounts.clear();

  // move from parent maildir to parent's sibling
  job = mStore->moveCollection( collection1_2, collection2 );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  collection = job->collection();
  QCOMPARE( collection.remoteId(), collection1_2.remoteId() );
  QCOMPARE( collection.parentCollection(), collection2 );

  QFileInfo fileInfo1_2( subDir1, collection.remoteId() );
  QVERIFY( !fileInfo1_2.exists() );
  fileInfo1_2 = QFileInfo( subDir2, collection.remoteId() );
  QVERIFY( fileInfo1_2.exists() );

  // check for index preservation
  var = job->property( "onDiskIndexInvalidated" );
  QVERIFY( var.isValid() );
  QCOMPARE( var.userType(), colListVar.userType() );

  collections = var.value<Collection::List>();
  QCOMPARE( (int)collections.count(), 1 );
  QCOMPARE( collections.first(), collection );

  // get the items and check the flags (see data/README)
  itemFetch = mStore->fetchItems( collection );
  QVERIFY( itemFetch->exec() );
  QCOMPARE( itemFetch->error(), 0 );

  items = itemFetch->items();
  QCOMPARE( (int)items.count(), 4 );
  Q_FOREACH( const Item &item, items ) {
    Q_FOREACH( const QByteArray &flag, item.flags() ) {
      ++flagCounts[ flag ];
    }
  }

  QCOMPARE( flagCounts[ "\\SEEN" ], 2 );
  QCOMPARE( flagCounts[ "\\FLAGGED" ], 1 );
  QCOMPARE( flagCounts[ "$TODO" ], 1 );
  flagCounts.clear();

  // move from parent mbox to parent's sibling
  job = mStore->moveCollection( collection4_1, collection2 );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  collection = job->collection();
  QCOMPARE( collection.remoteId(), collection4_1.remoteId() );
  QCOMPARE( collection.parentCollection(), collection2 );

  QFileInfo fileInfo4_1( subDir4, collection.remoteId() );
  QVERIFY( !fileInfo4_1.exists() );
  fileInfo4_1 = QFileInfo( subDir2, collection.remoteId() );
  QVERIFY( fileInfo4_1.exists() );

  // check for index preservation
  var = job->property( "onDiskIndexInvalidated" );
  QVERIFY( var.isValid() );
  QCOMPARE( var.userType(), colListVar.userType() );

  collections = var.value<Collection::List>();
  QCOMPARE( (int)collections.count(), 1 );
  QCOMPARE( collections.first(), collection );

  // get the items and check the flags (see data/README)
  itemFetch = mStore->fetchItems( collection );
  QVERIFY( itemFetch->exec() );
  QCOMPARE( itemFetch->error(), 0 );

  items = itemFetch->items();
  QCOMPARE( (int)items.count(), 4 );
  Q_FOREACH( const Item &item, items ) {
    Q_FOREACH( const QByteArray &flag, item.flags() ) {
      ++flagCounts[ flag ];
    }
  }

  QCOMPARE( flagCounts[ "\\SEEN" ], 2 );
  QCOMPARE( flagCounts[ "\\FLAGGED" ], 1 );
  QCOMPARE( flagCounts[ "$TODO" ], 1 );
  flagCounts.clear();

  // move from parent mbox to parent's sibling
  job = mStore->moveCollection( collection4_2, collection2 );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  collection = job->collection();
  QCOMPARE( collection.remoteId(), collection4_2.remoteId() );
  QCOMPARE( collection.parentCollection(), collection2 );

  QFileInfo fileInfo4_2( subDir4, collection.remoteId() );
  QVERIFY( !fileInfo4_2.exists() );
  fileInfo4_2 = QFileInfo( subDir2, collection.remoteId() );
  QVERIFY( fileInfo4_2.exists() );

  // check for index preservation
  var = job->property( "onDiskIndexInvalidated" );
  QVERIFY( var.isValid() );
  QCOMPARE( var.userType(), colListVar.userType() );

  collections = var.value<Collection::List>();
  QCOMPARE( (int)collections.count(), 1 );
  QCOMPARE( collections.first(), collection );

  // get the items and check the flags (see data/README)
  itemFetch = mStore->fetchItems( collection );
  QVERIFY( itemFetch->exec() );
  QCOMPARE( itemFetch->error(), 0 );

  items = itemFetch->items();
  QCOMPARE( (int)items.count(), 4 );
  Q_FOREACH( const Item &item, items ) {
    Q_FOREACH( const QByteArray &flag, item.flags() ) {
      ++flagCounts[ flag ];
    }
  }

  QCOMPARE( flagCounts[ "\\SEEN" ], 2 );
  QCOMPARE( flagCounts[ "\\FLAGGED" ], 1 );
  QCOMPARE( flagCounts[ "$TODO" ], 1 );
  flagCounts.clear();

  // move from parent maildir to grandparent
  collection1_1.setParentCollection( collection2 );

  job = mStore->moveCollection( collection1_1, target );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  collection = job->collection();
  QCOMPARE( collection.remoteId(), collection1_1.remoteId() );
  QCOMPARE( collection.parentCollection(), target );

  fileInfo1_1.refresh();
  QVERIFY( !fileInfo1_1.exists() );
  fileInfo1_1 = QFileInfo( subDirTarget, collection.remoteId() );
  QVERIFY( fileInfo1_1.exists() );

  // check for index preservation
  var = job->property( "onDiskIndexInvalidated" );
  QVERIFY( var.isValid() );
  QCOMPARE( var.userType(), colListVar.userType() );

  collections = var.value<Collection::List>();
  QCOMPARE( (int)collections.count(), 1 );
  QCOMPARE( collections.first(), collection );

  // get the items and check the flags (see data/README)
  itemFetch = mStore->fetchItems( collection );
  QVERIFY( itemFetch->exec() );
  QCOMPARE( itemFetch->error(), 0 );

  items = itemFetch->items();
  QCOMPARE( (int)items.count(), 4 );
  Q_FOREACH( const Item &item, items ) {
    Q_FOREACH( const QByteArray &flag, item.flags() ) {
      ++flagCounts[ flag ];
    }
  }

  QCOMPARE( flagCounts[ "\\SEEN" ], 2 );
  QCOMPARE( flagCounts[ "\\FLAGGED" ], 1 );
  QCOMPARE( flagCounts[ "$TODO" ], 1 );
  flagCounts.clear();

  // move from parent maildir to grandparent
  collection1_2.setParentCollection( collection2 );
  job = mStore->moveCollection( collection1_2, target );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  collection = job->collection();
  QCOMPARE( collection.remoteId(), collection1_2.remoteId() );
  QCOMPARE( collection.parentCollection(), target );

  fileInfo1_2.refresh();
  QVERIFY( !fileInfo1_2.exists() );
  fileInfo1_2 = QFileInfo( subDirTarget, collection.remoteId() );
  QVERIFY( fileInfo1_2.exists() );

  // check for index preservation
  var = job->property( "onDiskIndexInvalidated" );
  QVERIFY( var.isValid() );
  QCOMPARE( var.userType(), colListVar.userType() );

  collections = var.value<Collection::List>();
  QCOMPARE( (int)collections.count(), 1 );
  QCOMPARE( collections.first(), collection );

  // get the items and check the flags (see data/README)
  itemFetch = mStore->fetchItems( collection );
  QVERIFY( itemFetch->exec() );
  QCOMPARE( itemFetch->error(), 0 );

  items = itemFetch->items();
  QCOMPARE( (int)items.count(), 4 );
  Q_FOREACH( const Item &item, items ) {
    Q_FOREACH( const QByteArray &flag, item.flags() ) {
      ++flagCounts[ flag ];
    }
  }

  QCOMPARE( flagCounts[ "\\SEEN" ], 2 );
  QCOMPARE( flagCounts[ "\\FLAGGED" ], 1 );
  QCOMPARE( flagCounts[ "$TODO" ], 1 );
  flagCounts.clear();

  // move from parent mbox to grandparent
  Collection collection4_3;
  collection4_3.setName( QLatin1String( "collection4_3" ) );
  collection4_3.setRemoteId( QLatin1String( "collection4_3" ) );
  collection4_3.setParentCollection( collection4 );

  job = mStore->moveCollection( collection4_3, target );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  collection = job->collection();
  QCOMPARE( collection.remoteId(), collection4_3.remoteId() );
  QCOMPARE( collection.parentCollection(), target );

  QFileInfo fileInfo4_3( subDir4, collection.remoteId() );
  QVERIFY( !fileInfo4_3.exists() );
  fileInfo4_3 = QFileInfo( subDirTarget, collection.remoteId() );
  QVERIFY( fileInfo4_3.exists() );

  // check for index preservation
  var = job->property( "onDiskIndexInvalidated" );
  QVERIFY( var.isValid() );
  QCOMPARE( var.userType(), colListVar.userType() );

  collections = var.value<Collection::List>();
  QCOMPARE( (int)collections.count(), 1 );
  QCOMPARE( collections.first(), collection );

  // get the items and check the flags (see data/README)
  itemFetch = mStore->fetchItems( collection );
  QVERIFY( itemFetch->exec() );
  QCOMPARE( itemFetch->error(), 0 );

  items = itemFetch->items();
  QCOMPARE( (int)items.count(), 4 );
  Q_FOREACH( const Item &item, items ) {
    Q_FOREACH( const QByteArray &flag, item.flags() ) {
      ++flagCounts[ flag ];
    }
  }

  QCOMPARE( flagCounts[ "\\SEEN" ], 2 );
  QCOMPARE( flagCounts[ "\\FLAGGED" ], 1 );
  QCOMPARE( flagCounts[ "$TODO" ], 1 );
  flagCounts.clear();

  // move from parent mbox to grandparent
  Collection collection4_4;
  collection4_4.setName( QLatin1String( "collection4_4" ) );
  collection4_4.setRemoteId( QLatin1String( "collection4_4" ) );
  collection4_4.setParentCollection( collection4 );

  job = mStore->moveCollection( collection4_4, target );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  collection = job->collection();
  QCOMPARE( collection.remoteId(), collection4_4.remoteId() );
  QCOMPARE( collection.parentCollection(), target );

  QFileInfo fileInfo4_4( subDir4, collection.remoteId() );
  QVERIFY( !fileInfo4_4.exists() );
  fileInfo4_4 = QFileInfo( subDirTarget, collection.remoteId() );
  QVERIFY( fileInfo4_4.exists() );

  // check for index preservation
  var = job->property( "onDiskIndexInvalidated" );
  QVERIFY( var.isValid() );
  QCOMPARE( var.userType(), colListVar.userType() );

  collections = var.value<Collection::List>();
  QCOMPARE( (int)collections.count(), 1 );
  QCOMPARE( collections.first(), collection );

  // get the items and check the flags (see data/README)
  itemFetch = mStore->fetchItems( collection );
  QVERIFY( itemFetch->exec() );
  QCOMPARE( itemFetch->error(), 0 );

  items = itemFetch->items();
  QCOMPARE( (int)items.count(), 4 );
  Q_FOREACH( const Item &item, items ) {
    Q_FOREACH( const QByteArray &flag, item.flags() ) {
      ++flagCounts[ flag ];
    }
  }

  QCOMPARE( flagCounts[ "\\SEEN" ], 2 );
  QCOMPARE( flagCounts[ "\\FLAGGED" ], 1 );
  QCOMPARE( flagCounts[ "$TODO" ], 1 );
  flagCounts.clear();

  // move from maildir to grandchild
  collection1_1.setParentCollection( target );

  job = mStore->moveCollection( collection1_1, collection2 );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  collection = job->collection();
  QCOMPARE( collection.remoteId(), collection1_1.remoteId() );
  QCOMPARE( collection.parentCollection(), collection2 );

  fileInfo1_1.refresh();
  QVERIFY( !fileInfo1_1.exists() );
  fileInfo1_1 = QFileInfo( subDir2, collection.remoteId() );
  QVERIFY( fileInfo1_1.exists() );

  // check for index preservation
  var = job->property( "onDiskIndexInvalidated" );
  QVERIFY( var.isValid() );
  QCOMPARE( var.userType(), colListVar.userType() );

  collections = var.value<Collection::List>();
  QCOMPARE( (int)collections.count(), 1 );
  QCOMPARE( collections.first(), collection );

  // get the items and check the flags (see data/README)
  itemFetch = mStore->fetchItems( collection );
  QVERIFY( itemFetch->exec() );
  QCOMPARE( itemFetch->error(), 0 );

  items = itemFetch->items();
  QCOMPARE( (int)items.count(), 4 );
  Q_FOREACH( const Item &item, items ) {
    Q_FOREACH( const QByteArray &flag, item.flags() ) {
      ++flagCounts[ flag ];
    }
  }

  QCOMPARE( flagCounts[ "\\SEEN" ], 2 );
  QCOMPARE( flagCounts[ "\\FLAGGED" ], 1 );
  QCOMPARE( flagCounts[ "$TODO" ], 1 );
  flagCounts.clear();

  // move from maildir to grandchild
  collection1_2.setParentCollection( target );

  job = mStore->moveCollection( collection1_2, collection2 );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  collection = job->collection();
  QCOMPARE( collection.remoteId(), collection1_2.remoteId() );
  QCOMPARE( collection.parentCollection(), collection2 );

  fileInfo1_2.refresh();
  QVERIFY( !fileInfo1_2.exists() );
  fileInfo1_2 = QFileInfo( subDir2, collection.remoteId() );
  QVERIFY( fileInfo1_2.exists() );

  // check for index preservation
  var = job->property( "onDiskIndexInvalidated" );
  QVERIFY( var.isValid() );
  QCOMPARE( var.userType(), colListVar.userType() );

  collections = var.value<Collection::List>();
  QCOMPARE( (int)collections.count(), 1 );
  QCOMPARE( collections.first(), collection );

  // get the items and check the flags (see data/README)
  itemFetch = mStore->fetchItems( collection );
  QVERIFY( itemFetch->exec() );
  QCOMPARE( itemFetch->error(), 0 );

  items = itemFetch->items();
  QCOMPARE( (int)items.count(), 4 );
  Q_FOREACH( const Item &item, items ) {
    Q_FOREACH( const QByteArray &flag, item.flags() ) {
      ++flagCounts[ flag ];
    }
  }

  QCOMPARE( flagCounts[ "\\SEEN" ], 2 );
  QCOMPARE( flagCounts[ "\\FLAGGED" ], 1 );
  QCOMPARE( flagCounts[ "$TODO" ], 1 );
  flagCounts.clear();
}

void CollectionMoveTest::testMoveToMBox()
{
  QDir topDir( mDir->name() );

  // top level dir
  QVERIFY( TestDataUtil::installFolder( QLatin1String( "maildir" ), topDir.path(), QLatin1String( "collection1" ) ) );
  QFileInfo fileInfo1( topDir, QLatin1String( "collection1" ) );
  QVERIFY( TestDataUtil::installFolder( QLatin1String( "maildir" ), topDir.path(), QLatin1String( "collection2" ) ) );
  QFileInfo fileInfo2( topDir, QLatin1String( "collection2" ) );
  QVERIFY( TestDataUtil::installFolder( QLatin1String( "mbox" ), topDir.path(), QLatin1String( "collection3" ) ) );
  QFileInfo fileInfo3( topDir, QLatin1String( "collection3" ) );
  QVERIFY( TestDataUtil::installFolder( QLatin1String( "mbox" ), topDir.path(), QLatin1String( "collection4" ) ) );
  QFileInfo fileInfo4( topDir, QLatin1String( "collection4" ) );
  QVERIFY( TestDataUtil::installFolder( QLatin1String( "mbox" ), topDir.path(), QLatin1String( "collection5" ) ) );
  QFileInfo fileInfo5( topDir, QLatin1String( "collection5" ) );
  QVERIFY( TestDataUtil::installFolder( QLatin1String( "mbox" ), topDir.path(), QLatin1String( "collection6" ) ) );
  QFileInfo fileInfo6( topDir, QLatin1String( "collection6" ) );

  // first level maildir parent
  QDir subDir1 = topDir;
  QVERIFY( subDir1.mkdir( QLatin1String( ".collection1.directory" ) ) );
  QVERIFY( subDir1.cd( QLatin1String( ".collection1.directory" ) ) );
  QVERIFY( TestDataUtil::installFolder( QLatin1String( "maildir" ), subDir1.path(), QLatin1String( "collection1_1" ) ) );
  QVERIFY( TestDataUtil::installFolder( QLatin1String( "mbox" ), subDir1.path(), QLatin1String( "collection1_2" ) ) );
  QVERIFY( TestDataUtil::installFolder( QLatin1String( "maildir" ), subDir1.path(), QLatin1String( "collection1_3" ) ) );
  QVERIFY( TestDataUtil::installFolder( QLatin1String( "mbox" ), subDir1.path(), QLatin1String( "collection1_4" ) ) );

  // first level mbox parent
  QDir subDir4 = topDir;
  QVERIFY( subDir4.mkdir( QLatin1String( ".collection4.directory" ) ) );
  QVERIFY( subDir4.cd( QLatin1String( ".collection4.directory" ) ) );
  QVERIFY( TestDataUtil::installFolder( QLatin1String( "mbox" ), subDir4.path(), QLatin1String( "collection4_1" ) ) );
  QVERIFY( TestDataUtil::installFolder( QLatin1String( "maildir" ), subDir4.path(), QLatin1String( "collection4_2" ) ) );

  // target mbox
  QFileInfo fileInfoTarget( topDir.path(), QLatin1String( "target" ) );
  QFile fileTarget( fileInfoTarget.absoluteFilePath() );
  QVERIFY( fileTarget.open( QIODevice::WriteOnly ) );
  fileTarget.close();
  QVERIFY( fileInfoTarget.exists() );

  QDir subDirTarget;

  mStore->setPath( topDir.path() );

  // common variables
  FileStore::CollectionMoveJob *job = 0;
  FileStore::ItemFetchJob *itemFetch = 0;
  Collection collection;
  const QVariant colListVar = QVariant::fromValue<Collection::List>( Collection::List() );
  QVariant var;
  Collection::List collections;
  Item::List items;
  QMap<QByteArray, int> flagCounts;

  Collection target;
  target.setName( QLatin1String( "target" ) );
  target.setRemoteId( QLatin1String( "target" ) );
  target.setParentCollection( mStore->topLevelCollection() );

  // test move leaf maildir into sibling
  Collection collection2;
  collection2.setName( QLatin1String( "collection2" ) );
  collection2.setRemoteId( QLatin1String( "collection2" ) );
  collection2.setParentCollection( mStore->topLevelCollection() );

  job = mStore->moveCollection( collection2, target );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  subDirTarget = topDir;
  QVERIFY( subDirTarget.cd( QLatin1String( ".target.directory" ) ) );

  collection = job->collection();
  QCOMPARE( collection.remoteId(), collection2.remoteId() );
  QCOMPARE( collection.parentCollection(), target );

  fileInfo2.refresh();
  QVERIFY( !fileInfo2.exists() );
  fileInfo2 = QFileInfo( subDirTarget, collection.remoteId() );
  QVERIFY( fileInfo2.exists() );

  // check for index preservation
  var = job->property( "onDiskIndexInvalidated" );
  QVERIFY( var.isValid() );
  QCOMPARE( var.userType(), colListVar.userType() );

  collections = var.value<Collection::List>();
  QCOMPARE( (int)collections.count(), 1 );
  QCOMPARE( collections.first(), collection );

  // get the items and check the flags (see data/README)
  itemFetch = mStore->fetchItems( collection );
  QVERIFY( itemFetch->exec() );
  QCOMPARE( itemFetch->error(), 0 );

  items = itemFetch->items();
  QCOMPARE( (int)items.count(), 4 );
  Q_FOREACH( const Item &item, items ) {
    Q_FOREACH( const QByteArray &flag, item.flags() ) {
      ++flagCounts[ flag ];
    }
  }

  QCOMPARE( flagCounts[ "\\SEEN" ], 2 );
  QCOMPARE( flagCounts[ "\\FLAGGED" ], 1 );
  QCOMPARE( flagCounts[ "$TODO" ], 1 );
  flagCounts.clear();

  // test move leaf mbox into sibling
  Collection collection3;
  collection3.setName( QLatin1String( "collection3" ) );
  collection3.setRemoteId( QLatin1String( "collection3" ) );
  collection3.setParentCollection( mStore->topLevelCollection() );

  job = mStore->moveCollection( collection3, target );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  collection = job->collection();
  QCOMPARE( collection.remoteId(), collection3.remoteId() );
  QCOMPARE( collection.parentCollection(), target );

  fileInfo3.refresh();
  QVERIFY( !fileInfo3.exists() );
  fileInfo3 = QFileInfo( subDirTarget, collection.remoteId() );
  QVERIFY( fileInfo3.exists() );

  // check for index preservation
  var = job->property( "onDiskIndexInvalidated" );
  QVERIFY( var.isValid() );
  QCOMPARE( var.userType(), colListVar.userType() );

  collections = var.value<Collection::List>();
  QCOMPARE( (int)collections.count(), 1 );
  QCOMPARE( collections.first(), collection );

  // get the items and check the flags (see data/README)
  itemFetch = mStore->fetchItems( collection );
  QVERIFY( itemFetch->exec() );
  QCOMPARE( itemFetch->error(), 0 );

  items = itemFetch->items();
  QCOMPARE( (int)items.count(), 4 );
  Q_FOREACH( const Item &item, items ) {
    Q_FOREACH( const QByteArray &flag, item.flags() ) {
      ++flagCounts[ flag ];
    }
  }

  QCOMPARE( flagCounts[ "\\SEEN" ], 2 );
  QCOMPARE( flagCounts[ "\\FLAGGED" ], 1 );
  QCOMPARE( flagCounts[ "$TODO" ], 1 );
  flagCounts.clear();

  // test move leaf mbox into sibling without subtree
  Collection collection5;
  collection5.setName( QLatin1String( "collection5" ) );
  collection5.setRemoteId( QLatin1String( "collection5" ) );
  collection5.setParentCollection( mStore->topLevelCollection() );

  Collection collection6;
  collection6.setName( QLatin1String( "collection6" ) );
  collection6.setRemoteId( QLatin1String( "collection6" ) );
  collection6.setParentCollection( mStore->topLevelCollection() );

  job = mStore->moveCollection( collection5, collection6 );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  collection = job->collection();
  QCOMPARE( collection.remoteId(), collection5.remoteId() );
  QCOMPARE( collection.parentCollection(), collection6 );

  fileInfo5.refresh();
  QVERIFY( !fileInfo5.exists() );
  QDir subDir6 = topDir;
  QVERIFY( subDir6.cd( QLatin1String( ".collection6.directory" ) ) );
  fileInfo5 = QFileInfo( subDir6, collection.remoteId() );
  QVERIFY( fileInfo5.exists() );

  // check for index preservation
  var = job->property( "onDiskIndexInvalidated" );
  QVERIFY( var.isValid() );
  QCOMPARE( var.userType(), colListVar.userType() );

  collections = var.value<Collection::List>();
  QCOMPARE( (int)collections.count(), 1 );
  QCOMPARE( collections.first(), collection );

  // get the items and check the flags (see data/README)
  itemFetch = mStore->fetchItems( collection );
  QVERIFY( itemFetch->exec() );
  QCOMPARE( itemFetch->error(), 0 );

  items = itemFetch->items();
  QCOMPARE( (int)items.count(), 4 );
  Q_FOREACH( const Item &item, items ) {
    Q_FOREACH( const QByteArray &flag, item.flags() ) {
      ++flagCounts[ flag ];
    }
  }

  QCOMPARE( flagCounts[ "\\SEEN" ], 2 );
  QCOMPARE( flagCounts[ "\\FLAGGED" ], 1 );
  QCOMPARE( flagCounts[ "$TODO" ], 1 );
  flagCounts.clear();

  // test move maildir with subtree into sibling
  Collection collection1;
  collection1.setName( QLatin1String( "collection1" ) );
  collection1.setRemoteId( QLatin1String( "collection1" ) );
  collection1.setParentCollection( mStore->topLevelCollection() );

  // load sub collection index data to check for correct cache updates
  Collection collection1_1;
  collection1_1.setName( QLatin1String( "collection1_1" ) );
  collection1_1.setRemoteId( QLatin1String( "collection1_1" ) );
  collection1_1.setParentCollection( collection1 );
  itemFetch = mStore->fetchItems( collection1_1 );
  QVERIFY( itemFetch->exec() );

  Collection collection1_2;
  collection1_2.setName( QLatin1String( "collection1_2" ) );
  collection1_2.setRemoteId( QLatin1String( "collection1_2" ) );
  collection1_2.setParentCollection( collection1 );
  itemFetch = mStore->fetchItems( collection1_2 );
  QVERIFY( itemFetch->exec() );

  job = mStore->moveCollection( collection1, target );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  collection = job->collection();
  QCOMPARE( collection.remoteId(), collection1.remoteId() );
  QCOMPARE( collection.parentCollection(), target );

  fileInfo1.refresh();
  QVERIFY( !fileInfo1.exists() );
  fileInfo1 = QFileInfo( subDirTarget, collection.remoteId() );
  QVERIFY( fileInfo1.exists() );
  QVERIFY( !subDir1.exists() );
  subDir1 = subDirTarget;
  QVERIFY( subDir1.cd( QLatin1String( ".collection1.directory" ) ) );
  QCOMPARE( subDir1.entryList( QStringList() << QLatin1String( "collection*" ) ),
            QStringList() << QLatin1String( "collection1_1" ) << QLatin1String( "collection1_2" )
                          << QLatin1String( "collection1_3" ) << QLatin1String( "collection1_4" )
          );

  // check for index preservation
  var = job->property( "onDiskIndexInvalidated" );
  QVERIFY( var.isValid() );
  QCOMPARE( var.userType(), colListVar.userType() );

  collections = var.value<Collection::List>();
  QCOMPARE( (int)collections.count(), 1 );
  QCOMPARE( collections.first(), collection );

  // get the items and check the flags (see data/README)
  itemFetch = mStore->fetchItems( collection );
  QVERIFY( itemFetch->exec() );
  QCOMPARE( itemFetch->error(), 0 );

  items = itemFetch->items();
  QCOMPARE( (int)items.count(), 4 );
  Q_FOREACH( const Item &item, items ) {
    Q_FOREACH( const QByteArray &flag, item.flags() ) {
      ++flagCounts[ flag ];
    }
  }

  QCOMPARE( flagCounts[ "\\SEEN" ], 2 );
  QCOMPARE( flagCounts[ "\\FLAGGED" ], 1 );
  QCOMPARE( flagCounts[ "$TODO" ], 1 );
  flagCounts.clear();

  // check for children cache path updates
  collection1.setParentCollection( target );
  collection1_1.setParentCollection( collection1 );
  collection1_2.setParentCollection( collection1 );

  itemFetch = mStore->fetchItems( collection1_1 );
  QVERIFY( itemFetch->exec() );
  QCOMPARE( itemFetch->error(), 0 );

  items = itemFetch->items();
  QCOMPARE( (int)items.count(), 4 );
  Q_FOREACH( const Item &item, items ) {
    Q_FOREACH( const QByteArray &flag, item.flags() ) {
      ++flagCounts[ flag ];
    }
  }

  QCOMPARE( flagCounts[ "\\SEEN" ], 2 );
  QCOMPARE( flagCounts[ "\\FLAGGED" ], 1 );
  QCOMPARE( flagCounts[ "$TODO" ], 1 );
  flagCounts.clear();

  itemFetch = mStore->fetchItems( collection1_2 );
  QVERIFY( itemFetch->exec() );
  QCOMPARE( itemFetch->error(), 0 );

  items = itemFetch->items();
  QCOMPARE( (int)items.count(), 4 );
  Q_FOREACH( const Item &item, items ) {
    Q_FOREACH( const QByteArray &flag, item.flags() ) {
      ++flagCounts[ flag ];
    }
  }

  QCOMPARE( flagCounts[ "\\SEEN" ], 2 );
  QCOMPARE( flagCounts[ "\\FLAGGED" ], 1 );
  QCOMPARE( flagCounts[ "$TODO" ], 1 );
  flagCounts.clear();

  // test move mbox with subtree into sibling
  Collection collection4;
  collection4.setName( QLatin1String( "collection4" ) );
  collection4.setRemoteId( QLatin1String( "collection4" ) );
  collection4.setParentCollection( mStore->topLevelCollection() );

  // load sub collection index data to check for correct cache updates
  Collection collection4_1;
  collection4_1.setName( QLatin1String( "collection4_1" ) );
  collection4_1.setRemoteId( QLatin1String( "collection4_1" ) );
  collection4_1.setParentCollection( collection4 );
  itemFetch = mStore->fetchItems( collection4_1 );
  QVERIFY( itemFetch->exec() );

  Collection collection4_2;
  collection4_2.setName( QLatin1String( "collection4_2" ) );
  collection4_2.setRemoteId( QLatin1String( "collection4_2" ) );
  collection4_2.setParentCollection( collection4 );
  itemFetch = mStore->fetchItems( collection4_2 );
  QVERIFY( itemFetch->exec() );

  job = mStore->moveCollection( collection4, target );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  collection = job->collection();
  QCOMPARE( collection.remoteId(), collection4.remoteId() );
  QCOMPARE( collection.parentCollection(), target );

  fileInfo4.refresh();
  QVERIFY( !fileInfo4.exists() );
  fileInfo4 = QFileInfo( subDirTarget, collection.remoteId() );
  QVERIFY( fileInfo4.exists() );
  QVERIFY( !subDir4.exists() );
  subDir4 = subDirTarget;
  QVERIFY( subDir4.cd( QLatin1String( ".collection4.directory" ) ) );
  QCOMPARE( subDir4.entryList( QStringList() << QLatin1String( "collection*" ) ),
            QStringList() << QLatin1String( "collection4_1" ) << QLatin1String( "collection4_2" )
          );

  // check for index preservation
  var = job->property( "onDiskIndexInvalidated" );
  QVERIFY( var.isValid() );
  QCOMPARE( var.userType(), colListVar.userType() );

  collections = var.value<Collection::List>();
  QCOMPARE( (int)collections.count(), 1 );
  QCOMPARE( collections.first(), collection );

  // get the items and check the flags (see data/README)
  itemFetch = mStore->fetchItems( collection );
  QVERIFY( itemFetch->exec() );
  QCOMPARE( itemFetch->error(), 0 );

  items = itemFetch->items();
  QCOMPARE( (int)items.count(), 4 );
  Q_FOREACH( const Item &item, items ) {
    Q_FOREACH( const QByteArray &flag, item.flags() ) {
      ++flagCounts[ flag ];
    }
  }

  QCOMPARE( flagCounts[ "\\SEEN" ], 2 );
  QCOMPARE( flagCounts[ "\\FLAGGED" ], 1 );
  QCOMPARE( flagCounts[ "$TODO" ], 1 );
  flagCounts.clear();

  // check for children cache path updates
  collection4.setParentCollection( target );
  collection4_1.setParentCollection( collection4 );
  collection4_2.setParentCollection( collection4 );

  itemFetch = mStore->fetchItems( collection4_1 );
  QVERIFY( itemFetch->exec() );
  QCOMPARE( itemFetch->error(), 0 );

  items = itemFetch->items();
  QCOMPARE( (int)items.count(), 4 );
  Q_FOREACH( const Item &item, items ) {
    Q_FOREACH( const QByteArray &flag, item.flags() ) {
      ++flagCounts[ flag ];
    }
  }

  QCOMPARE( flagCounts[ "\\SEEN" ], 2 );
  QCOMPARE( flagCounts[ "\\FLAGGED" ], 1 );
  QCOMPARE( flagCounts[ "$TODO" ], 1 );
  flagCounts.clear();

  itemFetch = mStore->fetchItems( collection4_2 );
  QVERIFY( itemFetch->exec() );
  QCOMPARE( itemFetch->error(), 0 );

  items = itemFetch->items();
  QCOMPARE( (int)items.count(), 4 );
  Q_FOREACH( const Item &item, items ) {
    Q_FOREACH( const QByteArray &flag, item.flags() ) {
      ++flagCounts[ flag ];
    }
  }

  QCOMPARE( flagCounts[ "\\SEEN" ], 2 );
  QCOMPARE( flagCounts[ "\\FLAGGED" ], 1 );
  QCOMPARE( flagCounts[ "$TODO" ], 1 );
  flagCounts.clear();

  // move from parent maildir to parent's sibling
  collection3.setParentCollection( target );

  job = mStore->moveCollection( collection1_1, collection3 );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  QDir subDir3 = subDirTarget;
  QVERIFY( subDir3.cd( QLatin1String( ".collection3.directory" ) ) );

  collection = job->collection();
  QCOMPARE( collection.remoteId(), collection1_1.remoteId() );
  QCOMPARE( collection.parentCollection(), collection3 );

  QFileInfo fileInfo1_1( subDir1, collection.remoteId() );
  QVERIFY( !fileInfo1_1.exists() );
  fileInfo1_1 = QFileInfo( subDir3, collection.remoteId() );
  QVERIFY( fileInfo1_1.exists() );

  // check for index preservation
  var = job->property( "onDiskIndexInvalidated" );
  QVERIFY( var.isValid() );
  QCOMPARE( var.userType(), colListVar.userType() );

  collections = var.value<Collection::List>();
  QCOMPARE( (int)collections.count(), 1 );
  QCOMPARE( collections.first(), collection );

  // get the items and check the flags (see data/README)
  itemFetch = mStore->fetchItems( collection );
  QVERIFY( itemFetch->exec() );
  QCOMPARE( itemFetch->error(), 0 );

  items = itemFetch->items();
  QCOMPARE( (int)items.count(), 4 );
  Q_FOREACH( const Item &item, items ) {
    Q_FOREACH( const QByteArray &flag, item.flags() ) {
      ++flagCounts[ flag ];
    }
  }

  QCOMPARE( flagCounts[ "\\SEEN" ], 2 );
  QCOMPARE( flagCounts[ "\\FLAGGED" ], 1 );
  QCOMPARE( flagCounts[ "$TODO" ], 1 );
  flagCounts.clear();

  // move from parent maildir to parent's sibling
  job = mStore->moveCollection( collection1_2, collection3 );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  collection = job->collection();
  QCOMPARE( collection.remoteId(), collection1_2.remoteId() );
  QCOMPARE( collection.parentCollection(), collection3 );

  QFileInfo fileInfo1_2( subDir1, collection.remoteId() );
  QVERIFY( !fileInfo1_2.exists() );
  fileInfo1_2 = QFileInfo( subDir3, collection.remoteId() );
  QVERIFY( fileInfo1_2.exists() );

  // check for index preservation
  var = job->property( "onDiskIndexInvalidated" );
  QVERIFY( var.isValid() );
  QCOMPARE( var.userType(), colListVar.userType() );

  collections = var.value<Collection::List>();
  QCOMPARE( (int)collections.count(), 1 );
  QCOMPARE( collections.first(), collection );

  // get the items and check the flags (see data/README)
  itemFetch = mStore->fetchItems( collection );
  QVERIFY( itemFetch->exec() );
  QCOMPARE( itemFetch->error(), 0 );

  items = itemFetch->items();
  QCOMPARE( (int)items.count(), 4 );
  Q_FOREACH( const Item &item, items ) {
    Q_FOREACH( const QByteArray &flag, item.flags() ) {
      ++flagCounts[ flag ];
    }
  }

  QCOMPARE( flagCounts[ "\\SEEN" ], 2 );
  QCOMPARE( flagCounts[ "\\FLAGGED" ], 1 );
  QCOMPARE( flagCounts[ "$TODO" ], 1 );
  flagCounts.clear();

  // move from parent mbox to parent's sibling
  job = mStore->moveCollection( collection4_1, collection3 );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  collection = job->collection();
  QCOMPARE( collection.remoteId(), collection4_1.remoteId() );
  QCOMPARE( collection.parentCollection(), collection3 );

  QFileInfo fileInfo4_1( subDir4, collection.remoteId() );
  QVERIFY( !fileInfo4_1.exists() );
  fileInfo4_1 = QFileInfo( subDir3, collection.remoteId() );
  QVERIFY( fileInfo4_1.exists() );

  // check for index preservation
  var = job->property( "onDiskIndexInvalidated" );
  QVERIFY( var.isValid() );
  QCOMPARE( var.userType(), colListVar.userType() );

  collections = var.value<Collection::List>();
  QCOMPARE( (int)collections.count(), 1 );
  QCOMPARE( collections.first(), collection );

  // get the items and check the flags (see data/README)
  itemFetch = mStore->fetchItems( collection );
  QVERIFY( itemFetch->exec() );
  QCOMPARE( itemFetch->error(), 0 );

  items = itemFetch->items();
  QCOMPARE( (int)items.count(), 4 );
  Q_FOREACH( const Item &item, items ) {
    Q_FOREACH( const QByteArray &flag, item.flags() ) {
      ++flagCounts[ flag ];
    }
  }

  QCOMPARE( flagCounts[ "\\SEEN" ], 2 );
  QCOMPARE( flagCounts[ "\\FLAGGED" ], 1 );
  QCOMPARE( flagCounts[ "$TODO" ], 1 );
  flagCounts.clear();

  // move from parent mbox to parent's sibling
  job = mStore->moveCollection( collection4_2, collection3 );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  collection = job->collection();
  QCOMPARE( collection.remoteId(), collection4_2.remoteId() );
  QCOMPARE( collection.parentCollection(), collection3 );

  QFileInfo fileInfo4_2( subDir4, collection.remoteId() );
  QVERIFY( !fileInfo4_2.exists() );
  fileInfo4_2 = QFileInfo( subDir3, collection.remoteId() );
  QVERIFY( fileInfo4_2.exists() );

  // check for index preservation
  var = job->property( "onDiskIndexInvalidated" );
  QVERIFY( var.isValid() );
  QCOMPARE( var.userType(), colListVar.userType() );

  collections = var.value<Collection::List>();
  QCOMPARE( (int)collections.count(), 1 );
  QCOMPARE( collections.first(), collection );

  // get the items and check the flags (see data/README)
  itemFetch = mStore->fetchItems( collection );
  QVERIFY( itemFetch->exec() );
  QCOMPARE( itemFetch->error(), 0 );

  items = itemFetch->items();
  QCOMPARE( (int)items.count(), 4 );
  Q_FOREACH( const Item &item, items ) {
    Q_FOREACH( const QByteArray &flag, item.flags() ) {
      ++flagCounts[ flag ];
    }
  }

  QCOMPARE( flagCounts[ "\\SEEN" ], 2 );
  QCOMPARE( flagCounts[ "\\FLAGGED" ], 1 );
  QCOMPARE( flagCounts[ "$TODO" ], 1 );
  flagCounts.clear();

  // move from parent mbox to grandparent
  collection1_1.setParentCollection( collection3 );

  job = mStore->moveCollection( collection1_1, target );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  collection = job->collection();
  QCOMPARE( collection.remoteId(), collection1_1.remoteId() );
  QCOMPARE( collection.parentCollection(), target );

  fileInfo1_1.refresh();
  QVERIFY( !fileInfo1_1.exists() );
  fileInfo1_1 = QFileInfo( subDirTarget, collection.remoteId() );
  QVERIFY( fileInfo1_1.exists() );

  // check for index preservation
  var = job->property( "onDiskIndexInvalidated" );
  QVERIFY( var.isValid() );
  QCOMPARE( var.userType(), colListVar.userType() );

  collections = var.value<Collection::List>();
  QCOMPARE( (int)collections.count(), 1 );
  QCOMPARE( collections.first(), collection );

  // get the items and check the flags (see data/README)
  itemFetch = mStore->fetchItems( collection );
  QVERIFY( itemFetch->exec() );
  QCOMPARE( itemFetch->error(), 0 );

  items = itemFetch->items();
  QCOMPARE( (int)items.count(), 4 );
  Q_FOREACH( const Item &item, items ) {
    Q_FOREACH( const QByteArray &flag, item.flags() ) {
      ++flagCounts[ flag ];
    }
  }

  QCOMPARE( flagCounts[ "\\SEEN" ], 2 );
  QCOMPARE( flagCounts[ "\\FLAGGED" ], 1 );
  QCOMPARE( flagCounts[ "$TODO" ], 1 );
  flagCounts.clear();

  // move from parent mbox to grandparent
  collection1_2.setParentCollection( collection3 );
  job = mStore->moveCollection( collection1_2, target );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  collection = job->collection();
  QCOMPARE( collection.remoteId(), collection1_2.remoteId() );
  QCOMPARE( collection.parentCollection(), target );

  fileInfo1_2.refresh();
  QVERIFY( !fileInfo1_2.exists() );
  fileInfo1_2 = QFileInfo( subDirTarget, collection.remoteId() );
  QVERIFY( fileInfo1_2.exists() );

  // check for index preservation
  var = job->property( "onDiskIndexInvalidated" );
  QVERIFY( var.isValid() );
  QCOMPARE( var.userType(), colListVar.userType() );

  collections = var.value<Collection::List>();
  QCOMPARE( (int)collections.count(), 1 );
  QCOMPARE( collections.first(), collection );

  // get the items and check the flags (see data/README)
  itemFetch = mStore->fetchItems( collection );
  QVERIFY( itemFetch->exec() );
  QCOMPARE( itemFetch->error(), 0 );

  items = itemFetch->items();
  QCOMPARE( (int)items.count(), 4 );
  Q_FOREACH( const Item &item, items ) {
    Q_FOREACH( const QByteArray &flag, item.flags() ) {
      ++flagCounts[ flag ];
    }
  }

  QCOMPARE( flagCounts[ "\\SEEN" ], 2 );
  QCOMPARE( flagCounts[ "\\FLAGGED" ], 1 );
  QCOMPARE( flagCounts[ "$TODO" ], 1 );
  flagCounts.clear();

  // move from parent maildir to grandparent
  Collection collection1_3;
  collection1_3.setName( QLatin1String( "collection1_3" ) );
  collection1_3.setRemoteId( QLatin1String( "collection1_3" ) );
  collection1_3.setParentCollection( collection1 );

  job = mStore->moveCollection( collection1_3, target );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  collection = job->collection();
  QCOMPARE( collection.remoteId(), collection1_3.remoteId() );
  QCOMPARE( collection.parentCollection(), target );

  QFileInfo fileInfo1_3( subDir1, collection.remoteId() );
  QVERIFY( !fileInfo1_3.exists() );
  fileInfo1_3 = QFileInfo( subDirTarget, collection.remoteId() );
  QVERIFY( fileInfo1_3.exists() );

  // check for index preservation
  var = job->property( "onDiskIndexInvalidated" );
  QVERIFY( var.isValid() );
  QCOMPARE( var.userType(), colListVar.userType() );

  collections = var.value<Collection::List>();
  QCOMPARE( (int)collections.count(), 1 );
  QCOMPARE( collections.first(), collection );

  // get the items and check the flags (see data/README)
  itemFetch = mStore->fetchItems( collection );
  QVERIFY( itemFetch->exec() );
  QCOMPARE( itemFetch->error(), 0 );

  items = itemFetch->items();
  QCOMPARE( (int)items.count(), 4 );
  Q_FOREACH( const Item &item, items ) {
    Q_FOREACH( const QByteArray &flag, item.flags() ) {
      ++flagCounts[ flag ];
    }
  }

  QCOMPARE( flagCounts[ "\\SEEN" ], 2 );
  QCOMPARE( flagCounts[ "\\FLAGGED" ], 1 );
  QCOMPARE( flagCounts[ "$TODO" ], 1 );
  flagCounts.clear();

  // move from parent maildir to grandparent
  Collection collection1_4;
  collection1_4.setName( QLatin1String( "collection1_4" ) );
  collection1_4.setRemoteId( QLatin1String( "collection1_4" ) );
  collection1_4.setParentCollection( collection1 );

  job = mStore->moveCollection( collection1_4, target );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  collection = job->collection();
  QCOMPARE( collection.remoteId(), collection1_4.remoteId() );
  QCOMPARE( collection.parentCollection(), target );

  QFileInfo fileInfo1_4( subDir1, collection.remoteId() );
  QVERIFY( !fileInfo1_4.exists() );
  fileInfo1_4 = QFileInfo( subDirTarget, collection.remoteId() );
  QVERIFY( fileInfo1_4.exists() );

  // check for index preservation
  var = job->property( "onDiskIndexInvalidated" );
  QVERIFY( var.isValid() );
  QCOMPARE( var.userType(), colListVar.userType() );

  collections = var.value<Collection::List>();
  QCOMPARE( (int)collections.count(), 1 );
  QCOMPARE( collections.first(), collection );

  // get the items and check the flags (see data/README)
  itemFetch = mStore->fetchItems( collection );
  QVERIFY( itemFetch->exec() );
  QCOMPARE( itemFetch->error(), 0 );

  items = itemFetch->items();
  QCOMPARE( (int)items.count(), 4 );
  Q_FOREACH( const Item &item, items ) {
    Q_FOREACH( const QByteArray &flag, item.flags() ) {
      ++flagCounts[ flag ];
    }
  }

  QCOMPARE( flagCounts[ "\\SEEN" ], 2 );
  QCOMPARE( flagCounts[ "\\FLAGGED" ], 1 );
  QCOMPARE( flagCounts[ "$TODO" ], 1 );
  flagCounts.clear();

  // move from mbox to grandchild
  collection1_1.setParentCollection( target );

  job = mStore->moveCollection( collection1_1, collection3 );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  collection = job->collection();
  QCOMPARE( collection.remoteId(), collection1_1.remoteId() );
  QCOMPARE( collection.parentCollection(), collection3 );

  fileInfo1_1.refresh();
  QVERIFY( !fileInfo1_1.exists() );
  fileInfo1_1 = QFileInfo( subDir3, collection.remoteId() );
  QVERIFY( fileInfo1_1.exists() );

  // check for index preservation
  var = job->property( "onDiskIndexInvalidated" );
  QVERIFY( var.isValid() );
  QCOMPARE( var.userType(), colListVar.userType() );

  collections = var.value<Collection::List>();
  QCOMPARE( (int)collections.count(), 1 );
  QCOMPARE( collections.first(), collection );

  // get the items and check the flags (see data/README)
  itemFetch = mStore->fetchItems( collection );
  QVERIFY( itemFetch->exec() );
  QCOMPARE( itemFetch->error(), 0 );

  items = itemFetch->items();
  QCOMPARE( (int)items.count(), 4 );
  Q_FOREACH( const Item &item, items ) {
    Q_FOREACH( const QByteArray &flag, item.flags() ) {
      ++flagCounts[ flag ];
    }
  }

  QCOMPARE( flagCounts[ "\\SEEN" ], 2 );
  QCOMPARE( flagCounts[ "\\FLAGGED" ], 1 );
  QCOMPARE( flagCounts[ "$TODO" ], 1 );
  flagCounts.clear();

  // move from maildir to grandchild
  collection1_2.setParentCollection( target );

  job = mStore->moveCollection( collection1_2, collection3 );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  collection = job->collection();
  QCOMPARE( collection.remoteId(), collection1_2.remoteId() );
  QCOMPARE( collection.parentCollection(), collection3 );

  fileInfo1_2.refresh();
  QVERIFY( !fileInfo1_2.exists() );
  fileInfo1_2 = QFileInfo( subDir3, collection.remoteId() );
  QVERIFY( fileInfo1_2.exists() );

  // check for index preservation
  var = job->property( "onDiskIndexInvalidated" );
  QVERIFY( var.isValid() );
  QCOMPARE( var.userType(), colListVar.userType() );

  collections = var.value<Collection::List>();
  QCOMPARE( (int)collections.count(), 1 );
  QCOMPARE( collections.first(), collection );

  // get the items and check the flags (see data/README)
  itemFetch = mStore->fetchItems( collection );
  QVERIFY( itemFetch->exec() );
  QCOMPARE( itemFetch->error(), 0 );

  items = itemFetch->items();
  QCOMPARE( (int)items.count(), 4 );
  Q_FOREACH( const Item &item, items ) {
    Q_FOREACH( const QByteArray &flag, item.flags() ) {
      ++flagCounts[ flag ];
    }
  }

  QCOMPARE( flagCounts[ "\\SEEN" ], 2 );
  QCOMPARE( flagCounts[ "\\FLAGGED" ], 1 );
  QCOMPARE( flagCounts[ "$TODO" ], 1 );
  flagCounts.clear();
}

QTEST_KDEMAIN( CollectionMoveTest, NoGUI )

#include "collectionmovetest.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
