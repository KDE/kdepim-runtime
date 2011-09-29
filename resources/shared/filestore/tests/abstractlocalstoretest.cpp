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

#include "abstractlocalstore.h"

#include "collectioncreatejob.h"
#include "collectiondeletejob.h"
#include "collectionfetchjob.h"
#include "collectionmodifyjob.h"
#include "collectionmovejob.h"
#include "itemcreatejob.h"
#include "itemdeletejob.h"
#include "itemfetchjob.h"
#include "itemmodifyjob.h"
#include "itemmovejob.h"
#include "sessionimpls_p.h"
#include "storecompactjob.h"

#include <KRandom>

#include <qtest_kde.h>

using namespace Akonadi;
using namespace Akonadi::FileStore;

class TestStore : public AbstractLocalStore
{
  Q_OBJECT

  public:
    TestStore() : mLastCheckedJob( 0 ), mLastProcessedJob( 0 ), mErrorCode( 0 ) {}

  public:
    mutable Job *mLastCheckedJob;
    Job *mLastProcessedJob;

    Collection mTopLevelCollection;

    int mErrorCode;
    QString mErrorText;

  protected:
    void processJob( Job *job );

  protected:
    void setTopLevelCollection( const Collection &collection )
    {
      mTopLevelCollection = collection;

      Collection modifiedCollection = collection;
      modifiedCollection.setContentMimeTypes( QStringList() << Collection::mimeType() );

      AbstractLocalStore::setTopLevelCollection( modifiedCollection );
    }

    void checkCollectionCreate( CollectionCreateJob *job, int &errorCode, QString &errorText ) const
    {
      mLastCheckedJob = job;
      errorCode = mErrorCode;
      errorText = mErrorText;
    }

    void checkCollectionDelete( CollectionDeleteJob *job, int &errorCode, QString &errorText ) const
    {
      mLastCheckedJob = job;
      errorCode = mErrorCode;
      errorText = mErrorText;
    }

    void checkCollectionFetch( Akonadi::FileStore::CollectionFetchJob *job, int &errorCode, QString &errorText ) const
    {
      mLastCheckedJob = job;
      errorCode = mErrorCode;
      errorText = mErrorText;
    }

    void checkCollectionModify( Akonadi::FileStore::CollectionModifyJob *job, int &errorCode, QString &errorText ) const
    {
      mLastCheckedJob = job;
      errorCode = mErrorCode;
      errorText = mErrorText;
    }

    void checkCollectionMove( Akonadi::FileStore::CollectionMoveJob *job, int &errorCode, QString &errorText ) const
    {
      mLastCheckedJob = job;
      errorCode = mErrorCode;
      errorText = mErrorText;
    }

    void checkItemCreate( Akonadi::FileStore::ItemCreateJob *job, int &errorCode, QString &errorText ) const
    {
      mLastCheckedJob = job;
      errorCode = mErrorCode;
      errorText = mErrorText;
    }

    void checkItemDelete( Akonadi::FileStore::ItemDeleteJob *job, int &errorCode, QString &errorText ) const
    {
      mLastCheckedJob = job;
      errorCode = mErrorCode;
      errorText = mErrorText;
    }

    void checkItemFetch( ItemFetchJob *job, int &errorCode, QString &errorText ) const
    {
      mLastCheckedJob = job;
      errorCode = mErrorCode;
      errorText = mErrorText;
    }

    void checkItemModify( Akonadi::FileStore::ItemModifyJob *job, int &errorCode, QString &errorText ) const
    {
      mLastCheckedJob = job;
      errorCode = mErrorCode;
      errorText = mErrorText;
    }

    void checkItemMove( ItemMoveJob *job, int &errorCode, QString &errorText ) const
    {
      mLastCheckedJob = job;
      errorCode = mErrorCode;
      errorText = mErrorText;
    }

    void checkStoreCompact( StoreCompactJob *job, int &errorCode, QString &errorText ) const
    {
      mLastCheckedJob = job;
      errorCode = mErrorCode;
      errorText = mErrorText;
    }
};

void TestStore::processJob( Job *job )
{
  mLastProcessedJob = job;

  QCOMPARE( currentJob(), job );
  QVERIFY( job->error() == 0 );

  if ( mErrorCode != 0 ) {
    notifyError( mErrorCode, mErrorText );
  }
}

class AbstractLocalStoreTest : public QObject
{
  Q_OBJECT

  public:
    AbstractLocalStoreTest() : QObject(), mStore( 0 ) {}
    ~AbstractLocalStoreTest() { delete mStore; }

  private:
    TestStore *mStore;

