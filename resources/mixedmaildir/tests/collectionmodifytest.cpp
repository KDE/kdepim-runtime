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

#include "filestore/collectionmodifyjob.h"
#include "filestore/itemfetchjob.h"

#include "libmaildir/maildir.h"

#include <KTempDir>

#include <qtest_kde.h>

using namespace Akonadi;

class CollectionModifyTest : public QObject
{
  Q_OBJECT

  public:
    CollectionModifyTest() : QObject(), mStore( 0 ), mDir( 0 ) {}

    ~CollectionModifyTest() {
      delete mStore;
      delete mDir;
    }

  private:
    MixedMaildirStore *mStore;
    KTempDir *mDir;

  private Q_SLOTS:
    void init();
    void cleanup();
    void testRename();
    void testIndexPreservation();
};

void CollectionModifyTest::init()
{
  mStore = new MixedMaildirStore;

  mDir = new KTempDir;
  QVERIFY( mDir->exists() );
}

void CollectionModifyTest::cleanup()
{
  delete mStore;
  mStore = 0;
  delete mDir;
  mDir = 0;
}

void CollectionModifyTest::testRename()
{
  QDir topDir( mDir->name() );
  QVERIFY( topDir.mkdir( QLatin1String( "topLevel" ) ) );
  QVERIFY( topDir.cd( QLatin1String( "topLevel" ) ) );

  KPIM::Maildir topLevelMd( topDir.path(), true );
  QVERIFY( topLevelMd.isValid() );

  KPIM::Maildir md1( topLevelMd.addSubFolder( "collection1" ), false );
  KPIM::Maildir md1_2( md1.addSubFolder( "collection1_2" ), false );

  // simulate second level mbox in maildir parent
  QFileInfo fileInfo1_1( KPIM::Maildir::subDirPathForFolderPath( md1.path() ),
                         QLatin1String( "collection1_1" ));
  QFile file1_1( fileInfo1_1.absoluteFilePath() );
  file1_1.open( QIODevice::WriteOnly );
  file1_1.close();
  QVERIFY( fileInfo1_1.exists() );

  KPIM::Maildir md2( topLevelMd.addSubFolder( "collection2" ), false );

  // simulate first level mbox
  QFileInfo fileInfo3( topDir.path(), QLatin1String( "collection3" ));
  QFile file3( fileInfo3.absoluteFilePath() );
  file3.open( QIODevice::WriteOnly );
  file3.close();
  QVERIFY( fileInfo3.exists() );

  // simulate first level mbox with subtree
  QFileInfo fileInfo4( topDir.path(), QLatin1String( "collection4" ));
  QFile file4( fileInfo4.absoluteFilePath() );
  file4.open( QIODevice::WriteOnly );
  file4.close();
  QVERIFY( fileInfo4.exists() );

  QFileInfo subDirInfo4( KPIM::Maildir::subDirPathForFolderPath( fileInfo4.absoluteFilePath() ) );
  QVERIFY( topDir.mkpath( subDirInfo4.absoluteFilePath() ) );

  KPIM::Maildir md4( subDirInfo4.absoluteFilePath(), true );
  KPIM::Maildir md4_1( md4.addSubFolder( "collection4_1" ), false );

  // simulate second level mbox in mbox parent
  QFileInfo fileInfo4_2( subDirInfo4.absoluteFilePath(),
                         QLatin1String( "collection4_2" ));
  QFile file4_2( fileInfo4_2.absoluteFilePath() );
  file4_2.open( QIODevice::WriteOnly );
  file4_2.close();
  QVERIFY( fileInfo4_2.exists() );

  mStore->setPath( topDir.path() );

  FileStore::CollectionModifyJob *job = 0;
  Collection collection;

  // test renaming top level collection
  topDir.cdUp();
  QVERIFY( !topDir.exists( QLatin1String( "newTopLevel" ) ) );

  Collection topLevelCollection = mStore->topLevelCollection();
  topLevelCollection.setName( QLatin1String( "newTopLevel" ) );
  job = mStore->modifyCollection( topLevelCollection );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  QVERIFY( topDir.exists( QLatin1String( "newTopLevel" ) ) );
  QVERIFY( !topDir.exists( QLatin1String( "topLevel" ) ) );
  QVERIFY( topDir.cd( QLatin1String( "newTopLevel" ) ) );
  QCOMPARE( mStore->path(), topDir.path() );

  collection = job->collection();
  QCOMPARE( collection.remoteId(), mStore->path() );
  QCOMPARE( collection, mStore->topLevelCollection() );

  // test failure of renaming again
  job = mStore->modifyCollection( topLevelCollection );
  QVERIFY( !job->exec() );
  QCOMPARE( job->error(), (int) FileStore::Job::InvalidJobContext );
  QCOMPARE( collection.remoteId(), mStore->path() );
  QCOMPARE( collection, mStore->topLevelCollection() );

  // adjust local handles
  topLevelMd = KPIM::Maildir( topDir.path(), true );
  QVERIFY( topLevelMd.isValid() );

  md1 = topLevelMd.subFolder( "collection1" );
  QVERIFY( md1.isValid() );
  md1_2 = md1.subFolder( "collection1_2" );

  fileInfo1_1 = QFileInfo( KPIM::Maildir::subDirPathForFolderPath( md1.path() ),
                           QLatin1String( "collection1_1" ));
  QVERIFY( fileInfo1_1.exists() );

  md2 = topLevelMd.subFolder( "collection2" );

  fileInfo3 = QFileInfo( topDir.path(), QLatin1String( "collection3" ) );
  QVERIFY( fileInfo3.exists() );

  fileInfo4 = QFileInfo( topDir.path(), QLatin1String( "collection4" ) );
  QVERIFY( fileInfo4.exists() );

  subDirInfo4 = QFileInfo( KPIM::Maildir::subDirPathForFolderPath( fileInfo4.absoluteFilePath() ) );
  QVERIFY( subDirInfo4.exists() );

  md4 = KPIM::Maildir( subDirInfo4.absoluteFilePath(), true );
  QVERIFY( md4.isValid() );
  md4_1 = md4.subFolder( "collection4_1" );

  fileInfo4_2 = QFileInfo( subDirInfo4.absoluteFilePath(),
                           QLatin1String( "collection4_2" ) );
  QVERIFY( fileInfo4_2.exists() );

  QCOMPARE( topLevelMd.subFolderList(), QStringList() << QLatin1String( "collection1" ) << QLatin1String( "collection2" ) );

  // test rename first level maildir leaf
  Collection collection2;
  collection2.setRemoteId( QLatin1String( "collection2" ) );
  collection2.setParentCollection( mStore->topLevelCollection() );
  collection2.setName( QLatin1String( "collection2_renamed" ) );

  job = mStore->modifyCollection( collection2 );
  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  collection = job->collection();
  QCOMPARE( collection.remoteId(), collection.name() );
  QCOMPARE( collection, collection2 );
  QCOMPARE( topLevelMd.subFolderList(), QStringList() << QLatin1String( "collection1" ) << QLatin1String( "collection2_renamed" ) );
  QVERIFY( !md2.isValid() );
  md2 = topLevelMd.subFolder( collection.remoteId() );
  QVERIFY( md2.isValid() );

  // test failure of renaming again
  job = mStore->modifyCollection( collection2 );
  QVERIFY( !job->exec() );
  QCOMPARE( job->error(), (int) FileStore::Job::InvalidJobContext );
  QCOMPARE( topLevelMd.subFolderList(), QStringList() << QLatin1String( "collection1" ) << QLatin1String( "collection2_renamed" ) );
  QVERIFY( md2.isValid() );

  // test renaming of first level mbox leaf
  Collection collection3;
  collection3.setRemoteId( QLatin1String( "collection3" ) );
  collection3.setParentCollection( mStore->topLevelCollection() );
  collection3.setName( QLatin1String( "collection3_renamed" ) );

  job = mStore->modifyCollection( collection3 );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  collection = job->collection();
  QCOMPARE( collection.remoteId(), collection.name() );
  QCOMPARE( collection, collection3 );
  fileInfo3.refresh();
  QVERIFY( !fileInfo3.exists() );
  fileInfo3 = QFileInfo( topDir.path(), collection.remoteId() );
  QVERIFY( fileInfo3.exists() );

  // test failure of renaming again
  job = mStore->modifyCollection( collection3 );
  QVERIFY( !job->exec() );
  QCOMPARE( job->error(), (int) FileStore::Job::InvalidJobContext );
  fileInfo3.refresh();
  QVERIFY( fileInfo3.exists() );

  // test renaming second level maildir in mbox parent
  Collection collection4;
  collection4.setRemoteId( QLatin1String( "collection4" ) );
  collection4.setParentCollection( mStore->topLevelCollection() );
  collection4.setName( QLatin1String( "collection4" ) );

  Collection collection4_1;
  collection4_1.setRemoteId( QLatin1String( "collection4_1" ) );
  collection4_1.setParentCollection( collection4 );
  collection4_1.setName( QLatin1String( "collection4_1_renamed" ) );

  job = mStore->modifyCollection( collection4_1 );
  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  collection = job->collection();
  QCOMPARE( collection.remoteId(), collection.name() );
  QCOMPARE( collection, collection4_1 );
  QCOMPARE( md4.subFolderList(), QStringList() << QLatin1String( "collection4_1_renamed" ) );
  QVERIFY( !md4_1.isValid() );
  md4_1 = md4.subFolder( collection.remoteId() );
  QVERIFY( md4_1.isValid() );

  // test failure of renaming again
  job = mStore->modifyCollection( collection4_1 );
  QVERIFY( !job->exec() );
  QCOMPARE( job->error(), (int) FileStore::Job::InvalidJobContext );
  QCOMPARE( md4.subFolderList(), QStringList() << QLatin1String( "collection4_1_renamed" ) );
  QVERIFY( md4_1.isValid() );

  // test renaming of second level mbox in mbox parent
  Collection collection4_2;
  collection4_2.setRemoteId( QLatin1String( "collection4_2" ) );
  collection4_2.setParentCollection( collection4 );
  collection4_2.setName( QLatin1String( "collection4_2_renamed" ) );

  job = mStore->modifyCollection( collection4_2 );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  collection = job->collection();
  QCOMPARE( collection.remoteId(), collection.name() );
  QCOMPARE( collection, collection4_2 );
  fileInfo4_2.refresh();
  QVERIFY( !fileInfo4_2.exists() );
  fileInfo4_2 = QFileInfo( md4.path(), collection.remoteId() );
  QVERIFY( fileInfo4_2.exists() );

  // test failure of renaming again
  job = mStore->modifyCollection( collection4_2 );
  QVERIFY( !job->exec() );
  QCOMPARE( job->error(), (int) FileStore::Job::InvalidJobContext );
  fileInfo4_2.refresh();
  QVERIFY( fileInfo4_2.exists() );

  // test renaming of maildir with subtree
  Collection collection1;
  collection1.setRemoteId( QLatin1String( "collection1" ) );
  collection1.setParentCollection( mStore->topLevelCollection() );
  collection1.setName( QLatin1String( "collection1_renamed" ) );

  job = mStore->modifyCollection( collection1 );
  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  collection = job->collection();
  QCOMPARE( collection.remoteId(), collection.name() );
  QCOMPARE( collection, collection1 );
  QCOMPARE( topLevelMd.subFolderList(), QStringList() << QLatin1String( "collection1_renamed" ) << QLatin1String( "collection2_renamed" ) );
  QVERIFY( !md1.isValid() );
  md1 = topLevelMd.subFolder( collection.remoteId() );
  QVERIFY( md1.isValid() );
  fileInfo1_1.refresh();
  QVERIFY( !fileInfo1_1.exists() );
  QVERIFY( !md1_2.isValid() );
  fileInfo1_1 = QFileInfo( KPIM::Maildir::subDirPathForFolderPath( md1.path() ),
                           QLatin1String( "collection1_1" ));
  QVERIFY( fileInfo1_1.exists() );
  md1_2 = md1.subFolder( QLatin1String( "collection1_2" ) );
  QVERIFY( md1_2.isValid() );

  // test failure of renaming again
  job = mStore->modifyCollection( collection1 );
  QVERIFY( !job->exec() );
  QCOMPARE( job->error(), (int) FileStore::Job::InvalidJobContext );
  QCOMPARE( topLevelMd.subFolderList(), QStringList() << QLatin1String( "collection1_renamed" ) << QLatin1String( "collection2_renamed" ) );
  QVERIFY( md2.isValid() );
  QVERIFY( fileInfo1_1.exists() );
  QVERIFY( md1_2.isValid() );

  // test renaming of mbox with subtree
  collection4.setName( QLatin1String( "collection4_renamed" ) );
  job = mStore->modifyCollection( collection4 );
  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  collection = job->collection();
  QCOMPARE( collection.remoteId(), collection.name() );
  QCOMPARE( collection, collection4 );
  fileInfo4.refresh();
  QVERIFY( !fileInfo4.exists() );
  fileInfo4 = QFileInfo( topDir.path(), collection.remoteId() );
  QVERIFY( fileInfo4.exists() );
  md4 = KPIM::Maildir( KPIM::Maildir::subDirPathForFolderPath( fileInfo4.absoluteFilePath() ), true );
  QVERIFY( md4.isValid() );

  QVERIFY( !md4_1.isValid() );
  fileInfo4_2.refresh();
  QVERIFY( !fileInfo4_2.exists() );
  md4_1 = md4.subFolder( QLatin1String( "collection4_1_renamed" ) );
  QVERIFY( md4_1.isValid() );
  fileInfo4_2 = QFileInfo( md4.path(), QLatin1String( "collection4_2_renamed" ));
  QVERIFY( fileInfo4_2.exists() );

  // test failure of renaming again
  job = mStore->modifyCollection( collection4 );
  QVERIFY( !job->exec() );
  QCOMPARE( job->error(), (int) FileStore::Job::InvalidJobContext );
  fileInfo4.refresh();
  QVERIFY( fileInfo4.exists() );
}

