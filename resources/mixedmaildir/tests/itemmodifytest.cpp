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

#include "filestore/itemfetchjob.h"
#include "filestore/itemmodifyjob.h"
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

class ItemModifyTest : public QObject
{
  Q_OBJECT

  public:
    ItemModifyTest()
      : QObject(), mStore( 0 ), mDir( 0 ) {}

    ~ItemModifyTest() {
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
    void testIgnorePayload();
    void testModify();
};

void ItemModifyTest::init()
{
  mStore = new MixedMaildirStore;

  mDir = new KTempDir;
  QVERIFY( mDir->exists() );
}

void ItemModifyTest::cleanup()
{
  delete mStore;
  mStore = 0;
  delete mDir;
  mDir = 0;
}

void ItemModifyTest::testExpectedFail()
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
  FileStore::ItemModifyJob *job = 0;

  // test failure of modifying a non-existant maildir entry
  Collection collection1;
  collection1.setName( QLatin1String( "collection1" ) );
  collection1.setRemoteId( QLatin1String( "collection1" ) );
  collection1.setParentCollection( mStore->topLevelCollection() );

  QString remoteId1;
  do {
    remoteId1 = KRandom::randomString( 20 );
  } while ( entrySet1.contains( remoteId1 ) );

  KMime::Message::Ptr msgPtr( new KMime::Message );

  Item item1;
  item1.setMimeType( KMime::Message::mimeType() );
  item1.setRemoteId( remoteId1 );
  item1.setParentCollection( collection1 );
  item1.setPayload<KMime::Message::Ptr>( msgPtr );

  job = mStore->modifyItem( item1 );

  QVERIFY( !job->exec() );
  QCOMPARE( job->error(), (int)FileStore::Job::InvalidJobContext );

  QSet<QString> entrySet = QSet<QString>::fromList( md1.entryList() );
  QCOMPARE( entrySet, entrySet1 );

  // test failure of modifying a non-existant mbox entry
  Collection collection2;
  collection2.setName( QLatin1String( "collection2" ) );
  collection2.setRemoteId( QLatin1String( "collection2" ) );
  collection2.setParentCollection( mStore->topLevelCollection() );

  qint64 remoteId2;
  do {
    remoteId2 = KRandom::random();
  } while ( entrySet2.contains( remoteId2 ) );

  Item item2;
  item2.setMimeType( KMime::Message::mimeType() );
  item2.setRemoteId( QString::number( remoteId2 ) );
  item2.setParentCollection( collection2 );
  item2.setPayload<KMime::Message::Ptr>( msgPtr );

  job = mStore->modifyItem( item2 );

  QVERIFY( !job->exec() );
  QCOMPARE( job->error(), (int)FileStore::Job::InvalidJobContext );

  QVERIFY( mbox2.load( mbox2.fileName() ) );
  QList<MsgEntryInfo> entryList = mbox2.entryList();
  QCOMPARE( entryList, entryList2 );
}

void ItemModifyTest::testIgnorePayload()
{
  QDir topDir( mDir->name() );

  QVERIFY( TestDataUtil::installFolder( QLatin1String( "maildir" ), topDir.path(), QLatin1String( "collection1" ) ) );
  QVERIFY( TestDataUtil::installFolder( QLatin1String( "mbox" ), topDir.path(), QLatin1String( "collection2" ) ) );

  KPIM::Maildir topLevelMd( topDir.path(), true );

  KPIM::Maildir md1 = topLevelMd.subFolder( QLatin1String( "collection1" ) );
  QStringList entryList1 = md1.entryList();
  QCOMPARE( (int)entryList1.count(), 4 );

  QFileInfo fileInfo2( topDir.path(), QLatin1String( "collection2" ) );
  MBox mbox2;
  QVERIFY( mbox2.load( fileInfo2.absoluteFilePath() ) );
  QList<MsgEntryInfo> entryList2 = mbox2.entryList();
  QCOMPARE( (int)entryList2.count(), 4 );

  mStore->setPath( topDir.path() );

  // common variables
  FileStore::ItemModifyJob *job = 0;

  // test failure of modifying a non-existant maildir entry
  Collection collection1;
  collection1.setName( QLatin1String( "collection1" ) );
  collection1.setRemoteId( QLatin1String( "collection1" ) );
  collection1.setParentCollection( mStore->topLevelCollection() );

  const QByteArray data1 = md1.readEntry( entryList1.first() );

  KMime::Message::Ptr msgPtr( new KMime::Message );
  msgPtr->subject()->from7BitString( "Modify Test" );
  msgPtr->assemble();

  Item item1;
  item1.setMimeType( KMime::Message::mimeType() );
  item1.setRemoteId( entryList1.first() );
  item1.setParentCollection( collection1 );
  item1.setPayload<KMime::Message::Ptr>( msgPtr );

  job = mStore->modifyItem( item1 );
  job->setIgnorePayload( true );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  QCOMPARE( md1.entryList(), entryList1 );
  QCOMPARE( md1.readEntry( entryList1.first() ), data1 );

  // test failure of modifying a non-existant mbox entry
  Collection collection2;
  collection2.setName( QLatin1String( "collection2" ) );
  collection2.setRemoteId( QLatin1String( "collection2" ) );
  collection2.setParentCollection( mStore->topLevelCollection() );

  const QByteArray data2 = mbox2.readRawEntry( 0 );

  Item item2;
  item2.setMimeType( KMime::Message::mimeType() );
  item2.setRemoteId( QLatin1String( "0" ) );
  item2.setParentCollection( collection2 );
  item2.setPayload<KMime::Message::Ptr>( msgPtr );

  job = mStore->modifyItem( item2 );
  job->setIgnorePayload( true );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  QVERIFY( mbox2.load( mbox2.fileName() ) );
  QCOMPARE( mbox2.entryList(), entryList2 );
  QCOMPARE( mbox2.readRawEntry( 0 ), data2 );
}

