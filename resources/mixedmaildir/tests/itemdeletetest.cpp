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

#include "filestore/entitycompactchangeattribute.h"
#include "filestore/itemdeletejob.h"
#include "filestore/itemfetchjob.h"
#include "filestore/storecompactjob.h"

#include "libmaildir/maildir.h"

#include <kmbox/mbox.h>
#include <kmime/kmime_message.h>

#include <KRandom>
#include <KTempDir>

#include <QSignalSpy>

#include <qtest_kde.h>

using namespace Akonadi;

static Collection::List collectionsFromSpy( QSignalSpy *spy ) {
  Collection::List collections;

  QListIterator<QList<QVariant> > it( *spy );
  while( it.hasNext() ) {
    const QList<QVariant> invocation = it.next();
    Q_ASSERT( invocation.count() == 1 );

    collections << invocation.first().value<Collection::List>();
  }

  return collections;
}

static Item::List itemsFromSpy( QSignalSpy *spy ) {
  Item::List items;

  QListIterator<QList<QVariant> > it( *spy );
  while( it.hasNext() ) {
    const QList<QVariant> invocation = it.next();
    Q_ASSERT( invocation.count() == 1 );

    items << invocation.first().value<Item::List>();
  }

  return items;
}

static bool fullEntryCompare( const KMBox::MBoxEntry &a, const KMBox::MBoxEntry &b )
{
  return a.messageOffset() == b.messageOffset() &&
         a.separatorSize() == b.separatorSize() &&
         a.messageSize() == b.messageSize();
}

class ItemDeleteTest : public QObject
{
  Q_OBJECT

  public:
    ItemDeleteTest() : QObject(), mStore( 0 ), mDir( 0 ) {
      // for monitoring signals
      qRegisterMetaType<Akonadi::Collection::List>();
      qRegisterMetaType<Akonadi::Item::List>();
    }

    ~ItemDeleteTest() {
      delete mStore;
      delete mDir;
    }

  private:
    MixedMaildirStore *mStore;
    KTempDir *mDir;

  private Q_SLOTS:
    void init();
    void cleanup();
    void testMaildir();
    void testMBox();
    void testCachePreservation();
    void testExpectedFailure();
};

void ItemDeleteTest::init()
{
  mStore = new MixedMaildirStore;

  mDir = new KTempDir;
  QVERIFY( mDir->exists() );
}

void ItemDeleteTest::cleanup()
{
  delete mStore;
  mStore = 0;
  delete mDir;
  mDir = 0;
}

void ItemDeleteTest::testMaildir()
{
  QDir topDir( mDir->name() );

  QVERIFY( TestDataUtil::installFolder( QLatin1String( "maildir" ), topDir.path(), QLatin1String( "collection1" ) ) );

  KPIM::Maildir topLevelMd( topDir.path(), true );
  KPIM::Maildir md1 = topLevelMd.subFolder( QLatin1String( "collection1" ) );
  QVERIFY( md1.isValid() );

  QSet<QString> entrySet1 = QSet<QString>::fromList( md1.entryList() );
  QCOMPARE( (int)entrySet1.count(), 4 );

  mStore->setPath( topDir.path() );

  // common variables
  FileStore::ItemDeleteJob *job = 0;
  QSet<QString> entrySet;
  QSet<QString> delIdSet;
  QString delId;

  // test deleting one message
  Collection collection1;
  collection1.setName( QLatin1String( "collection1" ) );
  collection1.setRemoteId( QLatin1String( "collection1" ) );
  collection1.setParentCollection( mStore->topLevelCollection() );

  Item item1;
  item1.setMimeType( KMime::Message::mimeType() );
  item1.setId( KRandom::random() );
  item1.setRemoteId( entrySet1.values().first() );
  item1.setParentCollection( collection1 );

  job = mStore->deleteItem( item1 );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  Item item = job->item();
  QCOMPARE( item.id(), item1.id() );

  entrySet = QSet<QString>::fromList( md1.entryList() );
  QCOMPARE( (int)entrySet.count(), 3 );

  delIdSet = entrySet1.subtract( entrySet );
  QCOMPARE( (int)delIdSet.count(), 1 );

  delId = delIdSet.values().first();
  QCOMPARE( delId, entrySet1.values().first() );
  QCOMPARE( delId, item.remoteId() );

  // test failure of deleting again
  job = mStore->deleteItem( item1 );

  QVERIFY( !job->exec() );
  QCOMPARE( job->error(), (int)FileStore::Job::InvalidJobContext );
}