void CollectionModifyTest::testIndexPreservation()
{
  QVERIFY( TestDataUtil::installFolder( QLatin1String( "mbox" ), mDir->name(), QLatin1String( "collection1" ) ) );
  QVERIFY( TestDataUtil::installFolder( QLatin1String( "maildir" ), mDir->name(), QLatin1String( "collection2" ) ) );

  mStore->setPath( mDir->name() );

  const QVariant colListVar = QVariant::fromValue<Collection::List>( Collection::List() );
  FileStore::CollectionModifyJob *job = 0;
  FileStore::ItemFetchJob *itemFetch = 0;
  QVariant var;
  Collection::List collections;
  Item::List items;

  QMap<QByteArray, int> flagCounts;

  // test renaming mbox
  Collection collection1;
  collection1.setRemoteId( QLatin1String( "collection1" ) );
  collection1.setParentCollection( mStore->topLevelCollection() );
  collection1.setName( QLatin1String( "collection1_renamed" ) );

  job = mStore->modifyCollection( collection1 );
  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  var = job->property( "onDiskIndexInvalidated" );
  QVERIFY( var.isValid() );
  QCOMPARE( var.userType(), colListVar.userType() );

  collections = var.value<Collection::List>();
  QCOMPARE( (int)collections.count(), 1 );
  QCOMPARE( collections.first(), collection1 );

  const QFileInfo indexFileInfo1( mDir->name(), QLatin1String( ".collection1_renamed.index" ) );
  QVERIFY( !indexFileInfo1.exists() );

  // get the items and check the flags (see data/README)
  itemFetch = mStore->fetchItems( collections.first() );
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

  // test renaming maildir
  Collection collection2;
  collection2.setRemoteId( QLatin1String( "collection2" ) );
  collection2.setParentCollection( mStore->topLevelCollection() );
  collection2.setName( QLatin1String( "collection2_renamed" ) );

  job = mStore->modifyCollection( collection2 );
  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  var = job->property( "onDiskIndexInvalidated" );
  QVERIFY( var.isValid() );
  QCOMPARE( var.userType(), colListVar.userType() );

  collections = var.value<Collection::List>();
  QCOMPARE( (int)collections.count(), 1 );
  QCOMPARE( collections.first(), collection2 );

  const QFileInfo indexFileInfo2( mDir->name(), QLatin1String( ".collection2_renamed.index" ) );
  QVERIFY( !indexFileInfo2.exists() );

  // get the items and check the flags (see data/README)
  itemFetch = mStore->fetchItems( collections.first() );
  QVERIFY( itemFetch->exec() );
  QCOMPARE( itemFetch->error(), 0 );

  items = itemFetch->items();
  QCOMPARE( (int)items.count(), 4 );

  flagCounts.clear();
  Q_FOREACH( const Item &item, items ) {
    Q_FOREACH( const QByteArray &flag, item.flags() ) {
      ++flagCounts[ flag ];
    }
  }

  QCOMPARE( flagCounts[ "\\SEEN" ], 2 );
  QCOMPARE( flagCounts[ "\\FLAGGED" ], 1 );
  QCOMPARE( flagCounts[ "$TODO" ], 1 );
}

QTEST_KDEMAIN( CollectionModifyTest, NoGUI )

#include "collectionmodifytest.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
