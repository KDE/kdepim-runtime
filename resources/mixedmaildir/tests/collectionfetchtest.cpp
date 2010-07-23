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

#include "filestore/collectionfetchjob.h"

#include "libmaildir/maildir.h"

#include <kmime/kmime_message.h>

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

class CollectionFetchTest : public QObject
{
  Q_OBJECT

  public:
    CollectionFetchTest() : QObject(), mStore( 0 ), mDir( 0 ) {
      // for monitoring signals
      qRegisterMetaType<Akonadi::Collection::List>();
    }

    ~CollectionFetchTest() {
      delete mStore;
      delete mDir;
    }

  private:
    MixedMaildirStore *mStore;
    KTempDir *mDir;

  private Q_SLOTS:
    void init();
    void cleanup();
    void testEmptyDir();
    void testMixedTree();
};

void CollectionFetchTest::init()
{
  mStore = new MixedMaildirStore;

  mDir = new KTempDir;
  QVERIFY( mDir->exists() );
}

void CollectionFetchTest::cleanup()
{
  delete mStore;
  mStore = 0;
  delete mDir;
  mDir = 0;
}

void CollectionFetchTest::testEmptyDir()
{
  mStore->setPath( mDir->name() );

  FileStore::CollectionFetchJob *job = 0;
  QSignalSpy *spy = 0;
  Collection::List collections;

  // test base fetch of top level collection
  job = mStore->fetchCollections( mStore->topLevelCollection(), FileStore::CollectionFetchJob::Base );

  spy = new QSignalSpy( job, SIGNAL( collectionsReceived( const Akonadi::Collection::List& ) ) );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );
  QCOMPARE( spy->count(), 1 );

  collections = collectionsFromSpy( spy );
  QCOMPARE( collections.count(), 1 );
  QCOMPARE( collections.first(), mStore->topLevelCollection() );
  QCOMPARE( job->collections(), collections );

  // test first level fetch of top level collection
  job = mStore->fetchCollections( mStore->topLevelCollection(), FileStore::CollectionFetchJob::FirstLevel );

  spy = new QSignalSpy( job, SIGNAL( collectionsReceived( const Akonadi::Collection::List& ) ) );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );
  QCOMPARE( spy->count(), 0 );

  collections = collectionsFromSpy( spy );
  QCOMPARE( collections.count(), 0 );
  QCOMPARE( job->collections(), collections );

  // test recursive fetch of top level collection
  job = mStore->fetchCollections( mStore->topLevelCollection(), FileStore::CollectionFetchJob::Recursive );

  spy = new QSignalSpy( job, SIGNAL( collectionsReceived( const Akonadi::Collection::List& ) ) );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );
  QCOMPARE( spy->count(), 0 );

  collections = collectionsFromSpy( spy );
  QCOMPARE( collections.count(), 0 );
  QCOMPARE( job->collections(), collections );

  // test fail of base fetching non existant collection
  Collection collection;
  collection.setName( QLatin1String( "collection" ) );
  collection.setRemoteId( QLatin1String( "collection" ) );
  collection.setParentCollection( mStore->topLevelCollection() );

  job = mStore->fetchCollections( collection, FileStore::CollectionFetchJob::Base );

  spy = new QSignalSpy( job, SIGNAL( collectionsReceived( const Akonadi::Collection::List& ) ) );

  QVERIFY( !job->exec() );
  QCOMPARE( job->error(), (int)FileStore::Job::InvalidJobContext );
  QCOMPARE( spy->count(), 0 );

  collections = collectionsFromSpy( spy );
  QCOMPARE( collections.count(), 0 );
  QCOMPARE( job->collections(), collections );

  // test fail of first level fetching non existant collection
  job = mStore->fetchCollections( collection, FileStore::CollectionFetchJob::FirstLevel );

  spy = new QSignalSpy( job, SIGNAL( collectionsReceived( const Akonadi::Collection::List& ) ) );

  QVERIFY( !job->exec() );
  QCOMPARE( job->error(), (int)FileStore::Job::InvalidJobContext );
  QCOMPARE( spy->count(), 0 );

  collections = collectionsFromSpy( spy );
  QCOMPARE( collections.count(), 0 );
  QCOMPARE( job->collections(), collections );

  // test fail of recursive fetching non existant collection
  job = mStore->fetchCollections( collection, FileStore::CollectionFetchJob::FirstLevel );

  spy = new QSignalSpy( job, SIGNAL( collectionsReceived( const Akonadi::Collection::List& ) ) );

  QVERIFY( !job->exec() );
  QCOMPARE( job->error(), (int)FileStore::Job::InvalidJobContext );
  QCOMPARE( spy->count(), 0 );

  collections = collectionsFromSpy( spy );
  QCOMPARE( collections.count(), 0 );
  QCOMPARE( job->collections(), collections );
}