void ItemDeleteTest::testMBox()
{
  QDir topDir( mDir->name() );

  QVERIFY( TestDataUtil::installFolder( QLatin1String( "mbox" ), topDir.path(), QLatin1String( "collection1" ) ) );

  QFileInfo fileInfo1( topDir.path(), QLatin1String( "collection1" ) );
  KMBox::MBox mbox1;
  QVERIFY( mbox1.load( fileInfo1.absoluteFilePath() ) );
  KMBox::MBoxEntry::List entryList1 = mbox1.entries();
  QCOMPARE( (int)entryList1.count(), 4 );
  int size1 = fileInfo1.size();

  mStore->setPath( topDir.path() );

  // common variables
  FileStore::ItemDeleteJob *job = 0;
  FileStore::ItemFetchJob *itemFetch = 0;
  FileStore::StoreCompactJob *storeCompact = 0;

  Item::List items;
  Collection::List collections;
  KMBox::MBoxEntry::List entryList;

  QSignalSpy *collectionsSpy = 0;
  QSignalSpy *itemsSpy = 0;

  QVariant var;

  // test deleting last item in mbox
  // file stays untouched, message still accessible through MBox, but item gone
  Collection collection1;
  collection1.setName( QLatin1String( "collection1" ) );
  collection1.setRemoteId( QLatin1String( "collection1" ) );
  collection1.setParentCollection( mStore->topLevelCollection() );

  Item item4;
  item4.setMimeType( KMime::Message::mimeType() );
  item4.setId( KRandom::random() );
  item4.setRemoteId( QString::number( entryList1.value( 3 ).messageOffset() ) );
  item4.setParentCollection( collection1 );

  job = mStore->deleteItem( item4 );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  Item item = job->item();
  QCOMPARE( item.id(), item4.id() );

  fileInfo1.refresh();
  QCOMPARE( (int) fileInfo1.size(), size1 );
  QVERIFY( mbox1.load( fileInfo1.absoluteFilePath() ) );
  entryList = mbox1.entries();
  QCOMPARE( entryList.count(), entryList1.count() );
  QCOMPARE( entryList.value( 3 ).messageOffset(), entryList1.value( 3 ).messageOffset() );

  var = job->property( "compactStore" );
  QVERIFY( var.isValid() );
  QCOMPARE( var.type(), QVariant::Bool );
  QCOMPARE( var.toBool(), true );

  itemFetch = mStore->fetchItems( collection1 );

  QVERIFY( itemFetch->exec() );
  QCOMPARE( itemFetch->error(), 0 );

  items = itemFetch->items();
  QCOMPARE( (int)items.count(), 3 );
  QCOMPARE( items.value( 0 ).remoteId(), QString::number( entryList1.value( 0 ).messageOffset() ) );
  QCOMPARE( items.value( 1 ).remoteId(), QString::number( entryList1.value( 1 ).messageOffset() ) );
  QCOMPARE( items.value( 2 ).remoteId(), QString::number( entryList1.value( 2 ).messageOffset() ) );

  // test that the item is purged from the file on store compaction
  // last item purging does not change any others
  storeCompact = mStore->compactStore();

  collectionsSpy = new QSignalSpy( storeCompact, SIGNAL( collectionsChanged( Akonadi::Collection::List ) ) );
  itemsSpy = new QSignalSpy( storeCompact, SIGNAL( itemsChanged( Akonadi::Item::List ) ) );

  QVERIFY( storeCompact->exec() );
  QCOMPARE( storeCompact->error(), 0 );

  collections = storeCompact->changedCollections();
  QCOMPARE( collections.count(), 0 );
  items = storeCompact->changedItems();
  QCOMPARE( items.count(), 0 );

  QCOMPARE( collectionsFromSpy( collectionsSpy ), collections );
  QCOMPARE( itemsFromSpy( itemsSpy ), items );

  fileInfo1.refresh();
  QVERIFY( fileInfo1.size() < size1 );
  size1 = fileInfo1.size();
  QVERIFY( mbox1.load( fileInfo1.absoluteFilePath() ) );
  entryList = mbox1.entries();
  entryList1.pop_back();
  QVERIFY( std::equal( entryList1.begin(), entryList1.end(), entryList.begin(), fullEntryCompare ) );

  // test deleting item somewhere between first and last
  // again, file stays untouched, message still accessible through MBox, but item gone
  Item item2;
  item2.setMimeType( KMime::Message::mimeType() );
  item2.setId( KRandom::random() );
  item2.setRemoteId( QString::number( entryList1.value( 1 ).messageOffset() ) );
  item2.setParentCollection( collection1 );

  job = mStore->deleteItem( item2 );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  item = job->item();
  QCOMPARE( item.id(), item2.id() );

  fileInfo1.refresh();
  QCOMPARE( (int) fileInfo1.size(), size1 );
  QVERIFY( mbox1.load( fileInfo1.absoluteFilePath() ) );
  entryList = mbox1.entries();
  QCOMPARE( entryList.count(), entryList1.count() );
  QCOMPARE( entryList.value( 1 ).messageOffset(), entryList1.value( 1 ).messageOffset() );

  var = job->property( "compactStore" );
  QVERIFY( var.isValid() );
  QCOMPARE( var.type(), QVariant::Bool );
  QCOMPARE( var.toBool(), true );

  itemFetch = mStore->fetchItems( collection1 );

  QVERIFY( itemFetch->exec() );
  QCOMPARE( itemFetch->error(), 0 );

  items = itemFetch->items();
  QCOMPARE( (int)items.count(), 2 );
  QCOMPARE( items.value( 0 ).remoteId(), QString::number( entryList1.value( 0 ).messageOffset() ) );
  QCOMPARE( items.value( 1 ).remoteId(), QString::number( entryList1.value( 2 ).messageOffset() ) );

  // test that the item is purged from the file on store compaction
  // non-last item purging changes all items after it
  storeCompact = mStore->compactStore();

  collectionsSpy = new QSignalSpy( storeCompact, SIGNAL( collectionsChanged( Akonadi::Collection::List ) ) );
  itemsSpy = new QSignalSpy( storeCompact, SIGNAL( itemsChanged( Akonadi::Item::List ) ) );

  QVERIFY( storeCompact->exec() );
  QCOMPARE( storeCompact->error(), 0 );

  collections = storeCompact->changedCollections();
  QCOMPARE( collections.count(), 0 );
  items = storeCompact->changedItems();
  QCOMPARE( items.count(), 1 );

  QCOMPARE( collectionsFromSpy( collectionsSpy ), collections );
  QCOMPARE( itemsFromSpy( itemsSpy ), items );

  Item item3;
  item3.setRemoteId( QString::number( entryList1.value( 2 ).messageOffset() ) );

  item = items.first();
  QCOMPARE( item3.remoteId(), item.remoteId() );

  QVERIFY( item.hasAttribute<FileStore::EntityCompactChangeAttribute>() );
  FileStore::EntityCompactChangeAttribute *attribute =
    item.attribute<FileStore::EntityCompactChangeAttribute>();

  QString newRemoteId = attribute->remoteId();
  QVERIFY( !newRemoteId.isEmpty() );

  fileInfo1.refresh();
  QVERIFY( fileInfo1.size() < size1 );
  size1 = fileInfo1.size();
  QVERIFY( mbox1.load( fileInfo1.absoluteFilePath() ) );
  entryList = mbox1.entries();
  QCOMPARE( QString::number( entryList.value( 1 ).messageOffset() ), newRemoteId );

  entryList1.removeAt( 1 );
  QCOMPARE( entryList1.count(), entryList.count() );
  QCOMPARE( QString::number( entryList1.value( 1 ).messageOffset() ), item3.remoteId() );
}