  private Q_SLOTS:
    void init();
    void testSetPath();
    void testCreateCollection();
    void testDeleteCollection();
    void testFetchCollection();
    void testModifyCollection();
    void testMoveCollection();
    void testFetchItems();
    void testFetchItem();
    void testCreateItem();
    void testDeleteItem();
    void testModifyItem();
    void testMoveItem();
    void testCompactStore();
};

void AbstractLocalStoreTest::init()
{
  delete mStore;
  mStore = new TestStore;
}

void AbstractLocalStoreTest::testSetPath()
{
  const QString file = KRandom::randomString( 10 );
  const QString path = QLatin1String( "/tmp/test/" ) + file;

  // check that setTopLevelCollection() has been called
  mStore->setPath( path );
  QCOMPARE( mStore->mTopLevelCollection.remoteId(), path );

  // check that the modified collection is the one returned by topLevelCollection()
  QVERIFY( mStore->mTopLevelCollection.contentMimeTypes().isEmpty() );
  QCOMPARE( mStore->topLevelCollection().remoteId(), path );
  QCOMPARE( mStore->topLevelCollection().contentMimeTypes(), QStringList() << Collection::mimeType() );
  QCOMPARE( mStore->topLevelCollection().name(), file );

  // check that calling with the same path again, does not call the template method
  mStore->mTopLevelCollection = Collection();
  mStore->setPath( path );
  QVERIFY( mStore->mTopLevelCollection.remoteId().isEmpty() );
  QCOMPARE( mStore->topLevelCollection().remoteId(), path );
  QCOMPARE( mStore->topLevelCollection().contentMimeTypes(), QStringList() << Collection::mimeType() );
  QCOMPARE( mStore->topLevelCollection().name(), file );

  // check that calling with a different path works like the first call
  const QString file2 = KRandom::randomString( 10 );
  const QString path2 = QLatin1String( "/tmp/test2/" ) + file2;

  mStore->setPath( path2 );
  QCOMPARE( mStore->mTopLevelCollection.remoteId(), path2 );
  QCOMPARE( mStore->topLevelCollection().remoteId(), path2 );
  QCOMPARE( mStore->topLevelCollection().contentMimeTypes(), QStringList() << Collection::mimeType() );
  QCOMPARE( mStore->topLevelCollection().name(), file2 );
}

void AbstractLocalStoreTest::testCreateCollection()
{
  CollectionCreateJob *job = 0;

  // test without setPath()
  job = mStore->createCollection( Collection(), Collection() );
  QVERIFY( job != 0 );
  QCOMPARE( mStore->mLastCheckedJob, job );
  QCOMPARE( job->error(), (int)Job::InvalidStoreState );
  QVERIFY( !job->errorText().isEmpty() );

  QVERIFY( !job->exec() );
  QVERIFY( mStore->mLastProcessedJob == 0 );

  // test with path but with invalid collections
  mStore->setPath( QLatin1String( "/tmp/test" ) );
  job = mStore->createCollection( Collection(), Collection() );
  QVERIFY( job != 0 );
  QCOMPARE( mStore->mLastCheckedJob, job );
  QCOMPARE( job->error(), (int)Job::InvalidJobContext );
  QVERIFY( !job->errorText().isEmpty() );

  QVERIFY( !job->exec() );
  QVERIFY( mStore->mLastProcessedJob == 0 );

  // test with potentially valid collection (has remoteId), but invalid target parent
  Collection collection;
  collection.setRemoteId( QLatin1String( "/tmp/test/foo" ) );
  job = mStore->createCollection( collection, Collection() );
  QVERIFY( job != 0 );
  QCOMPARE( mStore->mLastCheckedJob, job );
  QCOMPARE( job->error(), (int)Job::InvalidJobContext );
  QVERIFY( !job->errorText().isEmpty() );

  QVERIFY( !job->exec() );
  QVERIFY( mStore->mLastProcessedJob == 0 );

  // test with potentially valid collections
  Collection targetParent;
  targetParent.setRemoteId( QLatin1String( "/tmp/test2" ) );
  job = mStore->createCollection( collection, targetParent );
  QVERIFY( job != 0 );
  QCOMPARE( mStore->mLastCheckedJob, job );
  QCOMPARE( job->error(), 0 );
  QVERIFY( job->errorText().isEmpty() );

  QVERIFY( job->exec() );
  QCOMPARE( mStore->mLastProcessedJob, job );
  mStore->mLastProcessedJob = 0;

  // test template check method
  mStore->mErrorCode = KRandom::random() + 1;
  mStore->mErrorText = KRandom::randomString( 10 );

  job = mStore->createCollection( collection, targetParent );
  QVERIFY( job != 0 );
  QCOMPARE( mStore->mLastCheckedJob, job );
  QCOMPARE( job->error(), mStore->mErrorCode );
  QCOMPARE( job->errorText(), mStore->mErrorText );

  QVERIFY( !job->exec() );
  QVERIFY( mStore->mLastProcessedJob == 0 );
  mStore->mErrorCode = 0;
  mStore->mErrorText = QString();
}

