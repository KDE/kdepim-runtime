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

#include "filestore/itemcreatejob.h"
#include "filestore/itemfetchjob.h"

#include "libmaildir/maildir.h"

#include <kmbox/mbox.h>
#include <kmime/kmime_message.h>

#include <KRandom>
#include <KTempDir>

#include <qtest_kde.h>

using namespace Akonadi;

class ItemCreateTest : public QObject
{
  Q_OBJECT

  public:
    ItemCreateTest() : QObject(), mStore( 0 ), mDir( 0 ) {}

    ~ItemCreateTest() {
      delete mStore;
      delete mDir;
    }

  private:
    MixedMaildirStore *mStore;
    KTempDir *mDir;

  private Q_SLOTS:
    void init();
    void cleanup();
    void testExpectedFail();
    void testMBox();
    void testMaildir();
};

void ItemCreateTest::init()
{
  mStore = new MixedMaildirStore;

  mDir = new KTempDir;
  QVERIFY( mDir->exists() );
}

void ItemCreateTest::cleanup()
{
  delete mStore;
  mStore = 0;
  delete mDir;
  mDir = 0;
}

void ItemCreateTest::testExpectedFail()
{
  QDir topDir( mDir->name() );

  QVERIFY( TestDataUtil::installFolder( QLatin1String( "maildir" ), topDir.path(), QLatin1String( "data" ) ) );
  QDir dataDir = topDir;
  QVERIFY( dataDir.cd( QLatin1String( "data" ) ) );
  KPIM::Maildir dataMd( dataDir.path(), false );
  QVERIFY( dataMd.isValid() );

  const QStringList dataEntryList = dataMd.entryList();
  QCOMPARE( dataEntryList.count(), 4 );
  KMime::Message::Ptr msgPtr( new KMime::Message );
  msgPtr->setContent( KMime::CRLFtoLF( dataMd.readEntry( dataEntryList.first() ) ) );

  QVERIFY( topDir.mkdir( QLatin1String( "store" ) ) );
  QVERIFY( topDir.cd( QLatin1String( "store" ) ) );
  mStore->setPath( topDir.path() );

  FileStore::ItemCreateJob *job = 0;

  // test failure of adding item to top level collection
  Item item;
  item.setMimeType( KMime::Message::mimeType() );
  item.setPayload<KMime::Message::Ptr>( msgPtr );

  job = mStore->createItem( item, mStore->topLevelCollection() );

  QVERIFY( !job->exec() );
  QCOMPARE( job->error(), (int)FileStore::Job::InvalidJobContext );

  // test failure of adding item to non existant collection
  Collection collection;
  collection.setName( QLatin1String( "collection" ) );
  collection.setRemoteId( QLatin1String( "collection" ) );
  collection.setParentCollection( mStore->topLevelCollection() );

  job = mStore->createItem( item, collection );

  QVERIFY( !job->exec() );
  QCOMPARE( job->error(), (int)FileStore::Job::InvalidJobContext );
}