void ItemDeleteTest::testCachePreservation()
{
  QDir topDir( mDir->name() );

  QVERIFY( TestDataUtil::installFolder( QLatin1String( "maildir" ), topDir.path(), QLatin1String( "collection1" ) ) );
  QVERIFY( TestDataUtil::installFolder( QLatin1String( "mbox" ), topDir.path(), QLatin1String( "collection2" ) ) );

  KPIM::Maildir topLevelMd( topDir.path(), true );
  KPIM::Maildir md1 = topLevelMd.subFolder( QLatin1String( "collection1" ) );
  QVERIFY( md1.isValid() );

  QSet<QString> entrySet1 = QSet<QString>::fromList( md1.entryList() );
  QCOMPARE( (int)entrySet1.count(), 4 );

  QFileInfo fileInfo2( topDir.path(), QLatin1String( "collection2" ) );
  KMBox::MBox mbox2;
  QVERIFY( mbox2.load( fileInfo2.absoluteFilePath() ) );
  KMBox::MBoxEntry::List entryList2 = mbox2.entries();
  QCOMPARE( (int)entryList2.count(), 4 );

  mStore->setPath( topDir.path() );

  // common variables
  const QVariant colListVar = QVariant::fromValue<Collection::List>( Collection::List() );
  QVariant var;
  Collection::List collections;
  Item::List items;
  QMap<QByteArray, int> flagCounts;

  FileStore::ItemDeleteJob *job = 0;
  FileStore::ItemFetchJob *itemFetch = 0;

  // test deleting from maildir
  Collection collection1;
  collection1.setName( QLatin1String( "collection1" ) );
  collection1.setRemoteId( QLatin1String( "collection1" ) );
  collection1.setParentCollection( mStore->topLevelCollection() );

  Item item1;
  item1.setMimeType( KMime::Message::mimeType() );
  item1.setId( KRandom::random() );
  item1.setRemoteId( entrySet1.values().first() );
  item1.setParentCollection( collection1 );

  job = mStore->deleteItem( item1 );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  Item item = job->item();
  QCOMPARE( item.id(), item1.id() );

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
  QCOMPARE( (int)items.count(), 3 );
  Q_FOREACH( const Item &item, items ) {
    Q_FOREACH( const QByteArray &flag, item.flags() ) {
      ++flagCounts[ flag ];
    }
  }

  // TODO since we don't know which message we've deleted, we can only check if some flags are present
  int flagCountTotal = 0;
  Q_FOREACH( int count, flagCounts ) {
    flagCountTotal += count;
  }
  QVERIFY( flagCountTotal > 0 );
  flagCounts.clear();

  // test deleting from mbox
  Collection collection2;
  collection2.setName( QLatin1String( "collection2" ) );
  collection2.setRemoteId( QLatin1String( "collection2" ) );
  collection2.setParentCollection( mStore->topLevelCollection() );

  Item item2;
  item2.setMimeType( KMime::Message::mimeType() );
  item2.setId( KRandom::random() );
  item2.setRemoteId( QString::number( entryList2.value( 1 ).messageOffset() ) );
  item2.setParentCollection( collection2 );

  job = mStore->deleteItem( item2 );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  item = job->item();
  QCOMPARE( item.id(), item2.id() );

  // at this point no change has been written to disk yet, so index and mbox file are
  // still in sync
  var = job->property( "onDiskIndexInvalidated" );
  QVERIFY( !var.isValid() );

  FileStore::StoreCompactJob *storeCompact = mStore->compactStore();

  QVERIFY( storeCompact->exec() );
  QCOMPARE( storeCompact->error(), 0 );

  // check for index preservation
  var = storeCompact->property( "onDiskIndexInvalidated" );
  QVERIFY( var.isValid() );
  QCOMPARE( var.userType(), colListVar.userType() );

  collections = var.value<Collection::List>();
  QCOMPARE( (int)collections.count(), 1 );
  QCOMPARE( collections.first(), collection2 );

  // get the items and check the flags (see data/README)
  itemFetch = mStore->fetchItems( collection2 );
  QVERIFY( itemFetch->exec() );
  QCOMPARE( itemFetch->error(), 0 );

  items = itemFetch->items();
  QCOMPARE( (int)items.count(), 3 );
  Q_FOREACH( const Item &item, items ) {
    Q_FOREACH( const QByteArray &flag, item.flags() ) {
      ++flagCounts[ flag ];
    }
  }

  // we've deleted message 2, it flagged TODO and seen
  QCOMPARE( flagCounts[ "\\SEEN" ], 1 );
  QCOMPARE( flagCounts[ "\\FLAGGED" ], 1 );
  flagCounts.clear();
}

