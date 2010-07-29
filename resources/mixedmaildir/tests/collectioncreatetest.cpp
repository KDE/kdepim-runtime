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

#include "filestore/collectioncreatejob.h"

#include "libmaildir/maildir.h"

#include <kmime/kmime_message.h>

#include <KTempDir>

#include <qtest_kde.h>

using namespace Akonadi;

class CollectionCreateTest : public QObject
{
  Q_OBJECT

  public:
    CollectionCreateTest() : QObject(), mStore( 0 ), mDir( 0 ) {}
    ~CollectionCreateTest() {
      delete mStore;
      delete mDir;
    }

  private:
    MixedMaildirStore *mStore;
    KTempDir *mDir;

  private Q_SLOTS:
    void init();
    void cleanup();
    void testCollectionProperties();
    void testEmptyDir();
    void testMaildirTree();
    void testMixedTree();
};

void CollectionCreateTest::init()
{
  mStore = new MixedMaildirStore;

  mDir = new KTempDir;
  QVERIFY( mDir->exists() );
}

void CollectionCreateTest::cleanup()
{
  delete mStore;
  mStore = 0;
  delete mDir;
  mDir = 0;
}

void CollectionCreateTest::testCollectionProperties()
{
  mStore->setPath( mDir->name() );

  FileStore::CollectionCreateJob *job = 0;

  Collection collection1;
  collection1.setName( QLatin1String( "collection1" ) );
  job = mStore->createCollection( collection1, mStore->topLevelCollection() );
  QVERIFY( job != 0 );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  collection1 = job->collection();
  QCOMPARE( collection1.remoteId(), collection1.name() );

  QCOMPARE( collection1.contentMimeTypes(), QStringList() << Collection::mimeType() << KMime::Message::mimeType() );

  QCOMPARE( collection1.rights(), Collection::CanCreateItem |
                                  Collection::CanChangeItem |
                                  Collection::CanDeleteItem |
                                  Collection::CanCreateCollection |
                                  Collection::CanChangeCollection |
                                  Collection::CanDeleteCollection );
}

void CollectionCreateTest::testEmptyDir()
{
  mStore->setPath( mDir->name() );

  KPIM::Maildir topLevelMd( mStore->path(), true );

  FileStore::CollectionCreateJob *job = 0;

  // test creating first level collections
  Collection collection1;
  collection1.setName( QLatin1String( "collection1" ) );
  job = mStore->createCollection( collection1, mStore->topLevelCollection() );
  QVERIFY( job != 0 );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  collection1 = job->collection();
  QVERIFY( !collection1.remoteId().isEmpty() );
  QVERIFY( collection1.parentCollection() == mStore->topLevelCollection() );

  QCOMPARE( topLevelMd.subFolderList(), QStringList() << QLatin1String( "collection1" ) );
  KPIM::Maildir md1 = topLevelMd.subFolder( collection1.remoteId() );
  QVERIFY( md1.isValid() );

  Collection collection2;
  collection2.setName( QLatin1String( "collection2" ) );
  job = mStore->createCollection( collection2, mStore->topLevelCollection() );
  QVERIFY( job != 0 );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  collection2 = job->collection();
  QVERIFY( !collection2.remoteId().isEmpty() );
  QVERIFY( collection2.parentCollection() == mStore->topLevelCollection() );

  QCOMPARE( topLevelMd.subFolderList(), QStringList() << QLatin1String( "collection1" ) << QLatin1String( "collection2" ) );
  KPIM::Maildir md2 = topLevelMd.subFolder( collection2.remoteId() );
  QVERIFY( md2.isValid() );

  // test creating second level collections
  Collection collection1_1;
  collection1_1.setName( QLatin1String( "collection1_1" ) );
  job = mStore->createCollection( collection1_1, collection1 );
  QVERIFY( job != 0 );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  collection1_1 = job->collection();
  QVERIFY( !collection1_1.remoteId().isEmpty() );
  QVERIFY( collection1_1.parentCollection() == collection1 );

  QCOMPARE( md1.subFolderList(), QStringList() << QLatin1String( "collection1_1" ) );
  KPIM::Maildir md1_1 = md1.subFolder( collection1_1.remoteId() );
  QVERIFY( md1_1.isValid() );

  Collection collection1_2;
  collection1_2.setName( QLatin1String( "collection1_2" ) );
  job = mStore->createCollection( collection1_2, collection1 );
  QVERIFY( job != 0 );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  collection1_2 = job->collection();
  QVERIFY( !collection1_2.remoteId().isEmpty() );
  QVERIFY( collection1_2.parentCollection() == collection1 );

  QCOMPARE( md1.subFolderList(), QStringList() << QLatin1String( "collection1_1" ) << QLatin1String( "collection1_2" ) );
  KPIM::Maildir md1_2 = md1.subFolder( collection1_2.remoteId() );
  QVERIFY( md1_2.isValid() );

  QCOMPARE( md2.subFolderList(), QStringList() );
}