void AbstractLocalStoreTest::testDeleteCollection()
{
  CollectionDeleteJob *job = 0;

  // test without setPath()
  job = mStore->deleteCollection( Collection() );
  QVERIFY( job != 0 );
  QCOMPARE( mStore->mLastCheckedJob, job );
  QCOMPARE( job->error(), (int)Job::InvalidStoreState );
  QVERIFY( !job->errorText().isEmpty() );

  QVERIFY( !job->exec() );
  QVERIFY( mStore->mLastProcessedJob == 0 );

  // test with path but with invalid collection
  mStore->setPath( QLatin1String( "/tmp/test" ) );
  job = mStore->deleteCollection( Collection() );
  QVERIFY( job != 0 );
  QCOMPARE( mStore->mLastCheckedJob, job );
  QCOMPARE( job->error(), (int)Job::InvalidJobContext );
  QVERIFY( !job->errorText().isEmpty() );

  QVERIFY( !job->exec() );
  QVERIFY( mStore->mLastProcessedJob == 0 );

  // test with ivalid collection (has remoteId, but no parent collection remoteId)
  Collection collection;
  collection.setRemoteId( QLatin1String( "/tmp/test/foo" ) );
  job = mStore->deleteCollection( collection );
  QVERIFY( job != 0 );
  QCOMPARE( mStore->mLastCheckedJob, job );
  QCOMPARE( job->error(), (int)Job::InvalidJobContext );
  QVERIFY( !job->errorText().isEmpty() );

  QVERIFY( !job->exec() );
  QVERIFY( mStore->mLastProcessedJob == 0 );

  // test with potentially valid collection (has remoteId and parent collection remoteId)
  Collection parentCollection;
  parentCollection.setRemoteId( QLatin1String( "/tmp/test" ) );
  collection.setParentCollection( parentCollection );
  job = mStore->deleteCollection( collection );
  QVERIFY( job != 0 );
  QCOMPARE( mStore->mLastCheckedJob, job );
  QCOMPARE( job->error(), 0 );
  QVERIFY( job->errorText().isEmpty() );

  QVERIFY( job->exec() );
  QCOMPARE( mStore->mLastProcessedJob, job );
  mStore->mLastProcessedJob = 0;

  // test template check method
  mStore->mErrorCode = KRandom::random() + 1;
  mStore->mErrorText = KRandom::randomString( 10 );

  job = mStore->deleteCollection( collection );
  QVERIFY( job != 0 );
  QCOMPARE( mStore->mLastCheckedJob, job );
  QCOMPARE( job->error(), mStore->mErrorCode );
  QCOMPARE( job->errorText(), mStore->mErrorText );

  QVERIFY( !job->exec() );
  QVERIFY( mStore->mLastProcessedJob == 0 );
  mStore->mErrorCode = 0;
  mStore->mErrorText = QString();
}

void AbstractLocalStoreTest::testFetchCollection()
{
  Akonadi::FileStore::CollectionFetchJob *job = 0;

  // test without setPath()
  job = mStore->fetchCollections( Collection() );
  QVERIFY( job != 0 );
  QCOMPARE( mStore->mLastCheckedJob, job );
  QCOMPARE( job->error(), (int)Job::InvalidStoreState );
  QVERIFY( !job->errorText().isEmpty() );

  QVERIFY( !job->exec() );
  QVERIFY( mStore->mLastProcessedJob == 0 );

  // test with path but with invalid collection
  mStore->setPath( QLatin1String( "/tmp/test" ) );
  job = mStore->fetchCollections( Collection() );
  QVERIFY( job != 0 );
  QCOMPARE( mStore->mLastCheckedJob, job );
  QCOMPARE( job->error(), (int)Job::InvalidJobContext );
  QVERIFY( !job->errorText().isEmpty() );

  QVERIFY( !job->exec() );
  QVERIFY( mStore->mLastProcessedJob == 0 );

  // test with potentially valid collection (has remoteId)
  Collection collection;
  collection.setRemoteId( QLatin1String( "/tmp/test/foo" ) );
  job = mStore->fetchCollections( collection );
  QVERIFY( job != 0 );
  QCOMPARE( mStore->mLastCheckedJob, job );
  QCOMPARE( job->error(), 0 );
  QVERIFY( job->errorText().isEmpty() );

  QVERIFY( job->exec() );
  QCOMPARE( mStore->mLastProcessedJob, job );
  mStore->mLastProcessedJob = 0;

  // test template check method
  mStore->mErrorCode = KRandom::random() + 1;
  mStore->mErrorText = KRandom::randomString( 10 );

  job = mStore->fetchCollections( collection );
  QVERIFY( job != 0 );
  QCOMPARE( mStore->mLastCheckedJob, job );
  QCOMPARE( job->error(), mStore->mErrorCode );
  QCOMPARE( job->errorText(), mStore->mErrorText );

  QVERIFY( !job->exec() );
  QVERIFY( mStore->mLastProcessedJob == 0 );
  mStore->mErrorCode = 0;
  mStore->mErrorText = QString();

  // test fetch of top level collection only
  collection.setRemoteId( mStore->topLevelCollection().remoteId() );
  job = mStore->fetchCollections( collection, Akonadi::FileStore::CollectionFetchJob::Base );
  QVERIFY( job != 0 );
  QCOMPARE( mStore->mLastCheckedJob, job );
  QCOMPARE( job->error(), 0 );
  QVERIFY( job->errorText().isEmpty() );

  QVERIFY( job->exec() );
  QVERIFY( mStore->mLastProcessedJob == 0 ); // job not handed to subclass because it is full processed in base class
  QCOMPARE( job->collections().count(), 1 );
  QCOMPARE( job->collections()[ 0 ], mStore->topLevelCollection() );
}