void ItemModifyTest::testModify()
{
  QDir topDir( mDir->name() );

  QVERIFY( TestDataUtil::installFolder( QLatin1String( "maildir" ), topDir.path(), QLatin1String( "collection1" ) ) );
  QVERIFY( TestDataUtil::installFolder( QLatin1String( "mbox" ), topDir.path(), QLatin1String( "collection2" ) ) );

  KPIM::Maildir topLevelMd( topDir.path(), true );

  KPIM::Maildir md1 = topLevelMd.subFolder( QLatin1String( "collection1" ) );
  QStringList entryList1 = md1.entryList();
  QCOMPARE( (int)entryList1.count(), 4 );

  QFileInfo fileInfo2( topDir.path(), QLatin1String( "collection2" ) );
  MBox mbox2;
  QVERIFY( mbox2.load( fileInfo2.absoluteFilePath() ) );
  QList<MsgEntryInfo> entryList2 = mbox2.entryList();
  QCOMPARE( (int)entryList2.count(), 4 );

  mStore->setPath( topDir.path() );

  // common variables
  FileStore::ItemModifyJob *job = 0;

  // test failure of modifying a non-existant maildir entry
  Collection collection1;
  collection1.setName( QLatin1String( "collection1" ) );
  collection1.setRemoteId( QLatin1String( "collection1" ) );
  collection1.setParentCollection( mStore->topLevelCollection() );

  const QByteArray data1 = md1.readEntry( entryList1.first() );

  KMime::Message::Ptr msgPtr( new KMime::Message );
  msgPtr->setContent( KMime::CRLFtoLF( data1 ) );
  msgPtr->subject()->from7BitString( "Modify Test" );
  msgPtr->assemble();

  Item item1;
  item1.setMimeType( KMime::Message::mimeType() );
  item1.setRemoteId( entryList1.first() );
  item1.setParentCollection( collection1 );
  item1.setPayload<KMime::Message::Ptr>( msgPtr );

  job = mStore->modifyItem( item1 );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  QCOMPARE( md1.entryList(), entryList1 );

  QCOMPARE( md1.readEntry( entryList1.first() ), msgPtr->encodedContent() );

  // test failure of modifying a non-existant mbox entry
  Collection collection2;
  collection2.setName( QLatin1String( "collection2" ) );
  collection2.setRemoteId( QLatin1String( "collection2" ) );
  collection2.setParentCollection( mStore->topLevelCollection() );

  const QByteArray data2 = mbox2.readRawEntry( 0 );

  msgPtr->setContent( KMime::CRLFtoLF( data2 ) );
  msgPtr->subject()->from7BitString( "Modify Test" );
  msgPtr->assemble();

  Item item2;
  item2.setMimeType( KMime::Message::mimeType() );
  item2.setRemoteId( QLatin1String( "0" ) );
  item2.setParentCollection( collection2 );
  item2.setPayload<KMime::Message::Ptr>( msgPtr );

  job = mStore->modifyItem( item2 );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  Item item = job->item();

  QVERIFY( mbox2.load( mbox2.fileName() ) );
  QList<MsgEntryInfo> entryList = mbox2.entryList();
  QCOMPARE( (int)entryList.count(), 5 ); // mbox file not purged yet
  QCOMPARE( entryList.last().offset, item.remoteId().toULongLong() );

  QVariant var = job->property( "compactStore" );
  QVERIFY( var.isValid() );
  QCOMPARE( var.type(), QVariant::Bool );
  QCOMPARE( var.toBool(), true );

  FileStore::ItemFetchJob *itemFetch = mStore->fetchItem( item2 );
  QVERIFY( !itemFetch->exec() ); // item at old offset gone

  FileStore::StoreCompactJob *storeCompact = mStore->compactStore();
  QVERIFY( storeCompact->exec() );

  QVERIFY( mbox2.load( mbox2.fileName() ) );
  entryList = mbox2.entryList();
  QCOMPARE( (int)entryList.count(), 4 );

  QCOMPARE( mbox2.readRawEntry( entryList.last().offset ), msgPtr->encodedContent() );
}

QTEST_KDEMAIN( ItemModifyTest, NoGUI )

#include "itemmodifytest.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