void ItemDeleteTest::testExpectedFailure()
{
  QDir topDir( mDir->name() );

  QVERIFY( TestDataUtil::installFolder( QLatin1String( "maildir" ), topDir.path(), QLatin1String( "collection1" ) ) );
  QVERIFY( TestDataUtil::installFolder( QLatin1String( "mbox" ), topDir.path(), QLatin1String( "collection2" ) ) );

  KPIM::Maildir topLevelMd( topDir.path(), true );
  KPIM::Maildir md1 = topLevelMd.subFolder( QLatin1String( "collection1" ) );
  QVERIFY( md1.isValid() );

  QSet<QString> entrySet1 = QSet<QString>::fromList( md1.entryList() );
  QCOMPARE( (int)entrySet1.count(), 4 );

  QFileInfo fileInfo2( topDir.path(), QLatin1String( "collection2" ) );
  KMBox::MBox mbox2;
  QVERIFY( mbox2.load( fileInfo2.absoluteFilePath() ) );
  KMBox::MBoxEntry::List entryList2 = mbox2.entries();
  QCOMPARE( (int)entryList2.count(), 4 );

  mStore->setPath( topDir.path() );

  // common variables
  FileStore::ItemDeleteJob *job = 0;
  FileStore::ItemFetchJob *itemFetch = 0;
  FileStore::StoreCompactJob *storeCompact = 0;

  // test failure of fetching an item previously deleted from maildir
  Collection collection1;
  collection1.setName( QLatin1String( "collection1" ) );
  collection1.setRemoteId( QLatin1String( "collection1" ) );
  collection1.setParentCollection( mStore->topLevelCollection() );

  Item item1_1;
  item1_1.setRemoteId( entrySet1.values().first() );
  item1_1.setParentCollection( collection1 );

  job = mStore->deleteItem( item1_1 );

  QVERIFY( job->exec() );

  itemFetch = mStore->fetchItem( item1_1 );

  QVERIFY( !itemFetch->exec() );
  QCOMPARE( itemFetch->error(), (int)FileStore::Job::InvalidJobContext );

  // test failure of deleting an item from maildir again
  job = mStore->deleteItem( item1_1 );

  QVERIFY( !job->exec() );
  QCOMPARE( job->error(), (int)FileStore::Job::InvalidJobContext );

  // test failure of fetching an item previously deleted from mbox
  Collection collection2;
  collection2.setName( QLatin1String( "collection2" ) );
  collection2.setRemoteId( QLatin1String( "collection2" ) );
  collection2.setParentCollection( mStore->topLevelCollection() );

  Item item2_1;
  item2_1.setRemoteId( QString::number( entryList2.value( 0 ).messageOffset() ) );
  item2_1.setParentCollection( collection2 );

  job = mStore->deleteItem( item2_1 );

  QVERIFY( job->exec() );

  itemFetch = mStore->fetchItem( item2_1 );

  QVERIFY( !itemFetch->exec() );
  QCOMPARE( itemFetch->error(), (int)FileStore::Job::InvalidJobContext );

  // test failure of deleting an item from mbox again
  job = mStore->deleteItem( item2_1 );

  QVERIFY( !job->exec() );
  QCOMPARE( job->error(), (int)FileStore::Job::InvalidJobContext );

  // compact store and check that offset 0 is a valid remoteId again, but
  // offset of other items (e.f. item 4) are no longer valid (moved to the front of the file)
  storeCompact = mStore->compactStore();

  QVERIFY( storeCompact->exec() );

  itemFetch = mStore->fetchItem( item2_1 );

  QVERIFY( itemFetch->exec() );

  Item item4_1;
  item4_1.setRemoteId( QString::number( entryList2.value( 3 ).messageOffset() ) );
  item4_1.setParentCollection( collection2 );

  itemFetch = mStore->fetchItem( item4_1 );

  QVERIFY( !itemFetch->exec() );
  QCOMPARE( itemFetch->error(), (int)FileStore::Job::InvalidJobContext );
}

QTEST_KDEMAIN( ItemDeleteTest, NoGUI )

#include "itemdeletetest.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
