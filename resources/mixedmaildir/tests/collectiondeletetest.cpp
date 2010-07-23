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

#include "filestore/collectiondeletejob.h"

#include "libmaildir/maildir.h"

#include <KTempDir>

#include <qtest_kde.h>

using namespace Akonadi;

class CollectionDeleteTest : public QObject
{
  Q_OBJECT

  public:
    CollectionDeleteTest() : QObject(), mStore( 0 ), mDir( 0 ) {}
    ~CollectionDeleteTest() {
      delete mStore;
      delete mDir;
    }

  private:
    MixedMaildirStore *mStore;
    KTempDir *mDir;

  private Q_SLOTS:
    void init();
    void cleanup();
    void testNonExisting();
    void testLeaves();
    void testSubTrees();
};

void CollectionDeleteTest::init()
{
  mStore = new MixedMaildirStore;

  mDir = new KTempDir;
  QVERIFY( mDir->exists() );
}

void CollectionDeleteTest::cleanup()
{
  delete mStore;
  mStore = 0;
  delete mDir;
  mDir = 0;
}

void CollectionDeleteTest::testNonExisting()
{
  KPIM::Maildir topLevelMd( mDir->name(), true );
  QVERIFY( topLevelMd.isValid() );

  KPIM::Maildir md1( topLevelMd.addSubFolder( "collection1" ), false );
  KPIM::Maildir md1_2( md1.addSubFolder( "collection1_2" ), false );

  KPIM::Maildir md2( topLevelMd.addSubFolder( "collection2" ), false );

  // simulate mbox
  QFileInfo fileInfo1( mDir->name(), QLatin1String( "collection3" ));
  QFile file1( fileInfo1.absoluteFilePath() );
  file1.open( QIODevice::WriteOnly );
  file1.close();
  QVERIFY( fileInfo1.exists() );

  // simulate mbox with empty subtree
  QFileInfo fileInfo2( mDir->name(), QLatin1String( "collection4" ));
  QFile file2( fileInfo2.absoluteFilePath() );
  file2.open( QIODevice::WriteOnly );
  file2.close();
  QVERIFY( fileInfo2.exists() );

  QFileInfo subDirInfo2( KPIM::Maildir::subDirPathForFolderPath( fileInfo2.absoluteFilePath() ) );
  QDir topDir( mDir->name() );
  QVERIFY( topDir.mkpath( subDirInfo2.absoluteFilePath() ) );

  mStore->setPath( mDir->name() );

  FileStore::CollectionDeleteJob *job = 0;

  // test fail of deleting first level collection
  Collection collection5;
  collection5.setName( QLatin1String( "collection5" ) );
  collection5.setRemoteId( QLatin1String( "collection5" ) );
  collection5.setParentCollection( mStore->topLevelCollection() );
  job = mStore->deleteCollection( collection5 );
  QVERIFY( job != 0 );

  QVERIFY( !job->exec() );
  QCOMPARE( job->error(), (int)FileStore::Job::InvalidJobContext );

  QCOMPARE( topLevelMd.subFolderList(), QStringList() << QLatin1String( "collection1" ) << QLatin1String( "collection2" ));
  QVERIFY( fileInfo1.exists() );

  // test fail of deleting second level collection in maildir leaf parent
  Collection collection2;
  collection2.setName( QLatin1String( "collection2" ) );
  collection2.setRemoteId( QLatin1String( "collection2" ) );
  collection2.setParentCollection( mStore->topLevelCollection() );

  Collection collection2_1;
  collection2_1.setName( QLatin1String( "collection2_1" ) );
  collection2_1.setRemoteId( QLatin1String( "collection2_1" ) );
  collection2_1.setParentCollection( collection2 );
  job = mStore->deleteCollection( collection2_1 );
  QVERIFY( job != 0 );

  QVERIFY( !job->exec() );
  QCOMPARE( job->error(), (int)FileStore::Job::InvalidJobContext );

  QCOMPARE( topLevelMd.subFolderList(), QStringList() << QLatin1String( "collection1" ) << QLatin1String( "collection2" ) );

  // test fail of deleting second level collection in maildir parent with subtree
  Collection collection1;
  collection1.setName( QLatin1String( "collection1" ) );
  collection1.setRemoteId( QLatin1String( "collection1" ) );
  collection1.setParentCollection( mStore->topLevelCollection() );

  Collection collection1_1;
  collection1_1.setName( QLatin1String( "collection1_1" ) );
  collection1_1.setRemoteId( QLatin1String( "collection1_1" ) );
  collection1_1.setParentCollection( collection1 );
  job = mStore->deleteCollection( collection1_1 );
  QVERIFY( job != 0 );

  QVERIFY( !job->exec() );
  QCOMPARE( job->error(), (int)FileStore::Job::InvalidJobContext );

  QCOMPARE( topLevelMd.subFolderList(), QStringList() << QLatin1String( "collection1" ) << QLatin1String( "collection2" ) );
  QCOMPARE( md1.subFolderList(), QStringList() << QLatin1String( "collection1_2" ) );

  // test fail of deleting second level collection in mbox leaf parent
  Collection collection3;
  collection3.setName( QLatin1String( "collection3" ) );
  collection3.setRemoteId( QLatin1String( "collection3" ) );
  collection3.setParentCollection( mStore->topLevelCollection() );

  Collection collection3_1;
  collection3_1.setName( QLatin1String( "collection3_1" ) );
  collection3_1.setRemoteId( QLatin1String( "collection3_1" ) );
  collection3_1.setParentCollection( collection3 );
  job = mStore->deleteCollection( collection3_1 );
  QVERIFY( job != 0 );

  QVERIFY( !job->exec() );
  QCOMPARE( job->error(), (int)FileStore::Job::InvalidJobContext );

  QVERIFY( fileInfo1.exists() );

  // test fail of deleting second level collection in mbox parent with subtree
  Collection collection4;
  collection4.setName( QLatin1String( "collection4" ) );
  collection4.setRemoteId( QLatin1String( "collection4" ) );
  collection4.setParentCollection( mStore->topLevelCollection() );

  Collection collection4_1;
  collection4_1.setName( QLatin1String( "collection4_1" ) );
  collection4_1.setRemoteId( QLatin1String( "collection4_1" ) );
  collection4_1.setParentCollection( collection4 );
  job = mStore->deleteCollection( collection4_1 );
  QVERIFY( job != 0 );

  QVERIFY( !job->exec() );
  QCOMPARE( job->error(), (int)FileStore::Job::InvalidJobContext );

  QVERIFY( fileInfo2.exists() );
  QVERIFY( subDirInfo2.exists() );

  // test fail of deleting second level collection with non existant parent
  Collection collection5_1;
  collection5_1.setName( QLatin1String( "collection5_1" ) );
  collection5_1.setRemoteId( QLatin1String( "collection5_1" ) );
  collection5_1.setParentCollection( collection5 );
  job = mStore->deleteCollection( collection5_1 );
  QVERIFY( job != 0 );

  QVERIFY( !job->exec() );
  QCOMPARE( job->error(), (int)FileStore::Job::InvalidJobContext );

  QCOMPARE( topLevelMd.subFolderList(), QStringList() << QLatin1String( "collection1" ) << QLatin1String( "collection2" ) );
  QVERIFY( fileInfo1.exists() );
  QCOMPARE( md1.subFolderList(), QStringList() << QLatin1String( "collection1_2" ) );
}