void ItemCreateTest::testMBox()
{
  QDir topDir( mDir->name() );

  QVERIFY( TestDataUtil::installFolder( QLatin1String( "maildir" ), topDir.path(), QLatin1String( "data" ) ) );
  QDir dataDir = topDir;
  QVERIFY( dataDir.cd( QLatin1String( "data" ) ) );
  KPIM::Maildir dataMd( dataDir.path(), false );
  QVERIFY( dataMd.isValid() );

  const QStringList dataEntryList = dataMd.entryList();
  QCOMPARE( dataEntryList.count(), 4 );
  KMime::Message::Ptr msgPtr1( new KMime::Message );
  msgPtr1->setContent( KMime::CRLFtoLF( dataMd.readEntry( dataEntryList.first() ) ) );
  KMime::Message::Ptr msgPtr2( new KMime::Message );
  msgPtr2->setContent( KMime::CRLFtoLF( dataMd.readEntry( dataEntryList.last() ) ) );

  QVERIFY( topDir.mkdir( QLatin1String( "store" ) ) );
  QVERIFY( topDir.cd( QLatin1String( "store" ) ) );

  QVERIFY( TestDataUtil::installFolder( QLatin1String( "mbox" ), topDir.path(), QLatin1String( "collection1" ) ) );

  QFileInfo fileInfo1( topDir.path(), QLatin1String( "collection1" ) );
  KMBox::MBox mbox1;
  QVERIFY( mbox1.load( fileInfo1.absoluteFilePath() ) );
  QCOMPARE( (int)mbox1.entries().count(), 4 );
  const int size1 = fileInfo1.size();

  // simulate empty mbox
  QFileInfo fileInfo2( topDir.path(), QLatin1String( "collection2" ) );
  QFile file2( fileInfo2.absoluteFilePath() );
  QVERIFY( file2.open( QIODevice::WriteOnly ) );
  file2.close();
  QVERIFY( file2.exists() );
  QCOMPARE( (int)file2.size(), 0 );

  mStore->setPath( topDir.path() );

  // common variables
  const QVariant colListVar = QVariant::fromValue<Collection::List>( Collection::List() );
  QVariant var;
  Collection::List collections;
  Item::List items;
  QMap<QByteArray, int> flagCounts;

  FileStore::ItemCreateJob *job = 0;
  FileStore::ItemFetchJob *itemFetch = 0;

  // test adding to empty mbox
  Collection collection2;
  collection2.setName( QLatin1String( "collection2" ) );
  collection2.setRemoteId( QLatin1String( "collection2" ) );
  collection2.setParentCollection( mStore->topLevelCollection() );

  Item item1;
  item1.setId( KRandom::random() );
  item1.setMimeType( KMime::Message::mimeType() );
  item1.setPayload<KMime::Message::Ptr>( msgPtr1 );

  job = mStore->createItem( item1, collection2 );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  Item item = job->item();
  QCOMPARE( item.id(), item1.id() );
  QVERIFY( !item.remoteId().isEmpty() );
  QCOMPARE( item.remoteId(), QLatin1String( "0" ) );
  QCOMPARE( item.parentCollection(), collection2 );

  fileInfo2.refresh();
  QVERIFY( fileInfo2.size() > 0 );
  const int size2 = fileInfo2.size();

  KMBox::MBox mbox2;
  QVERIFY( mbox2.load( fileInfo2.absoluteFilePath() ) );
  QCOMPARE( (int)mbox2.entries().count(), 1 );

  Item item2;
  item2.setId( KRandom::random() );
  item2.setMimeType( KMime::Message::mimeType() );
  item2.setPayload<KMime::Message::Ptr>( msgPtr2 );

  job = mStore->createItem( item2, collection2 );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  item = job->item();
  QCOMPARE( item.id(), item2.id() );
  QVERIFY( !item.remoteId().isEmpty() );
  QCOMPARE( item.remoteId(), QString::number( size2 + 1) );
  QCOMPARE( item.parentCollection(), collection2 );

  fileInfo2.refresh();
  QVERIFY( fileInfo2.size() > 0 );

  QVERIFY( mbox2.load( fileInfo2.absoluteFilePath() ) );
  QCOMPARE( (int)mbox2.entries().count(), 2 );

  // test adding to non-empty mbox
  Collection collection1;
  collection1.setName( QLatin1String( "collection1" ) );
  collection1.setRemoteId( QLatin1String( "collection1" ) );
  collection1.setParentCollection( mStore->topLevelCollection() );

  job = mStore->createItem( item1, collection1 );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  item = job->item();
  QCOMPARE( item.id(), item1.id() );
  QVERIFY( !item.remoteId().isEmpty() );
  QCOMPARE( item.remoteId(), QString::number( size1 + 1 ) );
  QCOMPARE( item.parentCollection(), collection1 );

  fileInfo1.refresh();
  QVERIFY( fileInfo1.size() > size1 );

  QVERIFY( mbox1.load( fileInfo1.absoluteFilePath() ) );
  QCOMPARE( (int)mbox1.entries().count(), 5 );

  // check for index preservation
  var = job->property( "onDiskIndexInvalidated" );
  QVERIFY( var.isValid() );
  QCOMPARE( var.userType(), colListVar.userType() );

  collections = var.value<Collection::List>();
  QCOMPARE( (int)collections.count(), 1 );
  QCOMPARE( collections.first(), collection1 );

  // get the items and check the flags (see data/README)
  itemFetch = mStore->fetchItems( collection1 );
  QVERIFY( itemFetch->exec() );
  QCOMPARE( itemFetch->error(), 0 );

  items = itemFetch->items();
  QCOMPARE( (int)items.count(), 5 );
  Q_FOREACH( const Item &item, items ) {
    Q_FOREACH( const QByteArray &flag, item.flags() ) {
      ++flagCounts[ flag ];
    }
  }

  QCOMPARE( flagCounts[ "\\SEEN" ], 2 );
  QCOMPARE( flagCounts[ "\\FLAGGED" ], 1 );
  QCOMPARE( flagCounts[ "$TODO" ], 1 );
  flagCounts.clear();

  job = mStore->createItem( item2, collection1 );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  item = job->item();
  QCOMPARE( item.id(), item2.id() );
  QVERIFY( !item.remoteId().isEmpty() );
  QCOMPARE( item.remoteId(), QString::number( size1 + 1 + size2 + 1 ) );
  QCOMPARE( item.parentCollection(), collection1 );

  fileInfo1.refresh();
  QVERIFY( fileInfo1.size() > (size1 + size2 ) );

  QVERIFY( mbox1.load( fileInfo1.absoluteFilePath() ) );
  QCOMPARE( (int)mbox1.entries().count(), 6 );

  // check for index preservation
  var = job->property( "onDiskIndexInvalidated" );
  QVERIFY( var.isValid() );
  QCOMPARE( var.userType(), colListVar.userType() );

  collections = var.value<Collection::List>();
  QCOMPARE( (int)collections.count(), 1 );
  QCOMPARE( collections.first(), collection1 );

  // get the items and check the flags (see data/README)
  itemFetch = mStore->fetchItems( collection1 );
  QVERIFY( itemFetch->exec() );
  QCOMPARE( itemFetch->error(), 0 );

  items = itemFetch->items();
  QCOMPARE( (int)items.count(), 6 );
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

void ItemCreateTest::testMaildir()
{
  QDir topDir( mDir->name() );

  QVERIFY( TestDataUtil::installFolder( QLatin1String( "maildir" ), topDir.path(), QLatin1String( "data" ) ) );
  QDir dataDir = topDir;
  QVERIFY( dataDir.cd( QLatin1String( "data" ) ) );
  KPIM::Maildir dataMd( dataDir.path(), false );
  QVERIFY( dataMd.isValid() );

  const QStringList dataEntryList = dataMd.entryList();
  QCOMPARE( dataEntryList.count(), 4 );
  KMime::Message::Ptr msgPtr1( new KMime::Message );
  msgPtr1->setContent( KMime::CRLFtoLF( dataMd.readEntry( dataEntryList.first() ) ) );
  KMime::Message::Ptr msgPtr2( new KMime::Message );
  msgPtr2->setContent( KMime::CRLFtoLF( dataMd.readEntry( dataEntryList.last() ) ) );

  QVERIFY( topDir.mkdir( QLatin1String( "store" ) ) );
  QVERIFY( topDir.cd( QLatin1String( "store" ) ) );

  QVERIFY( TestDataUtil::installFolder( QLatin1String( "maildir" ), topDir.path(), QLatin1String( "collection1" ) ) );

  KPIM::Maildir topLevelMd( topDir.path(), true );
  KPIM::Maildir md1 = topLevelMd.subFolder( QLatin1String( "collection1" ) );
  QVERIFY( md1.isValid() );

  QSet<QString> entrySet1 = QSet<QString>::fromList( md1.entryList() );
  QCOMPARE( (int)entrySet1.count(), 4 );

  // simulate empty maildir
  KPIM::Maildir md2( topLevelMd.addSubFolder( QLatin1String( "collection2" ) ), false );
  QVERIFY( md2.isValid() );

  QSet<QString> entrySet2 = QSet<QString>::fromList( md2.entryList() );
  QCOMPARE( (int)entrySet2.count(), 0 );

  mStore->setPath( topDir.path() );

  // common variables
  const QVariant colListVar = QVariant::fromValue<Collection::List>( Collection::List() );
  QVariant var;
  Collection::List collections;
  Item::List items;
  QMap<QByteArray, int> flagCounts;

  QSet<QString> entrySet;
  QSet<QString> newIdSet;
  QString newId;

  FileStore::ItemCreateJob *job = 0;
  FileStore::ItemFetchJob *itemFetch = 0;

  // test adding to empty maildir
  Collection collection2;
  collection2.setName( QLatin1String( "collection2" ) );
  collection2.setRemoteId( QLatin1String( "collection2" ) );
  collection2.setParentCollection( mStore->topLevelCollection() );

  Item item1;
  item1.setId( KRandom::random() );
  item1.setMimeType( KMime::Message::mimeType() );
  item1.setPayload<KMime::Message::Ptr>( msgPtr1 );

  job = mStore->createItem( item1, collection2 );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  Item item = job->item();
  QCOMPARE( item.id(), item1.id() );
  QVERIFY( !item.remoteId().isEmpty() );
  QCOMPARE( item.parentCollection(), collection2 );

  entrySet = QSet<QString>::fromList( md2.entryList() );
  QCOMPARE( (int)entrySet.count(), 1 );

  newIdSet = entrySet.subtract( entrySet2 );
  QCOMPARE( (int)newIdSet.count(), 1 );

  newId = newIdSet.values().first();
  QCOMPARE( item.remoteId(), newId );
  entrySet2 << newId;
  QCOMPARE( (int)entrySet2.count(), 1 );

  Item item2;
  item2.setId( KRandom::random() );
  item2.setMimeType( KMime::Message::mimeType() );
  item2.setPayload<KMime::Message::Ptr>( msgPtr2 );

  job = mStore->createItem( item2, collection2 );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  item = job->item();
  QCOMPARE( item.id(), item2.id() );
  QVERIFY( !item.remoteId().isEmpty() );
  QCOMPARE( item.parentCollection(), collection2 );

  entrySet = QSet<QString>::fromList( md2.entryList() );
  QCOMPARE( (int)entrySet.count(), 2 );

  newIdSet = entrySet.subtract( entrySet2 );
  QCOMPARE( (int)newIdSet.count(), 1 );

  newId = newIdSet.values().first();
  QCOMPARE( item.remoteId(), newId );
  entrySet2 << newId;
  QCOMPARE( (int)entrySet2.count(), 2 );

  // test adding to non-empty maildir
  Collection collection1;
  collection1.setName( QLatin1String( "collection1" ) );
  collection1.setRemoteId( QLatin1String( "collection1" ) );
  collection1.setParentCollection( mStore->topLevelCollection() );

  job = mStore->createItem( item1, collection1 );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  item = job->item();
  QCOMPARE( item.id(), item1.id() );
  QVERIFY( !item.remoteId().isEmpty() );
  QCOMPARE( item.parentCollection(), collection1 );

  entrySet = QSet<QString>::fromList( md1.entryList() );
  QCOMPARE( (int)entrySet.count(), 5 );

  newIdSet = entrySet.subtract( entrySet1 );
  QCOMPARE( (int)newIdSet.count(), 1 );

  newId = newIdSet.values().first();
  QCOMPARE( item.remoteId(), newId );
  entrySet1 << newId;
  QCOMPARE( (int)entrySet1.count(), 5 );

  // check for index preservation
  var = job->property( "onDiskIndexInvalidated" );
  QVERIFY( var.isValid() );
  QCOMPARE( var.userType(), colListVar.userType() );

  collections = var.value<Collection::List>();
  QCOMPARE( (int)collections.count(), 1 );
  QCOMPARE( collections.first(), collection1 );

  // get the items and check the flags (see data/README)
  itemFetch = mStore->fetchItems( collection1 );
  QVERIFY( itemFetch->exec() );
  QCOMPARE( itemFetch->error(), 0 );

  items = itemFetch->items();
  QCOMPARE( (int)items.count(), 5 );
  Q_FOREACH( const Item &item, items ) {
    Q_FOREACH( const QByteArray &flag, item.flags() ) {
      ++flagCounts[ flag ];
    }
  }

  QCOMPARE( flagCounts[ "\\SEEN" ], 2 );
  QCOMPARE( flagCounts[ "\\FLAGGED" ], 1 );
  QCOMPARE( flagCounts[ "$TODO" ], 1 );
  flagCounts.clear();

  job = mStore->createItem( item2, collection1 );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  item = job->item();
  QCOMPARE( item.id(), item2.id() );
  QVERIFY( !item.remoteId().isEmpty() );
  QCOMPARE( item.parentCollection(), collection1 );

  entrySet = QSet<QString>::fromList( md1.entryList() );
  QCOMPARE( (int)entrySet.count(), 6 );

  newIdSet = entrySet.subtract( entrySet1 );
  QCOMPARE( (int)newIdSet.count(), 1 );

  newId = newIdSet.values().first();
  QCOMPARE( item.remoteId(), newId );
  entrySet1 << newId;
  QCOMPARE( (int)entrySet1.count(), 6 );

  // check for index preservation
  var = job->property( "onDiskIndexInvalidated" );
  QVERIFY( var.isValid() );
  QCOMPARE( var.userType(), colListVar.userType() );

  collections = var.value<Collection::List>();
  QCOMPARE( (int)collections.count(), 1 );
  QCOMPARE( collections.first(), collection1 );

  // get the items and check the flags (see data/README)
  itemFetch = mStore->fetchItems( collection1 );
  QVERIFY( itemFetch->exec() );
  QCOMPARE( itemFetch->error(), 0 );

  items = itemFetch->items();
  QCOMPARE( (int)items.count(), 6 );
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

QTEST_KDEMAIN( ItemCreateTest, NoGUI )

#include "itemcreatetest.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