void AbstractLocalStoreTest::testModifyCollection()
{
  Akonadi::FileStore::CollectionModifyJob *job = 0;

  // test without setPath()
  job = mStore->modifyCollection( Collection() );
  QVERIFY( job != 0 );
  QCOMPARE( mStore->mLastCheckedJob, job );
  QCOMPARE( job->error(), (int)Job::InvalidStoreState );
  QVERIFY( !job->errorText().isEmpty() );

  QVERIFY( !job->exec() );
  QVERIFY( mStore->mLastProcessedJob == 0 );

  // test with path but with invalid item
  mStore->setPath( QLatin1String( "/tmp/test" ) );
  job = mStore->modifyCollection( Collection() );
  QVERIFY( job != 0 );
  QCOMPARE( mStore->mLastCheckedJob, job );
  QCOMPARE( job->error(), (int)Job::InvalidJobContext );
  QVERIFY( !job->errorText().isEmpty() );

  QVERIFY( !job->exec() );
  QVERIFY( mStore->mLastProcessedJob == 0 );

  // test with potentially valid collection (has remoteId, but no parent remoteId)
  Collection collection;
  collection.setRemoteId( QLatin1String( "/tmp/test/foo" ) );
  job = mStore->modifyCollection( collection );
  QVERIFY( job != 0 );
  QCOMPARE( mStore->mLastCheckedJob, job );
  QCOMPARE( job->error(), 0 );
  QVERIFY( job->errorText().isEmpty() );

  QVERIFY( job->exec() );
  QCOMPARE( mStore->mLastProcessedJob, job );
  mStore->mLastProcessedJob = 0;

  // test with potentially valid collection (has remoteId and parent remoteId)
  Collection parentCollection;
  parentCollection.setRemoteId( QLatin1String( "/tmp/test" ) );
  collection.setParentCollection( parentCollection );
  job = mStore->modifyCollection( collection );
  QVERIFY( job != 0 );
  QCOMPARE( mStore->mLastCheckedJob, job );
  QCOMPARE( job->error(), 0 );
  QVERIFY( job->errorText().isEmpty() );

  QVERIFY( job->exec() );
  QCOMPARE( mStore->mLastProcessedJob, job );
  mStore->mLastProcessedJob = 0;

  // test template check method
  mStore->mErrorCode = KRandom::random() + 1;
  mStore->mErrorText = KRandom::randomString( 10 );

  job = mStore->modifyCollection( collection );
  QVERIFY( job != 0 );
  QCOMPARE( mStore->mLastCheckedJob, job );
  QCOMPARE( job->error(), mStore->mErrorCode );
  QCOMPARE( job->errorText(), mStore->mErrorText );

  QVERIFY( !job->exec() );
  QVERIFY( mStore->mLastProcessedJob == 0 );
}