void CollectionDeleteTest::testLeaves()
{
  KPIM::Maildir topLevelMd( mDir->name(), true );
  QVERIFY( topLevelMd.isValid() );

  QDir topDir( mDir->name() );

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
  QFileInfo fileInfo3( mDir->name(), QLatin1String( "collection3" ));
  QFile file3( fileInfo3.absoluteFilePath() );
  file3.open( QIODevice::WriteOnly );
  file3.close();
  QVERIFY( fileInfo3.exists() );

  // simulate first level mbox with subtree
  QFileInfo fileInfo4( mDir->name(), QLatin1String( "collection4" ));
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

  mStore->setPath( mDir->name() );

  FileStore::CollectionDeleteJob *job = 0;

  // test second level leaves in maildir parent
  Collection collection1;
  collection1.setName( QLatin1String( "collection1" ) );
  collection1.setRemoteId( QLatin1String( "collection1" ) );
  collection1.setParentCollection( mStore->topLevelCollection() );

  Collection collection1_1;
  collection1_1.setName( QLatin1String( "collection1_1" ) );
  collection1_1.setRemoteId( QLatin1String( "collection1_1" ) );
  collection1_1.setParentCollection( collection1 );
  job = mStore->deleteCollection( collection1_1 );
  QVERIFY( job != 0 );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  fileInfo1_1.refresh();
  QVERIFY( !fileInfo1_1.exists() );

  Collection collection1_2;
  collection1_2.setName( QLatin1String( "collection1_2" ) );
  collection1_2.setRemoteId( QLatin1String( "collection1_2" ) );
  collection1_2.setParentCollection( collection1 );
  job = mStore->deleteCollection( collection1_2 );
  QVERIFY( job != 0 );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  QVERIFY( !md1_2.isValid() );
  QCOMPARE( md1.subFolderList(), QStringList() );

  // test second level leaves in mbox parent
  Collection collection4;
  collection4.setName( QLatin1String( "collection4" ) );
  collection4.setRemoteId( QLatin1String( "collection4" ) );
  collection4.setParentCollection( mStore->topLevelCollection() );

  Collection collection4_1;
  collection4_1.setName( QLatin1String( "collection4_1" ) );
  collection4_1.setRemoteId( QLatin1String( "collection4_1" ) );
  collection4_1.setParentCollection( collection4 );
  job = mStore->deleteCollection( collection4_1 );
  QVERIFY( job != 0 );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  QVERIFY( !md4_1.isValid() );
  QCOMPARE( md4.subFolderList(), QStringList() );

  Collection collection4_2;
  collection4_2.setName( QLatin1String( "collection4_2" ) );
  collection4_2.setRemoteId( QLatin1String( "collection4_2" ) );
  collection4_2.setParentCollection( collection4 );
  job = mStore->deleteCollection( collection4_2 );
  QVERIFY( job != 0 );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  fileInfo4_2.refresh();
  QVERIFY( !fileInfo4_2.exists() );

  // test deleting of first level leaves
  Collection collection2;
  collection2.setName( QLatin1String( "collection2" ) );
  collection2.setRemoteId( QLatin1String( "collection2" ) );
  collection2.setParentCollection( mStore->topLevelCollection() );

  job = mStore->deleteCollection( collection2 );
  QVERIFY( job != 0 );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  QVERIFY( !md2.isValid() );
  QCOMPARE( topLevelMd.subFolderList(), QStringList() << QLatin1String( "collection1" ) );

  Collection collection3;
  collection3.setName( QLatin1String( "collection3" ) );
  collection3.setRemoteId( QLatin1String( "collection3" ) );
  collection3.setParentCollection( mStore->topLevelCollection() );

  job = mStore->deleteCollection( collection3 );
  QVERIFY( job != 0 );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  fileInfo3.refresh();
  QVERIFY( !fileInfo3.exists() );

  // test deleting of first level leaves with empty subtrees
  QFileInfo subDirInfo1( KPIM::Maildir::subDirPathForFolderPath( md1.path() ) );
  QVERIFY( subDirInfo1.exists() );

  job = mStore->deleteCollection( collection1 );
  QVERIFY( job != 0 );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  QVERIFY( !md1.isValid() );
  subDirInfo1.refresh();
  QVERIFY( !subDirInfo1.exists() );
  QCOMPARE( topLevelMd.subFolderList(), QStringList() );

  job = mStore->deleteCollection( collection4 );
  QVERIFY( job != 0 );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  fileInfo4.refresh();
  QVERIFY( !fileInfo4.exists() );
  subDirInfo4.refresh();
  QVERIFY( !subDirInfo4.exists() );
}