void CollectionFetchTest::testMixedTree()
{
  QDir topDir( mDir->name() );

  KPIM::Maildir topLevelMd( mDir->name(), true );
  QVERIFY( topLevelMd.isValid() );

  KPIM::Maildir md1( topLevelMd.addSubFolder( "collection1" ), false );
  KPIM::Maildir md1_2( md1.addSubFolder( "collection1_2" ), false );
  KPIM::Maildir md1_2_1( md1_2.addSubFolder( "collection1_2_1" ), false );

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

  QSet<QString> firstLevelNames;
  firstLevelNames << md1.name() << md2.name() << fileInfo3.fileName() << fileInfo4.fileName();

  QSet<QString> secondLevelNames;
  secondLevelNames << md1_2.name() << md4_1.name()
                   << fileInfo1_1.fileName() << fileInfo4_2.fileName();

  mStore->setPath( mDir->name() );

  FileStore::CollectionFetchJob *job = 0;
  QSignalSpy *spy = 0;
  Collection::List collections;

  // test base fetch of top level collection
  job = mStore->fetchCollections( mStore->topLevelCollection(), FileStore::CollectionFetchJob::Base );

  spy = new QSignalSpy( job, SIGNAL( collectionsReceived( const Akonadi::Collection::List& ) ) );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );
  QCOMPARE( spy->count(), 1 );

  collections = collectionsFromSpy( spy );
  QCOMPARE( collections.count(), 1 );
  QCOMPARE( collections.first(), mStore->topLevelCollection() );
  QCOMPARE( job->collections(), collections );

  // test first level fetch of top level collection
  job = mStore->fetchCollections( mStore->topLevelCollection(), FileStore::CollectionFetchJob::FirstLevel );

  spy = new QSignalSpy( job, SIGNAL( collectionsReceived( const Akonadi::Collection::List& ) ) );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );
  QVERIFY( spy->count() > 0 );

  collections = collectionsFromSpy( spy );
  QCOMPARE( collections.count(), firstLevelNames.count() );
  QCOMPARE( job->collections(), collections );

  Q_FOREACH( const Collection &collection, collections ) {
    QVERIFY( !collection.remoteId().isEmpty() );
    QCOMPARE( collection.remoteId(), collection.name() );
    QCOMPARE( collection.contentMimeTypes(), QStringList() << Collection::mimeType() << KMime::Message::mimeType() );

    QCOMPARE( collection.rights(), Collection::CanCreateItem |
                                   Collection::CanChangeItem |
                                   Collection::CanDeleteItem |
                                   Collection::CanCreateCollection |
                                   Collection::CanChangeCollection |
                                   Collection::CanDeleteCollection );

    QCOMPARE( collection.parentCollection(), mStore->topLevelCollection() );
    QVERIFY( firstLevelNames.contains( collection.name() ) );
  }

  // test recursive fetch of top level collection
  job = mStore->fetchCollections( mStore->topLevelCollection(), FileStore::CollectionFetchJob::Recursive );

  spy = new QSignalSpy( job, SIGNAL( collectionsReceived( const Akonadi::Collection::List& ) ) );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );
  QVERIFY( spy->count() > 0 );

  collections = collectionsFromSpy( spy );
  QCOMPARE( collections.count(), firstLevelNames.count() + secondLevelNames.count() + 1 );
  QCOMPARE( job->collections(), collections );

  Q_FOREACH( const Collection &collection, collections ) {
    QVERIFY( !collection.remoteId().isEmpty() );
    QCOMPARE( collection.remoteId(), collection.name() );
    QCOMPARE( collection.contentMimeTypes(), QStringList() << Collection::mimeType() << KMime::Message::mimeType() );

    QCOMPARE( collection.rights(), Collection::CanCreateItem |
                                   Collection::CanChangeItem |
                                   Collection::CanDeleteItem |
                                   Collection::CanCreateCollection |
                                   Collection::CanChangeCollection |
                                   Collection::CanDeleteCollection );

    if ( firstLevelNames.contains( collection.name() ) ) {
      QCOMPARE( collection.parentCollection(), mStore->topLevelCollection() );
    } else if ( secondLevelNames.contains( collection.name() ) ) {
      QVERIFY( firstLevelNames.contains( collection.parentCollection().name() ) );
      QCOMPARE( collection.parentCollection().parentCollection(), mStore->topLevelCollection() );
    } else {
      QCOMPARE( collection.name(), md1_2_1.name() );
      QCOMPARE( collection.parentCollection().name(), md1_2.name() );
      QCOMPARE( collection.parentCollection().parentCollection().parentCollection(),
                mStore->topLevelCollection() );
    }
  }
}

QTEST_KDEMAIN( CollectionFetchTest, NoGUI )

#include "collectionfetchtest.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