void AbstractLocalStoreTest::testMoveCollection()
{
  CollectionMoveJob *job = 0;

  // test without setPath()
  job = mStore->moveCollection( Collection(), Collection() );
  QVERIFY( job != 0 );
  QCOMPARE( mStore->mLastCheckedJob, job );
  QCOMPARE( job->error(), (int)Job::InvalidStoreState );
  QVERIFY( !job->errorText().isEmpty() );

  QVERIFY( !job->exec() );
  QVERIFY( mStore->mLastProcessedJob == 0 );

  // test with path but with invalid collections
  mStore->setPath( QLatin1String( "/tmp/test" ) );
  job = mStore->moveCollection( Collection(), Collection() );
  QVERIFY( job != 0 );
  QCOMPARE( mStore->mLastCheckedJob, job );
  QCOMPARE( job->error(), (int)Job::InvalidJobContext );
  QVERIFY( !job->errorText().isEmpty() );

  QVERIFY( !job->exec() );
  QVERIFY( mStore->mLastProcessedJob == 0 );

  // test with potentially valid collection (has remoteId and parent remoteId), but invalid target parent
  Collection collection;
  collection.setRemoteId( QLatin1String( "/tmp/test/foo" ) );
  Collection parentCollection;
  parentCollection.setRemoteId( QLatin1String( "/tmp/test" ) );
  collection.setParentCollection( parentCollection );
  job = mStore->moveCollection( collection, Collection() );
  QVERIFY( job != 0 );
  QCOMPARE( mStore->mLastCheckedJob, job );
  QCOMPARE( job->error(), (int)Job::InvalidJobContext );
  QVERIFY( !job->errorText().isEmpty() );

  QVERIFY( !job->exec() );
  QVERIFY( mStore->mLastProcessedJob == 0 );

  // test with invalid parent collection, but with potentially valid collection and target parent
  collection.setParentCollection( Collection() );
  job = mStore->moveCollection( collection, parentCollection );
  QVERIFY( job != 0 );
  QCOMPARE( mStore->mLastCheckedJob, job );
  QCOMPARE( job->error(), (int)Job::InvalidJobContext );
  QVERIFY( !job->errorText().isEmpty() );

  QVERIFY( !job->exec() );
  QVERIFY( mStore->mLastProcessedJob == 0 );

  // test with potentially valid collections
  Collection targetParent;
  targetParent.setRemoteId( QLatin1String( "/tmp/test2" ) );
  collection.setParentCollection( parentCollection );
  job = mStore->moveCollection( collection, targetParent );
  QVERIFY( job != 0 );
  QCOMPARE( mStore->mLastCheckedJob, job );
  QCOMPARE( job->error(), 0 );
  QVERIFY( job->errorText().isEmpty() );

  QVERIFY( job->exec() );
  QCOMPARE( mStore->mLastProcessedJob, job );
  mStore->mLastProcessedJob = 0;

  // test template check method
  mStore->mErrorCode = KRandom::random() + 1;
  mStore->mErrorText = KRandom::randomString( 10 );

  job = mStore->moveCollection( collection, targetParent );
  QVERIFY( job != 0 );
  QCOMPARE( mStore->mLastCheckedJob, job );
  QCOMPARE( job->error(), mStore->mErrorCode );
  QCOMPARE( job->errorText(), mStore->mErrorText );

  QVERIFY( !job->exec() );
  QVERIFY( mStore->mLastProcessedJob == 0 );
}

void AbstractLocalStoreTest::testFetchItems()
{
  ItemFetchJob *job = 0;

  // test without setPath()
  job = mStore->fetchItems( Collection() );
  QVERIFY( job != 0 );
  QCOMPARE( mStore->mLastCheckedJob, job );
  QCOMPARE( job->error(), (int)Job::InvalidStoreState );
  QVERIFY( !job->errorText().isEmpty() );

  QVERIFY( !job->exec() );
  QVERIFY( mStore->mLastProcessedJob == 0 );

  // test with path but with invalid collection
  mStore->setPath( QLatin1String( "/tmp/test" ) );
  job = mStore->fetchItems( Collection() );
  QVERIFY( job != 0 );
  QCOMPARE( mStore->mLastCheckedJob, job );
  QCOMPARE( job->error(), (int)Job::InvalidJobContext );
  QVERIFY( !job->errorText().isEmpty() );

  QVERIFY( !job->exec() );
  QVERIFY( mStore->mLastProcessedJob == 0 );

  // test with potentially valid collection (has remoteId)
  Collection collection;
  collection.setRemoteId( QLatin1String( "/tmp/test/foo" ) );
  job = mStore->fetchItems( collection );
  QVERIFY( job != 0 );
  QCOMPARE( mStore->mLastCheckedJob, job );
  QCOMPARE( job->error(), 0 );
  QVERIFY( job->errorText().isEmpty() );

  QVERIFY( job->exec() );
  QCOMPARE( mStore->mLastProcessedJob, job );
  mStore->mLastProcessedJob = 0;

  // test template check method
  mStore->mErrorCode = KRandom::random() + 1;
  mStore->mErrorText = KRandom::randomString( 10 );

  job = mStore->fetchItems( collection );
  QVERIFY( job != 0 );
  QCOMPARE( mStore->mLastCheckedJob, job );
  QCOMPARE( job->error(), mStore->mErrorCode );
  QCOMPARE( job->errorText(), mStore->mErrorText );

  QVERIFY( !job->exec() );
  QVERIFY( mStore->mLastProcessedJob == 0 );
  mStore->mErrorCode = 0;
  mStore->mErrorText = QString();
}