void CollectionCreateTest::testMaildirTree()
{
  KPIM::Maildir topLevelMd( mDir->name(), true );
  QVERIFY( topLevelMd.isValid() );

  KPIM::Maildir md1( topLevelMd.addSubFolder( "collection1" ), false );

  KPIM::Maildir md1_2( md1.addSubFolder( "collection1_2" ), false );

  mStore->setPath( mDir->name() );

  FileStore::CollectionCreateJob *job = 0;

  // test creating first level collections
  Collection collection1;
  collection1.setName( QLatin1String( "collection1" ) );
  job = mStore->createCollection( collection1, mStore->topLevelCollection() );
  QVERIFY( job != 0 );

  QVERIFY( job->exec() ); // works because it already exists
  QCOMPARE( job->error(), 0 );

  collection1 = job->collection();
  QVERIFY( !collection1.remoteId().isEmpty() );
  QVERIFY( collection1.parentCollection() == mStore->topLevelCollection() );

  Collection collection2;
  collection2.setName( QLatin1String( "collection2" ) );
  job = mStore->createCollection( collection2, mStore->topLevelCollection() );
  QVERIFY( job != 0 );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  collection2 = job->collection();
  QVERIFY( !collection2.remoteId().isEmpty() );
  QVERIFY( collection2.parentCollection() == mStore->topLevelCollection() );

  QCOMPARE( topLevelMd.subFolderList(), QStringList() << QLatin1String( "collection1" ) << QLatin1String( "collection2" ) );
  KPIM::Maildir md2 = topLevelMd.subFolder( collection2.remoteId() );
  QVERIFY( md2.isValid() );

  // test creating second level collections
  Collection collection1_1;
  collection1_1.setName( QLatin1String( "collection1_1" ) );
  job = mStore->createCollection( collection1_1, collection1 );
  QVERIFY( job != 0 );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  collection1_1 = job->collection();
  QVERIFY( !collection1_1.remoteId().isEmpty() );
  QCOMPARE( collection1_1.parentCollection().remoteId(), QLatin1String( "collection1" ) );

  QCOMPARE( md1.subFolderList(), QStringList() << QLatin1String( "collection1_1" ) << QLatin1String( "collection1_2" ) );
  KPIM::Maildir md1_1 = md1.subFolder( collection1_1.remoteId() );
  QVERIFY( md1_1.isValid() );

  Collection collection1_2;
  collection1_2.setName( QLatin1String( "collection1_2" ) );
  job = mStore->createCollection( collection1_2, collection1 );
  QVERIFY( job != 0 );

  QVERIFY( job->exec() ); // works because it already exists
  QCOMPARE( job->error(), 0 );

  collection1_2 = job->collection();
  QVERIFY( !collection1_2.remoteId().isEmpty() );
  QCOMPARE( collection1_2.parentCollection().remoteId(), QLatin1String( "collection1" ) );

  QCOMPARE( md2.subFolderList(), QStringList() );
}

void CollectionCreateTest::testMixedTree()
{
  KPIM::Maildir topLevelMd( mDir->name(), true );
  QVERIFY( topLevelMd.isValid() );

  // simulate a first level MBox
  QFileInfo fileInfo1( mDir->name(), QLatin1String( "collection1" ) );
  QFile file1( fileInfo1.absoluteFilePath() );
  file1.open( QIODevice::WriteOnly );
  file1.close();
  QVERIFY( fileInfo1.exists() );

  mStore->setPath( mDir->name() );

  FileStore::CollectionCreateJob *job = 0;

  // test creating first level collections
  Collection collection1;
  collection1.setName( QLatin1String( "collection1" ) );
  job = mStore->createCollection( collection1, mStore->topLevelCollection() );
  QVERIFY( job != 0 );

  QVERIFY( !job->exec() ); // fails, there is an MBox with that name
  QCOMPARE( job->error(), (int)FileStore::Job::InvalidJobContext );

  collection1 = job->collection();
  QVERIFY( collection1.remoteId().isEmpty() );

  collection1.setRemoteId( QLatin1String( "collection1" ) );
  collection1.setParentCollection( mStore->topLevelCollection() );

  Collection collection2;
  collection2.setName( QLatin1String( "collection2" ) );
  job = mStore->createCollection( collection2, mStore->topLevelCollection() );
  QVERIFY( job != 0 );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  collection2 = job->collection();
  QVERIFY( !collection2.remoteId().isEmpty() );
  QVERIFY( collection2.parentCollection() == mStore->topLevelCollection() );

  // mbox does not show up as a maildir subfolder
  QCOMPARE( topLevelMd.subFolderList(), QStringList() << QLatin1String( "collection2" ) );
  KPIM::Maildir md2 = topLevelMd.subFolder( collection2.remoteId() );
  QVERIFY( md2.isValid() );

  // test creating second level collections inside mbox
  Collection collection1_1;
  collection1_1.setName( QLatin1String( "collection1_1" ) );
  job = mStore->createCollection( collection1_1, collection1 );
  QVERIFY( job != 0 );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  collection1_1 = job->collection();
  QVERIFY( !collection1_1.remoteId().isEmpty() );
  QCOMPARE( collection1_1.parentCollection().remoteId(), QLatin1String( "collection1" ) );

  // treat the MBox subdir path like a top level maildir
  KPIM::Maildir md1( KPIM::Maildir::subDirPathForFolderPath( fileInfo1.absoluteFilePath() ), true );
  KPIM::Maildir md1_1 = md1.subFolder( collection1_1.remoteId() );
  QVERIFY( md1_1.isValid() );

  QCOMPARE( md1.subFolderList(), QStringList() << QLatin1String( "collection1_1" ) );
}

QTEST_KDEMAIN( CollectionCreateTest, NoGUI )

#include "collectioncreatetest.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
