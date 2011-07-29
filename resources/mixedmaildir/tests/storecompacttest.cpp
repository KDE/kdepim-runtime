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
#include "filestore/storecompactjob.h"

#include <kmbox/mbox.h>

#include <KRandom>
#include <KTempDir>

#include <QSignalSpy>

#include <qtest_kde.h>

using namespace Akonadi;
using namespace KMBox;

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

static bool fullEntryCompare( const MBoxEntry &a, const MBoxEntry &b )
{
  return a.messageOffset() == b.messageOffset() &&
         a.separatorSize() == b.separatorSize() &&
         a.messageSize() == b.messageSize();
}

static quint64 changedOffset( const Item &item ) {
  Q_ASSERT( item.hasAttribute<FileStore::EntityCompactChangeAttribute>() );

  const QString remoteId = item.attribute<FileStore::EntityCompactChangeAttribute>()->remoteId();
  Q_ASSERT( !remoteId.isEmpty() );

  bool ok = false;
  const quint64 result = remoteId.toULongLong( &ok );
  Q_ASSERT( ok );

  return result;
}

class StoreCompactTest : public QObject
{
  Q_OBJECT

  public:
    StoreCompactTest() : QObject(), mStore( 0 ), mDir( 0 ) {
      // for monitoring signals
      qRegisterMetaType<Akonadi::Collection::List>();
      qRegisterMetaType<Akonadi::Item::List>();
    }

    ~StoreCompactTest() {
      delete mStore;
      delete mDir;
    }

  private:
    MixedMaildirStore *mStore;
    KTempDir *mDir;

  private Q_SLOTS:
    void init();
    void cleanup();
    void testCompact();
};

void StoreCompactTest::init()
{
  mStore = new MixedMaildirStore;

  mDir = new KTempDir;
  QVERIFY( mDir->exists() );
}

void StoreCompactTest::cleanup()
{
  delete mStore;
  mStore = 0;
  delete mDir;
  mDir = 0;
}