void AbstractLocalStoreTest::testFetchItem()
{
  ItemFetchJob *job = 0;

  // test without setPath()
  job = mStore->fetchItem( Item() );
  QVERIFY( job != 0 );
  QCOMPARE( mStore->mLastCheckedJob, job );
  QCOMPARE( job->error(), (int)Job::InvalidStoreState );
  QVERIFY( !job->errorText().isEmpty() );

  QVERIFY( !job->exec() );
  QVERIFY( mStore->mLastProcessedJob == 0 );

  // test with path but with invalid item
  mStore->setPath( QLatin1String( "/tmp/test" ) );
  job = mStore->fetchItem( Item() );
  QVERIFY( job != 0 );
  QCOMPARE( mStore->mLastCheckedJob, job );
  QCOMPARE( job->error(), (int)Job::InvalidJobContext );
  QVERIFY( !job->errorText().isEmpty() );

  QVERIFY( !job->exec() );
  QVERIFY( mStore->mLastProcessedJob == 0 );

  // test with potentially valid item (has remoteId)
  Item item;
  item.setRemoteId( QLatin1String( "/tmp/test/foo" ) );
  job = mStore->fetchItem( item );
  QVERIFY( job != 0 );
  QCOMPARE( mStore->mLastCheckedJob, job );
  QCOMPARE( job->error(), 0 );
  QVERIFY( job->errorText().isEmpty() );

  QVERIFY( job->exec() );
  QCOMPARE( mStore->mLastProcessedJob, job );
  mStore->mLastProcessedJob = 0;

  // test template check method
  mStore->mErrorCode = KRandom::random() + 1;
  mStore->mErrorText = KRandom::randomString( 10 );

  job = mStore->fetchItem( item );
  QVERIFY( job != 0 );
  QCOMPARE( mStore->mLastCheckedJob, job );
  QCOMPARE( job->error(), mStore->mErrorCode );
  QCOMPARE( job->errorText(), mStore->mErrorText );

  QVERIFY( !job->exec() );
  QVERIFY( mStore->mLastProcessedJob == 0 );
  mStore->mErrorCode = 0;
  mStore->mErrorText = QString();
}

void AbstractLocalStoreTest::testCreateItem()
{
  Akonadi::FileStore::ItemCreateJob *job = 0;

  // test without setPath()
  job = mStore->createItem( Item(), Collection() );
  QVERIFY( job != 0 );
  QCOMPARE( mStore->mLastCheckedJob, job );
  QCOMPARE( job->error(), (int)Job::InvalidStoreState );
  QVERIFY( !job->errorText().isEmpty() );

  QVERIFY( !job->exec() );
  QVERIFY( mStore->mLastProcessedJob == 0 );

  // test with path but with invalid collection
  mStore->setPath( QLatin1String( "/tmp/test" ) );
  job = mStore->createItem( Item(), Collection() );
  QVERIFY( job != 0 );
  QCOMPARE( mStore->mLastCheckedJob, job );
  QCOMPARE( job->error(), (int)Job::InvalidJobContext );
  QVERIFY( !job->errorText().isEmpty() );

  QVERIFY( !job->exec() );
  QVERIFY( mStore->mLastProcessedJob == 0 );

  // test with potentially valid collection (has remoteId)
  Collection collection;
  collection.setRemoteId( QLatin1String( "/tmp/test/foo" ) );
  job = mStore->createItem( Item(), collection );
  QVERIFY( job != 0 );
  QCOMPARE( mStore->mLastCheckedJob, job );
  QCOMPARE( job->error(), 0 );
  QVERIFY( job->errorText().isEmpty() );

  QVERIFY( job->exec() );
  QCOMPARE( mStore->mLastProcessedJob, job );
  mStore->mLastProcessedJob = 0;

  // test template check method
  mStore->mErrorCode = KRandom::random() + 1;
  mStore->mErrorText = KRandom::randomString( 10 );

  job = mStore->createItem( Item(), collection );
  QVERIFY( job != 0 );
  QCOMPARE( mStore->mLastCheckedJob, job );
  QCOMPARE( job->error(), mStore->mErrorCode );
  QCOMPARE( job->errorText(), mStore->mErrorText );

  QVERIFY( !job->exec() );
  QVERIFY( mStore->mLastProcessedJob == 0 );
  mStore->mErrorCode = 0;
  mStore->mErrorText = QString();
}

