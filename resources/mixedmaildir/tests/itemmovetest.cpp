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

  QCOMPARE( mbox2.entryList(), entryList2 );
}

QTEST_KDEMAIN( ItemMoveTest, NoGUI )

#include "itemmovetest.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
