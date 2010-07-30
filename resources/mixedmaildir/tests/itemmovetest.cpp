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
#include "filestore/itemfetchjob.h"
#include "filestore/itemmovejob.h"
#include "filestore/storecompactjob.h"

#include "libmaildir/maildir.h"
#include "libmbox/mbox.h"

#include <kmime/kmime_message.h>

#include <KRandom>
#include <KTempDir>

#include <QSignalSpy>

#include <qtest_kde.h>

using namespace Akonadi;

static bool operator==( const MsgEntryInfo &a, const MsgEntryInfo &b )
{
  return a.offset == b.offset &&
         a.separatorSize == b.separatorSize &&
         a.entrySize == b.entrySize;
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

class ItemMoveTest : public QObject
{
  Q_OBJECT

  public:
    ItemMoveTest()
      : QObject(), mStore( 0 ), mDir( 0 ) {}

    ~ItemMoveTest() {
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
    void testMaildirItem();
    void testMBoxItem();
};

void ItemMoveTest::init()
{
  mStore = new MixedMaildirStore;

  mDir = new KTempDir;
  QVERIFY( mDir->exists() );
}

void ItemMoveTest::cleanup()
{
  delete mStore;
  mStore = 0;
  delete mDir;
  mDir = 0;
}

void ItemMoveTest::testExpectedFail()
{
  QDir topDir( mDir->name() );

  QVERIFY( TestDataUtil::installFolder( QLatin1String( "maildir" ), topDir.path(), QLatin1String( "collection1" ) ) );
  QVERIFY( TestDataUtil::installFolder( QLatin1String( "mbox" ), topDir.path(), QLatin1String( "collection2" ) ) );

  KPIM::Maildir topLevelMd( topDir.path(), true );

  KPIM::Maildir md1 = topLevelMd.subFolder( QLatin1String( "collection1" ) );
  QSet<QString> entrySet1 = QSet<QString>::fromList( md1.entryList() );
  QCOMPARE( (int)entrySet1.count(), 4 );

  QFileInfo fileInfo2( topDir.path(), QLatin1String( "collection2" ) );
  MBox mbox2;
  QVERIFY( mbox2.load( fileInfo2.absoluteFilePath() ) );
  QList<MsgEntryInfo> entryList2 = mbox2.entryList();
  QCOMPARE( (int)entryList2.count(), 4 );

  QSet<qint64> entrySet2;
  Q_FOREACH( const MsgEntryInfo &entry, entryList2 ) {
    entrySet2 << entry.offset;
  }

  mStore->setPath( topDir.path() );

  // common variables
  FileStore::ItemMoveJob *job = 0;

  // test failure of moving from a non-existant collection
  Collection collection1;
  collection1.setName( QLatin1String( "collection1" ) );
  collection1.setRemoteId( QLatin1String( "collection1" ) );
  collection1.setParentCollection( mStore->topLevelCollection() );

  Collection collection3;
  collection3.setName( QLatin1String( "collection3" ) );
  collection3.setRemoteId( QLatin1String( "collection3" ) );
  collection3.setParentCollection( mStore->topLevelCollection() );

  Item item3;
  item3.setRemoteId( QLatin1String( "item3" ) );
  item3.setParentCollection( collection3 );

  job = mStore->moveItem( item3, collection1 );
  QVERIFY( !job->exec() );
  QCOMPARE( job->error(), (int)FileStore::Job::InvalidJobContext );

  QCOMPARE( QSet<QString>::fromList( md1.entryList() ), entrySet1 );

  // test failure of moving from maildir to non-existant collection
  Item item1;
  item1.setId( KRandom::random() );
  item1.setRemoteId( entrySet1.values().first() );
  item1.setParentCollection( collection1 );

  job = mStore->moveItem( item1, collection3 );
  QVERIFY( !job->exec() );
  QCOMPARE( job->error(), (int)FileStore::Job::InvalidJobContext );

  QCOMPARE( QSet<QString>::fromList( md1.entryList() ), entrySet1 );

  // test failure of moving from mbox to non-existant collection
  Collection collection2;
  collection2.setName( QLatin1String( "collection2" ) );
  collection2.setRemoteId( QLatin1String( "collection2" ) );
  collection2.setParentCollection( mStore->topLevelCollection() );

  Item item2;
  item2.setId( KRandom::random() );
  item2.setRemoteId( QLatin1String( "0" ) );
  item2.setParentCollection( collection2 );

  job = mStore->moveItem( item1, collection3 );
  QVERIFY( !job->exec() );
  QCOMPARE( job->error(), (int)FileStore::Job::InvalidJobContext );

  QVERIFY( mbox2.load( fileInfo2.absoluteFilePath() ) );
  QCOMPARE( mbox2.entryList(), entryList2 );

  // test failure of moving from maildir to top level collection
  job = mStore->moveItem( item1, mStore->topLevelCollection() );
  QVERIFY( !job->exec() );
  QCOMPARE( job->error(), (int)FileStore::Job::InvalidJobContext );

  QCOMPARE( QSet<QString>::fromList( md1.entryList() ), entrySet1 );

  // test failure of moving from mbox to top level collection
  job = mStore->moveItem( item1, mStore->topLevelCollection() );
  QVERIFY( !job->exec() );
  QCOMPARE( job->error(), (int)FileStore::Job::InvalidJobContext );

  QVERIFY( mbox2.load( fileInfo2.absoluteFilePath() ) );
  QCOMPARE( mbox2.entryList(), entryList2 );

  // test failure of moving a non-existant maildir entry
  QString remoteId1;
  do {
    remoteId1 = KRandom::randomString( 20 );
  } while ( entrySet1.contains( remoteId1 ) );

  item1.setRemoteId( remoteId1 );

  job = mStore->moveItem( item1, collection2 );
  QVERIFY( !job->exec() );
  QCOMPARE( job->error(), (int)FileStore::Job::InvalidJobContext );

  QCOMPARE( QSet<QString>::fromList( md1.entryList() ), entrySet1 );
  QVERIFY( mbox2.load( fileInfo2.absoluteFilePath() ) );
  QCOMPARE( mbox2.entryList(), entryList2 );

  // test failure of moving a non-existant mbox entry
  quint64 remoteId2;
  do {
    remoteId2 = KRandom::random();
  } while ( entrySet2.contains( remoteId2 ) );

  item2.setRemoteId( QString::number( remoteId2 ) );

  job = mStore->moveItem( item2, collection1 );
  QVERIFY( !job->exec() );
  QCOMPARE( job->error(), (int)FileStore::Job::InvalidJobContext );

  QCOMPARE( QSet<QString>::fromList( md1.entryList() ), entrySet1 );
  QVERIFY( mbox2.load( fileInfo2.absoluteFilePath() ) );
  QCOMPARE( mbox2.entryList(), entryList2 );
}

void ItemMoveTest::testMaildirItem()
{
  QDir topDir( mDir->name() );

  QVERIFY( TestDataUtil::installFolder( QLatin1String( "maildir" ), topDir.path(), QLatin1String( "collection1" ) ) );
  QVERIFY( TestDataUtil::installFolder( QLatin1String( "mbox" ), topDir.path(), QLatin1String( "collection2" ) ) );
  QVERIFY( TestDataUtil::installFolder( QLatin1String( "maildir" ), topDir.path(), QLatin1String( "collection5" ) ) );

  KPIM::Maildir topLevelMd( topDir.path(), true );

  KPIM::Maildir md1 = topLevelMd.subFolder( QLatin1String( "collection1" ) );
  QSet<QString> entrySet1 = QSet<QString>::fromList( md1.entryList() );
  QCOMPARE( (int)entrySet1.count(), 4 );

  QFileInfo fileInfo2( topDir.path(), QLatin1String( "collection2" ) );
  MBox mbox2;
  QVERIFY( mbox2.load( fileInfo2.absoluteFilePath() ) );
  QList<MsgEntryInfo> entryList2 = mbox2.entryList();
  QCOMPARE( (int)entryList2.count(), 4 );

  KPIM::Maildir md3( topLevelMd.addSubFolder( QLatin1String( "collection3" ) ), false );
  QVERIFY( md3.isValid() );
  QSet<QString> entrySet3 = QSet<QString>::fromList( md3.entryList() );
  QCOMPARE( (int)entrySet3.count(), 0 );

  QFileInfo fileInfo4( topDir.path(), QLatin1String( "collection4" ) );
  QFile file4( fileInfo4.absoluteFilePath() );
  QVERIFY( file4.open( QIODevice::WriteOnly ) );
  file4.close();
  fileInfo4.refresh();
  QVERIFY( fileInfo4.exists() );
  MBox mbox4;
  QVERIFY( mbox4.load( fileInfo4.absoluteFilePath() ) );
  QList<MsgEntryInfo> entryList4 = mbox4.entryList();
  QCOMPARE( (int)entryList4.count(), 0 );

  KPIM::Maildir md5 = topLevelMd.subFolder( QLatin1String( "collection5" ) );
  QSet<QString> entrySet5 = QSet<QString>::fromList( md5.entryList() );
  QCOMPARE( (int)entrySet5.count(), 4 );

  mStore->setPath( topDir.path() );

  // common variables
  FileStore::ItemMoveJob *job = 0;

  const QVariant colListVar = QVariant::fromValue<Collection::List>( Collection::List() );
  QVariant var;
  Collection::List collections;
  Item movedItem;
  QList<MsgEntryInfo> entryList;

  // test moving to an empty maildir
  Collection collection1;
  collection1.setName( QLatin1String( "collection1" ) );
  collection1.setRemoteId( QLatin1String( "collection1" ) );
  collection1.setParentCollection( mStore->topLevelCollection() );

  Collection collection3;
  collection3.setName( QLatin1String( "collection3" ) );
  collection3.setRemoteId( QLatin1String( "collection3" ) );
  collection3.setParentCollection( mStore->topLevelCollection() );

  Item item1;
  item1.setId( KRandom::random() );
  item1.setRemoteId( entrySet1.values().first() );
  item1.setParentCollection( collection1 );

  job = mStore->moveItem( item1, collection3 );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  movedItem = job->item();
  QCOMPARE( movedItem.id(), item1.id() );
  QCOMPARE( movedItem.parentCollection(), collection3 );

  entrySet3 << movedItem.remoteId();
  QCOMPARE( QSet<QString>::fromList( md3.entryList() ), entrySet3 );
  entrySet1.remove( item1.remoteId() );
  QCOMPARE( QSet<QString>::fromList( md1.entryList() ), entrySet1 );

  // check for index preservation
  var = job->property( "onDiskIndexInvalidated" );
  QVERIFY( var.isValid() );
  QCOMPARE( var.userType(), colListVar.userType() );

  collections = var.value<Collection::List>();
  QCOMPARE( (int)collections.count(), 1 );
  QCOMPARE( collections.first(), collection1 );

  // test moving to a non empty maildir
  item1.setRemoteId( entrySet1.values().first() );

  Collection collection5;
  collection5.setName( QLatin1String( "collection5" ) );
  collection5.setRemoteId( QLatin1String( "collection5" ) );
  collection5.setParentCollection( mStore->topLevelCollection() );

  job = mStore->moveItem( item1, collection5 );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  movedItem = job->item();
  QCOMPARE( movedItem.id(), item1.id() );
  QCOMPARE( movedItem.parentCollection(), collection5 );

  entrySet5 << movedItem.remoteId();
  QCOMPARE( QSet<QString>::fromList( md5.entryList() ), entrySet5 );
  entrySet1.remove( item1.remoteId() );
  QCOMPARE( QSet<QString>::fromList( md1.entryList() ), entrySet1 );

  // check for index preservation
  var = job->property( "onDiskIndexInvalidated" );
  QVERIFY( var.isValid() );
  QCOMPARE( var.userType(), colListVar.userType() );

  collections = var.value<Collection::List>();
  QCOMPARE( (int)collections.count(), 2 );
  QCOMPARE( collections, Collection::List() << collection1 << collection5 );

  // test moving to an empty mbox
  Collection collection4;
  collection4.setName( QLatin1String( "collection4" ) );
  collection4.setRemoteId( QLatin1String( "collection4" ) );
  collection4.setParentCollection( mStore->topLevelCollection() );

  item1.setRemoteId( entrySet1.values().first() );

  job = mStore->moveItem( item1, collection4 );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  movedItem = job->item();
  QCOMPARE( movedItem.id(), item1.id() );
  QCOMPARE( movedItem.parentCollection(), collection4 );

  QVERIFY( mbox4.load( mbox4.fileName() ) );
  entryList = mbox4.entryList();
  QCOMPARE( (int)entryList.count(), 1 );

  QCOMPARE( entryList.last().offset, movedItem.remoteId().toULongLong() );
  entrySet1.remove( item1.remoteId() );
  QCOMPARE( QSet<QString>::fromList( md1.entryList() ), entrySet1 );

  // check for index preservation
  var = job->property( "onDiskIndexInvalidated" );
  QVERIFY( var.isValid() );
  QCOMPARE( var.userType(), colListVar.userType() );

  collections = var.value<Collection::List>();
  QCOMPARE( (int)collections.count(), 1 );
  QCOMPARE( collections.first(), collection1 );

  // test moving to a non empty mbox
  item1.setRemoteId( entrySet1.values().first() );

  Collection collection2;
  collection2.setName( QLatin1String( "collection2" ) );
  collection2.setRemoteId( QLatin1String( "collection2" ) );
  collection2.setParentCollection( mStore->topLevelCollection() );

  job = mStore->moveItem( item1, collection2 );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  movedItem = job->item();
  QCOMPARE( movedItem.id(), item1.id() );
  QCOMPARE( movedItem.parentCollection(), collection2 );

  QVERIFY( mbox2.load( mbox2.fileName() ) );
  entryList = mbox2.entryList();
  QCOMPARE( (int)entryList.count(), 5 );

  QCOMPARE( entryList.last().offset, movedItem.remoteId().toULongLong() );
  entrySet1.remove( item1.remoteId() );
  QCOMPARE( QSet<QString>::fromList( md1.entryList() ), entrySet1 );

  // check for index preservation
  var = job->property( "onDiskIndexInvalidated" );
  QVERIFY( var.isValid() );
  QCOMPARE( var.userType(), colListVar.userType() );

  collections = var.value<Collection::List>();
  QCOMPARE( (int)collections.count(), 2 );
  QCOMPARE( collections, Collection::List() << collection1 << collection2 );
}

void ItemMoveTest::testMBoxItem()
{
  QDir topDir( mDir->name() );

  QVERIFY( TestDataUtil::installFolder( QLatin1String( "mbox" ), topDir.path(), QLatin1String( "collection1" ) ) );
  QVERIFY( TestDataUtil::installFolder( QLatin1String( "maildir" ), topDir.path(), QLatin1String( "collection2" ) ) );
  QVERIFY( TestDataUtil::installFolder( QLatin1String( "mbox" ), topDir.path(), QLatin1String( "collection5" ) ) );

  QFileInfo fileInfo1( topDir.path(), QLatin1String( "collection1" ) );
  MBox mbox1;
  QVERIFY( mbox1.load( fileInfo1.absoluteFilePath() ) );
  QList<MsgEntryInfo> entryList1 = mbox1.entryList();
  QCOMPARE( (int)entryList1.count(), 4 );

  KPIM::Maildir topLevelMd( topDir.path(), true );

  KPIM::Maildir md2 = topLevelMd.subFolder( QLatin1String( "collection2" ) );
  QSet<QString> entrySet2 = QSet<QString>::fromList( md2.entryList() );
  QCOMPARE( (int)entrySet2.count(), 4 );

  KPIM::Maildir md3( topLevelMd.addSubFolder( QLatin1String( "collection3" ) ), false );
  QVERIFY( md3.isValid() );
  QSet<QString> entrySet3 = QSet<QString>::fromList( md3.entryList() );
  QCOMPARE( (int)entrySet3.count(), 0 );

  QFileInfo fileInfo4( topDir.path(), QLatin1String( "collection4" ) );
  QFile file4( fileInfo4.absoluteFilePath() );
  QVERIFY( file4.open( QIODevice::WriteOnly ) );
  file4.close();
  fileInfo4.refresh();
  QVERIFY( fileInfo4.exists() );
  MBox mbox4;
  QVERIFY( mbox4.load( fileInfo4.absoluteFilePath() ) );
  QList<MsgEntryInfo> entryList4 = mbox4.entryList();
  QCOMPARE( (int)entryList4.count(), 0 );

  QFileInfo fileInfo5( topDir.path(), QLatin1String( "collection5" ) );
  MBox mbox5;
  QVERIFY( mbox5.load( fileInfo5.absoluteFilePath() ) );
  QList<MsgEntryInfo> entryList5 = mbox5.entryList();
  QCOMPARE( (int)entryList5.count(), 4 );

  mStore->setPath( topDir.path() );

  // common variables
  FileStore::ItemMoveJob *job = 0;
  FileStore::StoreCompactJob *compactStore = 0;

  const QVariant colListVar = QVariant::fromValue<Collection::List>( Collection::List() );
  QVariant var;
  Collection::List collections;
  Item movedItem;
  QList<MsgEntryInfo> entryList;
  Item::List items;

  // test moving to an empty maildir
  Collection collection1;
  collection1.setName( QLatin1String( "collection1" ) );
  collection1.setRemoteId( QLatin1String( "collection1" ) );
  collection1.setParentCollection( mStore->topLevelCollection() );

  Collection collection3;
  collection3.setName( QLatin1String( "collection3" ) );
  collection3.setRemoteId( QLatin1String( "collection3" ) );
  collection3.setParentCollection( mStore->topLevelCollection() );

  Item item1;
  item1.setId( KRandom::random() );
  item1.setRemoteId( QString::number( entryList1.first().offset ) );
  item1.setParentCollection( collection1 );

  job = mStore->moveItem( item1, collection3 );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  movedItem = job->item();
  QCOMPARE( movedItem.id(), item1.id() );
  QCOMPARE( movedItem.parentCollection(), collection3 );

  var = job->property( "compactStore" );
  QVERIFY( var.isValid() );
  QCOMPARE( var.type(), QVariant::Bool );
  QCOMPARE( var.toBool(), true );

  compactStore = mStore->compactStore();
  QVERIFY( compactStore->exec() );

  items = compactStore->changedItems();
  QCOMPARE( (int)items.count(), 3 );

  entrySet3 << movedItem.remoteId();
  QCOMPARE( QSet<QString>::fromList( md3.entryList() ), entrySet3 );

  entryList1.removeAt( 0 );
  entryList1[ 0 ].offset = changedOffset( items[ 0 ] );
  entryList1[ 1 ].offset = changedOffset( items[ 1 ] );
  entryList1[ 2 ].offset = changedOffset( items[ 2 ] );
  QVERIFY( mbox1.load( mbox1.fileName() ) );
  QCOMPARE( mbox1.entryList(), entryList1 );

  // test moving to a non empty mbox
  Collection collection5;
  collection5.setName( QLatin1String( "collection5" ) );
  collection5.setRemoteId( QLatin1String( "collection5" ) );
  collection5.setParentCollection( mStore->topLevelCollection() );

  job = mStore->moveItem( item1, collection5 );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  movedItem = job->item();
  QCOMPARE( movedItem.id(), item1.id() );
  QCOMPARE( movedItem.parentCollection(), collection5 );

  var = job->property( "compactStore" );
  QVERIFY( var.isValid() );
  QCOMPARE( var.type(), QVariant::Bool );
  QCOMPARE( var.toBool(), true );

  // check for index preservation
  var = job->property( "onDiskIndexInvalidated" );
  QVERIFY( var.isValid() );
  QCOMPARE( var.userType(), colListVar.userType() );

  collections = var.value<Collection::List>();
  QCOMPARE( (int)collections.count(), 2 );
  QCOMPARE( collections, Collection::List() << collection1 << collection5 );

  compactStore = mStore->compactStore();
  QVERIFY( compactStore->exec() );

  items = compactStore->changedItems();
  QCOMPARE( (int)items.count(), 2 );

  QVERIFY( mbox5.load( mbox5.fileName() ) );
  QCOMPARE( mbox5.entryList().count(), entryList5.count() + 1 );
  QCOMPARE( mbox5.entryList().last().offset, movedItem.remoteId().toULongLong() );

  entryList1.removeAt( 0 );
  entryList1[ 0 ].offset = changedOffset( items[ 0 ] );
  entryList1[ 1 ].offset = changedOffset( items[ 1 ] );
  QVERIFY( mbox1.load( mbox1.fileName() ) );
  QCOMPARE( mbox1.entryList(), entryList1 );

  // test moving to an empty mbox
  Collection collection4;
  collection4.setName( QLatin1String( "collection4" ) );
  collection4.setRemoteId( QLatin1String( "collection4" ) );
  collection4.setParentCollection( mStore->topLevelCollection() );

  job = mStore->moveItem( item1, collection4 );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  movedItem = job->item();
  QCOMPARE( movedItem.id(), item1.id() );
  QCOMPARE( movedItem.parentCollection(), collection4 );

  QVERIFY( mbox4.load( mbox4.fileName() ) );
  entryList = mbox4.entryList();
  QCOMPARE( (int)entryList.count(), 1 );

  QCOMPARE( entryList.last().offset, movedItem.remoteId().toULongLong() );

  var = job->property( "compactStore" );
  QVERIFY( var.isValid() );
  QCOMPARE( var.type(), QVariant::Bool );
  QCOMPARE( var.toBool(), true );

  // check for index preservation
  var = job->property( "onDiskIndexInvalidated" );
  QVERIFY( var.isValid() );
  QCOMPARE( var.userType(), colListVar.userType() );

  collections = var.value<Collection::List>();
  QCOMPARE( (int)collections.count(), 1 );
  QCOMPARE( collections.first(), collection1 );

  compactStore = mStore->compactStore();
  QVERIFY( compactStore->exec() );

  items = compactStore->changedItems();
  QCOMPARE( (int)items.count(), 1 );

  QCOMPARE( mbox4.entryList().count(), entryList4.count() + 1 );
  QCOMPARE( mbox4.entryList().last().offset, movedItem.remoteId().toULongLong() );

  entryList1.removeAt( 0 );
  entryList1[ 0 ].offset = changedOffset( items[ 0 ] );
  QVERIFY( mbox1.load( mbox1.fileName() ) );
  QCOMPARE( mbox1.entryList(), entryList1 );

  // test moving to a non empty maildir
  Collection collection2;
  collection2.setName( QLatin1String( "collection2" ) );
  collection2.setRemoteId( QLatin1String( "collection2" ) );
  collection2.setParentCollection( mStore->topLevelCollection() );

  job = mStore->moveItem( item1, collection2 );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  movedItem = job->item();
  QCOMPARE( movedItem.id(), item1.id() );
  QCOMPARE( movedItem.parentCollection(), collection2 );

  QSet<QString> entrySet = QSet<QString>::fromList( md2.entryList() );
  QCOMPARE( (int)entrySet.count(), 5 );

  QVERIFY( entrySet.contains( movedItem.remoteId() ) );

  var = job->property( "compactStore" );
  QVERIFY( var.isValid() );
  QCOMPARE( var.type(), QVariant::Bool );
  QCOMPARE( var.toBool(), true );

  // check for index preservation
  var = job->property( "onDiskIndexInvalidated" );
  QVERIFY( var.isValid() );
  QCOMPARE( var.userType(), colListVar.userType() );

  collections = var.value<Collection::List>();
  QCOMPARE( (int)collections.count(), 2 );
  QCOMPARE( collections.first(), collection1 );

  compactStore = mStore->compactStore();
  QVERIFY( compactStore->exec() );

  items = compactStore->changedItems();
  QCOMPARE( (int)items.count(), 0 );

  entryList1.removeAt( 0 );
  QVERIFY( mbox1.load( mbox1.fileName() ) );
  QCOMPARE( mbox1.entryList(), entryList1 );
}

QTEST_KDEMAIN( ItemMoveTest, NoGUI )

#include "itemmovetest.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