void AbstractLocalStoreTest::testDeleteItem()
{
  ItemDeleteJob *job = 0;

  // test without setPath()
  job = mStore->deleteItem( Item() );
  QVERIFY( job != 0 );
  QCOMPARE( mStore->mLastCheckedJob, job );
  QCOMPARE( job->error(), (int)Job::InvalidStoreState );
  QVERIFY( !job->errorText().isEmpty() );

  QVERIFY( !job->exec() );
  QVERIFY( mStore->mLastProcessedJob == 0 );

  // test with path but with invalid item
  mStore->setPath( QLatin1String( "/tmp/test" ) );
  job = mStore->deleteItem( Item() );
  QVERIFY( job != 0 );
  QCOMPARE( mStore->mLastCheckedJob, job );
  QCOMPARE( job->error(), (int)Job::InvalidJobContext );
  QVERIFY( !job->errorText().isEmpty() );

  QVERIFY( !job->exec() );
  QVERIFY( mStore->mLastProcessedJob == 0 );

  // test with potentially valid item (has remoteId)
  Item item;
  item.setRemoteId( QLatin1String( "/tmp/test/foo" ) );
  job = mStore->deleteItem( item );
  QVERIFY( job != 0 );
  QCOMPARE( mStore->mLastCheckedJob, job );
  QCOMPARE( job->error(), 0 );
  QVERIFY( job->errorText().isEmpty() );

  QVERIFY( job->exec() );
  QCOMPARE( mStore->mLastProcessedJob, job );
  mStore->mLastProcessedJob = 0;

  // test template check method
  mStore->mErrorCode = KRandom::random() + 1;
  mStore->mErrorText = KRandom::randomString( 10 );

  job = mStore->deleteItem( item );
  QVERIFY( job != 0 );
  QCOMPARE( mStore->mLastCheckedJob, job );
  QCOMPARE( job->error(), mStore->mErrorCode );
  QCOMPARE( job->errorText(), mStore->mErrorText );

  QVERIFY( !job->exec() );
  QVERIFY( mStore->mLastProcessedJob == 0 );
  mStore->mErrorCode = 0;
  mStore->mErrorText = QString();
}

void AbstractLocalStoreTest::testModifyItem()
{
  Akonadi::FileStore::ItemModifyJob *job = 0;

  // test without setPath()
  job = mStore->modifyItem( Item() );
  QVERIFY( job != 0 );
  QCOMPARE( mStore->mLastCheckedJob, job );
  QCOMPARE( job->error(), (int)Job::InvalidStoreState );
  QVERIFY( !job->errorText().isEmpty() );

  QVERIFY( !job->exec() );
  QVERIFY( mStore->mLastProcessedJob == 0 );

  // test with path but with invalid item
  mStore->setPath( QLatin1String( "/tmp/test" ) );
  job = mStore->modifyItem( Item() );
  QVERIFY( job != 0 );
  QCOMPARE( mStore->mLastCheckedJob, job );
  QCOMPARE( job->error(), (int)Job::InvalidJobContext );
  QVERIFY( !job->errorText().isEmpty() );

  QVERIFY( !job->exec() );
  QVERIFY( mStore->mLastProcessedJob == 0 );

  // test with potentially valid item (has remoteId)
  Item item;
  item.setRemoteId( QLatin1String( "/tmp/test/foo" ) );
  job = mStore->modifyItem( item );
  QVERIFY( job != 0 );
  QCOMPARE( mStore->mLastCheckedJob, job );
  QCOMPARE( job->error(), 0 );
  QVERIFY( job->errorText().isEmpty() );

  QVERIFY( job->exec() );
  QCOMPARE( mStore->mLastProcessedJob, job );
  mStore->mLastProcessedJob = 0;

  // test template check method
  mStore->mErrorCode = KRandom::random() + 1;
  mStore->mErrorText = KRandom::randomString( 10 );

  job = mStore->modifyItem( item );
  QVERIFY( job != 0 );
  QCOMPARE( mStore->mLastCheckedJob, job );
  QCOMPARE( job->error(), mStore->mErrorCode );
  QCOMPARE( job->errorText(), mStore->mErrorText );

  QVERIFY( !job->exec() );
  QVERIFY( mStore->mLastProcessedJob == 0 );
}