void CollectionDeleteTest::testSubTrees()
{
  KPIM::Maildir topLevelMd( mDir->name(), true );
  QVERIFY( topLevelMd.isValid() );

  QDir topDir( mDir->name() );

  KPIM::Maildir md1( topLevelMd.addSubFolder( "collection1" ), false );
  KPIM::Maildir md1_2( md1.addSubFolder( "collection1_2" ), false );

  // simulate second level mbox in maildir parent
  QFileInfo fileInfo1_1( KPIM::Maildir::subDirPathForFolderPath( md1.path() ),
                         QLatin1String( "collection1_1" ));
  QFile file1_1( fileInfo1_1.absoluteFilePath() );
  file1_1.open( QIODevice::WriteOnly );
  file1_1.close();
  QVERIFY( fileInfo1_1.exists() );

  // simulate first level mbox with subtree
  QFileInfo fileInfo2( mDir->name(), QLatin1String( "collection2" ));
  QFile file2( fileInfo2.absoluteFilePath() );
  file2.open( QIODevice::WriteOnly );
  file2.close();
  QVERIFY( fileInfo2.exists() );

  QFileInfo subDirInfo2( KPIM::Maildir::subDirPathForFolderPath( fileInfo2.absoluteFilePath() ) );
  QVERIFY( topDir.mkpath( subDirInfo2.absoluteFilePath() ) );

  KPIM::Maildir md2( subDirInfo2.absoluteFilePath(), true );
  KPIM::Maildir md2_1( md2.addSubFolder( "collection2_1" ), false );

  // simulate second level mbox in mbox parent
  QFileInfo fileInfo2_2( subDirInfo2.absoluteFilePath(),
                         QLatin1String( "collection2_2" ));
  QFile file2_2( fileInfo2_2.absoluteFilePath() );
  file2_2.open( QIODevice::WriteOnly );
  file2_2.close();
  QVERIFY( fileInfo2_2.exists() );

  mStore->setPath( mDir->name() );

  FileStore::CollectionDeleteJob *job = 0;

  // test deleting maildir subtree
  QFileInfo subDirInfo1( KPIM::Maildir::subDirPathForFolderPath( md1.path() ) );
  QVERIFY( subDirInfo1.exists() );

  Collection collection1;
  collection1.setName( QLatin1String( "collection1" ) );
  collection1.setRemoteId( QLatin1String( "collection1" ) );
  collection1.setParentCollection( mStore->topLevelCollection() );

  job = mStore->deleteCollection( collection1 );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  QVERIFY( !md1.isValid() );
  QVERIFY( !md1_2.isValid() );
  fileInfo1_1.refresh();
  QVERIFY( !fileInfo1_1.exists() );

  // test deleting mbox subtree
  Collection collection2;
  collection2.setName( QLatin1String( "collection2" ) );
  collection2.setRemoteId( QLatin1String( "collection2" ) );
  collection2.setParentCollection( mStore->topLevelCollection() );

  job = mStore->deleteCollection( collection2 );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  fileInfo2.refresh();
  QVERIFY( !fileInfo2.exists() );
  QVERIFY( !md2_1.isValid() );
  fileInfo2_2.refresh();
  QVERIFY( !fileInfo2_2.exists() );
  QVERIFY( !subDirInfo2.exists() );
}

QTEST_KDEMAIN( CollectionDeleteTest, NoGUI )

#include "collectiondeletetest.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