void StoreCompactTest::testCompact()
{
  QDir topDir( mDir->name() );

  QVERIFY( TestDataUtil::installFolder( QLatin1String( "mbox" ), topDir.path(), QLatin1String( "collection1" ) ) );
  QVERIFY( TestDataUtil::installFolder( QLatin1String( "mbox" ), topDir.path(), QLatin1String( "collection2" ) ) );
  QVERIFY( TestDataUtil::installFolder( QLatin1String( "mbox" ), topDir.path(), QLatin1String( "collection3" ) ) );
  QVERIFY( TestDataUtil::installFolder( QLatin1String( "mbox" ), topDir.path(), QLatin1String( "collection4" ) ) );

  QFileInfo fileInfo1( topDir.path(), QLatin1String( "collection1" ) );
  MBox mbox1;
  QVERIFY( mbox1.load( fileInfo1.absoluteFilePath() ) );
  MBoxEntry::List entryList1 = mbox1.entries();
  QCOMPARE( (int)entryList1.count(), 4 );

  QFileInfo fileInfo2( topDir.path(), QLatin1String( "collection2" ) );
  MBox mbox2;
  QVERIFY( mbox2.load( fileInfo2.absoluteFilePath() ) );
  MBoxEntry::List entryList2 = mbox2.entries();
  QCOMPARE( (int)entryList2.count(), 4 );

  QFileInfo fileInfo3( topDir.path(), QLatin1String( "collection3" ) );
  MBox mbox3;
  QVERIFY( mbox3.load( fileInfo3.absoluteFilePath() ) );
  MBoxEntry::List entryList3 = mbox3.entries();
  QCOMPARE( (int)entryList3.count(), 4 );

  QFileInfo fileInfo4( topDir.path(), QLatin1String( "collection4" ) );
  MBox mbox4;
  QVERIFY( mbox4.load( fileInfo4.absoluteFilePath() ) );
  MBoxEntry::List entryList4 = mbox4.entries();
  QCOMPARE( (int)entryList4.count(), 4 );

  mStore->setPath( topDir.path() );

  // common variables
  FileStore::CollectionFetchJob *collectionFetch = 0;
  FileStore::ItemDeleteJob *itemDelete = 0;
  FileStore::StoreCompactJob *job = 0;

  Collection::List collections;
  Item::List items;

  QSignalSpy *collectionSpy = 0;
  QSignalSpy *itemSpy = 0;

  MBoxEntry::List entryList;
  Collection collection;
  FileStore::EntityCompactChangeAttribute *attribute = 0;

  const QVariant colListVar = QVariant::fromValue<Collection::List>( Collection::List() );
  QVariant var;

  // test compact after delete from the end of an mbox
  Collection collection1;
  collection1.setName( QLatin1String( "collection1" ) );
  collection1.setRemoteId( QLatin1String( "collection1" ) );
  collection1.setParentCollection( mStore->topLevelCollection() );

  Item item1;
  item1.setRemoteId( QString::number( entryList1.last().messageOffset() ) );
  item1.setParentCollection( collection1 );

  itemDelete = mStore->deleteItem( item1 );

  QVERIFY( itemDelete->exec() );

  job = mStore->compactStore();

  collectionSpy = new QSignalSpy( job, SIGNAL(collectionsChanged(Akonadi::Collection::List)) );
  itemSpy = new QSignalSpy( job, SIGNAL(itemsChanged(Akonadi::Item::List)) );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  collections = job->changedCollections();
  items = job->changedItems();

  QCOMPARE( collections.count(), 0 );
  QCOMPARE( items.count(), 0 );

  QCOMPARE( collectionSpy->count(), 0 );
  QCOMPARE( itemSpy->count(), 0 );

  QVERIFY( mbox1.load( mbox1.fileName() ) );
  entryList = mbox1.entries();
  entryList1.pop_back();
  QVERIFY( std::equal( entryList.begin(), entryList.end(), entryList1.begin(), fullEntryCompare ) );

  var = job->property( "onDiskIndexInvalidated" );
  QVERIFY( var.isValid() );
  QCOMPARE( var.userType(), colListVar.userType() );

  collections = var.value<Collection::List>();
  QCOMPARE( (int)collections.count(), 1 );
  QCOMPARE( collections, Collection::List() << collection1 );

  // test compact after delete from before the end of an mbox
  Collection collection2;
  collection2.setName( QLatin1String( "collection2" ) );
  collection2.setRemoteId( QLatin1String( "collection2" ) );
  collection2.setParentCollection( mStore->topLevelCollection() );

  Item item2;
  item2.setRemoteId( QString::number( entryList2.first().messageOffset() ) );
  item2.setParentCollection( collection2 );

  itemDelete = mStore->deleteItem( item2 );

  QVERIFY( itemDelete->exec() );

  job = mStore->compactStore();

  collectionSpy = new QSignalSpy( job, SIGNAL(collectionsChanged(Akonadi::Collection::List)) );
  itemSpy = new QSignalSpy( job, SIGNAL(itemsChanged(Akonadi::Item::List)) );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  collections = job->changedCollections();
  items = job->changedItems();

  QCOMPARE( collections.count(), 1 );
  QCOMPARE( items.count(), 3 );

  QCOMPARE( collectionSpy->count(), 1 );
  QCOMPARE( itemSpy->count(), 1 );

  QCOMPARE( collectionsFromSpy( collectionSpy ), collections );
  QCOMPARE( itemsFromSpy( itemSpy ), items );

  collection = collections.first();
  QCOMPARE( collection, collection2 );
  QVERIFY( collection.hasAttribute<FileStore::EntityCompactChangeAttribute>() );
  attribute = collection.attribute<FileStore::EntityCompactChangeAttribute>();
  QCOMPARE( attribute->remoteRevision().toInt(), collection2.remoteRevision().toInt() + 1 );

  QVERIFY( mbox2.load( mbox2.fileName() ) );
  entryList = mbox2.entries();

  entryList2.pop_front();
  for ( int i = 0; i < items.count(); ++i ) {
    entryList2[ i ] = MBoxEntry( changedOffset( items[ i ] ) );
  }
  QCOMPARE( entryList, entryList2 );

  var = job->property( "onDiskIndexInvalidated" );
  QVERIFY( var.isValid() );
  QCOMPARE( var.userType(), colListVar.userType() );

  collections = var.value<Collection::List>();
  QCOMPARE( (int)collections.count(), 1 );
  QCOMPARE( collections, Collection::List() << collection2 );

  collectionFetch = mStore->fetchCollections( collection2, FileStore::CollectionFetchJob::Base );

  QVERIFY( collectionFetch->exec() );

  collections = collectionFetch->collections();
  QCOMPARE( (int)collections.count(), 1 );

  collection = collections.first();
  QCOMPARE( collection, collection2 );
  QCOMPARE( collection.remoteRevision(), attribute->remoteRevision() );

  // test compact after delete from before the end of more than one mbox
  Collection collection3;
  collection3.setName( QLatin1String( "collection3" ) );
  collection3.setRemoteId( QLatin1String( "collection3" ) );
  collection3.setParentCollection( mStore->topLevelCollection() );

  Item item3;
  item3.setRemoteId( QString::number( entryList3.first().messageOffset() ) );
  item3.setParentCollection( collection3 );

  itemDelete = mStore->deleteItem( item3 );

  QVERIFY( itemDelete->exec() );

  Collection collection4;
  collection4.setName( QLatin1String( "collection4" ) );
  collection4.setRemoteId( QLatin1String( "collection4" ) );
  collection4.setParentCollection( mStore->topLevelCollection() );

  Item item4;
  item4.setRemoteId( QString::number( entryList3.value( 1 ).messageOffset() ) );
  item4.setParentCollection( collection4 );

  itemDelete = mStore->deleteItem( item4 );

  QVERIFY( itemDelete->exec() );

  job = mStore->compactStore();

  collectionSpy = new QSignalSpy( job, SIGNAL(collectionsChanged(Akonadi::Collection::List)) );
  itemSpy = new QSignalSpy( job, SIGNAL(itemsChanged(Akonadi::Item::List)) );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  collections = job->changedCollections();
  items = job->changedItems();

  QCOMPARE( collections.count(), 2 );
  QCOMPARE( items.count(), 5 );

  QCOMPARE( collectionSpy->count(), 2 );
  QCOMPARE( itemSpy->count(), 2 );

  QCOMPARE( collectionsFromSpy( collectionSpy ), collections );
  QCOMPARE( itemsFromSpy( itemSpy ), items );

  collection = collections.first();
  QCOMPARE( collection, collection3 );
  QVERIFY( collection.hasAttribute<FileStore::EntityCompactChangeAttribute>() );
  attribute = collection.attribute<FileStore::EntityCompactChangeAttribute>();
  QCOMPARE( attribute->remoteRevision().toInt(), collection3.remoteRevision().toInt() + 1 );

  QVERIFY( mbox3.load( mbox3.fileName() ) );
  entryList = mbox3.entries();

  entryList3.pop_front();
  for ( int i = 0; i < entryList3.count(); ++i ) {
    entryList3[ i ] = MBoxEntry( changedOffset( items.first() ) );
    items.pop_front();
  }
  QCOMPARE( entryList, entryList3 );

  collection = collections.last();
  QCOMPARE( collection, collection4 );
  QVERIFY( collection.hasAttribute<FileStore::EntityCompactChangeAttribute>() );
  attribute = collection.attribute<FileStore::EntityCompactChangeAttribute>();
  QCOMPARE( attribute->remoteRevision().toInt(), collection4.remoteRevision().toInt() + 1 );

  QVERIFY( mbox4.load( mbox4.fileName() ) );
  entryList = mbox4.entries();

  entryList4.removeAt( 1 );
  for ( int i = 0; i < items.count(); ++i ) {
    entryList4[ i + 1 ] = MBoxEntry( changedOffset( items[ i ] ) );
  }
  QCOMPARE( entryList, entryList4 );

  var = job->property( "onDiskIndexInvalidated" );
  QVERIFY( var.isValid() );
  QCOMPARE( var.userType(), colListVar.userType() );

  collections = var.value<Collection::List>();
  QCOMPARE( (int)collections.count(), 2 );
  QCOMPARE( collections, Collection::List() << collection3 << collection4 );
}

QTEST_KDEMAIN( StoreCompactTest, NoGUI )

#include "storecompacttest.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