void AbstractLocalStoreTest::testMoveItem()
{
  ItemMoveJob *job = 0;

  // test without setPath()
  job = mStore->moveItem( Item(), Collection() );
  QVERIFY( job != 0 );
  QCOMPARE( mStore->mLastCheckedJob, job );
  QCOMPARE( job->error(), (int)Job::InvalidStoreState );
  QVERIFY( !job->errorText().isEmpty() );

  QVERIFY( !job->exec() );
  QVERIFY( mStore->mLastProcessedJob == 0 );

  // test with path but with invalid item and collection
  mStore->setPath( QLatin1String( "/tmp/test" ) );
  job = mStore->moveItem( Item(), Collection() );
  QVERIFY( job != 0 );
  QCOMPARE( mStore->mLastCheckedJob, job );
  QCOMPARE( job->error(), (int)Job::InvalidJobContext );
  QVERIFY( !job->errorText().isEmpty() );

  QVERIFY( !job->exec() );
  QVERIFY( mStore->mLastProcessedJob == 0 );

  // test with potentially valid item (has remoteId and parent remoteId), but invalid target parent
  Item item;
  item.setRemoteId( QLatin1String( "/tmp/test/foo" ) );
  Collection parentCollection;
  parentCollection.setRemoteId( QLatin1String( "/tmp/test" ) );
  item.setParentCollection( parentCollection );
  job = mStore->moveItem( item, Collection() );
  QVERIFY( job != 0 );
  QCOMPARE( mStore->mLastCheckedJob, job );
  QCOMPARE( job->error(), (int)Job::InvalidJobContext );
  QVERIFY( !job->errorText().isEmpty() );

  QVERIFY( !job->exec() );
  QVERIFY( mStore->mLastProcessedJob == 0 );

  // test with invalid parent collection, but with potentially valid item and target parent
  item.setParentCollection( Collection() );
  job = mStore->moveItem( item, parentCollection );
  QVERIFY( job != 0 );
  QCOMPARE( mStore->mLastCheckedJob, job );
  QCOMPARE( job->error(), (int)Job::InvalidJobContext );
  QVERIFY( !job->errorText().isEmpty() );

  QVERIFY( !job->exec() );
  QVERIFY( mStore->mLastProcessedJob == 0 );

  // test with potentially valid item and collections
  Collection targetParent;
  targetParent.setRemoteId( QLatin1String( "/tmp/test2" ) );
  item.setParentCollection( parentCollection );
  job = mStore->moveItem( item, targetParent );
  QVERIFY( job != 0 );
  QCOMPARE( mStore->mLastCheckedJob, job );
  QCOMPARE( job->error(), 0 );
  QVERIFY( job->errorText().isEmpty() );

  QVERIFY( job->exec() );
  QCOMPARE( mStore->mLastProcessedJob, job );
  mStore->mLastProcessedJob = 0;

  // test template check method
  mStore->mErrorCode = KRandom::random() + 1;
  mStore->mErrorText = KRandom::randomString( 10 );

  job = mStore->moveItem( item, targetParent );
  QVERIFY( job != 0 );
  QCOMPARE( mStore->mLastCheckedJob, job );
  QCOMPARE( job->error(), mStore->mErrorCode );
  QCOMPARE( job->errorText(), mStore->mErrorText );

  QVERIFY( !job->exec() );
  QVERIFY( mStore->mLastProcessedJob == 0 );
}

void AbstractLocalStoreTest::testCompactStore()
{
  StoreCompactJob *job = 0;

  // test without setPath()
  job = mStore->compactStore();
  QVERIFY( job != 0 );
  QCOMPARE( mStore->mLastCheckedJob, job );
  QCOMPARE( job->error(), (int)Job::InvalidStoreState );
  QVERIFY( !job->errorText().isEmpty() );

  QVERIFY( !job->exec() );
  QVERIFY( mStore->mLastProcessedJob == 0 );

  // test with path
  mStore->setPath( QLatin1String( "/tmp/test" ) );
  job = mStore->compactStore();
  QVERIFY( job != 0 );
  QCOMPARE( mStore->mLastCheckedJob, job );
  QCOMPARE( job->error(), 0 );
  QVERIFY( job->errorText().isEmpty() );

  QVERIFY( job->exec() );
  QCOMPARE( mStore->mLastProcessedJob, job );
  mStore->mLastProcessedJob = 0;

  // test template check method
  mStore->mErrorCode = KRandom::random() + 1;
  mStore->mErrorText = KRandom::randomString( 10 );

  job = mStore->compactStore();
  QVERIFY( job != 0 );
  QCOMPARE( mStore->mLastCheckedJob, job );
  QCOMPARE( job->error(), mStore->mErrorCode );
  QCOMPARE( job->errorText(), mStore->mErrorText );

  QVERIFY( !job->exec() );
  QVERIFY( mStore->mLastProcessedJob == 0 );
  mStore->mErrorCode = 0;
  mStore->mErrorText = QString();
}

QTEST_KDEMAIN( AbstractLocalStoreTest, NoGUI )

#include "abstractlocalstoretest.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
