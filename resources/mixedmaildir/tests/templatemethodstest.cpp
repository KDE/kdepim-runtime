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

#include <akonadi/cachepolicy.h>
#include <akonadi/kmime/messageparts.h>

#include <kmime/kmime_message.h>

#include <KRandom>

#include <qtest_kde.h>

using namespace Akonadi;

class TestStore : public MixedMaildirStore
{
  Q_OBJECT

  public:
    TestStore() : mLastCheckedJob( 0 ), mErrorCode( 0 ) {}

  public:
    Collection mTopLevelCollection;

    mutable FileStore::Job *mLastCheckedJob;
    mutable int mErrorCode;
    mutable QString mErrorText;

  protected:
    void setTopLevelCollection( const Collection &collection )
    {
      MixedMaildirStore::setTopLevelCollection( collection );
    }

    void checkCollectionMove( FileStore::CollectionMoveJob *job, int &errorCode, QString &errorText ) const
    {
      MixedMaildirStore::checkCollectionMove( job, errorCode, errorText );

      mLastCheckedJob = job;
      mErrorCode = errorCode;
      mErrorText = errorText;
    }

    void checkItemCreate( FileStore::ItemCreateJob *job, int &errorCode, QString &errorText ) const
    {
      MixedMaildirStore::checkItemCreate( job, errorCode, errorText );

      mLastCheckedJob = job;
      mErrorCode = errorCode;
      mErrorText = errorText;
    }
};

class TemplateMethodsTest : public QObject
{
  Q_OBJECT

  public:
    TemplateMethodsTest() : QObject(), mStore( 0 ) {}
    ~TemplateMethodsTest() { delete mStore; }

  private:
    TestStore *mStore;

  private Q_SLOTS:
    void init();
    void testSetTopLevelCollection();
    void testMoveCollection();
    void testCreateItem();
};

void TemplateMethodsTest::init()
{
  delete mStore;
  mStore = new TestStore;
}

void TemplateMethodsTest::testSetTopLevelCollection()
{
  const QString file = KRandom::randomString( 10 );
  const QString path = QLatin1String( "/tmp/test/" ) + file;

  mStore->setPath( path );

  // check the adjustments on the top level collection
  const Collection collection = mStore->topLevelCollection();
  QCOMPARE( collection.contentMimeTypes(), QStringList() << Collection::mimeType() );
  QCOMPARE( collection.rights(), Collection::CanCreateCollection |
                                 Collection::CanChangeCollection |
                                 Collection::CanDeleteCollection );
  const CachePolicy cachePolicy = collection.cachePolicy();
  QVERIFY( !cachePolicy.inheritFromParent() );
  QCOMPARE( cachePolicy.localParts(), QStringList() << MessagePart::Envelope );
  QVERIFY( cachePolicy.syncOnDemand() );
}

void TemplateMethodsTest::testMoveCollection()
{
  mStore->setPath( QLatin1String( "/tmp/test" ) );

  FileStore::CollectionMoveJob *job = 0;

  // test moving into itself
  Collection collection( KRandom::random() );
  collection.setParentCollection( mStore->topLevelCollection() );
  collection.setRemoteId( "collection" );
  job = mStore->moveCollection( collection, collection );
  QVERIFY( job != 0 );
  QCOMPARE( job->error(), (int)FileStore::Job::InvalidJobContext );
  QVERIFY( !job->errorText().isEmpty() );
  QCOMPARE( mStore->mLastCheckedJob, job );
  QCOMPARE( mStore->mErrorCode, job->error() );
  QCOMPARE( mStore->mErrorText, job->errorText() );

  QVERIFY( !job->exec() );

  // test moving into child
  Collection childCollection( collection.id() + 1 );
  childCollection.setParentCollection( collection );
  childCollection.setRemoteId( "child" );
  job = mStore->moveCollection( collection, childCollection );
  QVERIFY( job != 0 );
  QCOMPARE( job->error(), (int)FileStore::Job::InvalidJobContext );
  QVERIFY( !job->errorText().isEmpty() );
  QCOMPARE( mStore->mLastCheckedJob, job );
  QCOMPARE( mStore->mErrorCode, job->error() );
  QCOMPARE( mStore->mErrorText, job->errorText() );

  QVERIFY( !job->exec() );

  // test moving into grand child child
  Collection grandChildCollection( collection.id() + 2 );
  grandChildCollection.setParentCollection( childCollection );
  grandChildCollection.setRemoteId( "grandchild" );
  job = mStore->moveCollection( collection, grandChildCollection );
  QVERIFY( job != 0 );
  QCOMPARE( job->error(), (int)FileStore::Job::InvalidJobContext );
  QVERIFY( !job->errorText().isEmpty() );
  QCOMPARE( mStore->mLastCheckedJob, job );
  QCOMPARE( mStore->mErrorCode, job->error() );
  QCOMPARE( mStore->mErrorText, job->errorText() );

  QVERIFY( !job->exec() );

  // test moving into unrelated collection
  Collection otherCollection( collection.id() + KRandom::random() );
  otherCollection.setRemoteId( "other" );
  job = mStore->moveCollection( collection, otherCollection );
  QVERIFY( job != 0 );
  QCOMPARE( job->error(), 0 );
  QVERIFY( job->errorText().isEmpty() );
  QCOMPARE( mStore->mLastCheckedJob, job );

  // collections don't exist in the store, so processing still fails
  QVERIFY( !job->exec() );
}

void TemplateMethodsTest::testCreateItem()
{
  mStore->setPath( QLatin1String( "/tmp/test" ) );

  Collection collection( KRandom::random() );
  collection.setParentCollection( mStore->topLevelCollection() );
  collection.setRemoteId( "collection" );

  FileStore::ItemCreateJob *job = 0;

  // test item without payload
  Item item;
  job = mStore->createItem( item, collection );
  QVERIFY( job != 0 );
  QCOMPARE( job->error(), (int)FileStore::Job::InvalidJobContext );
  QVERIFY( !job->errorText().isEmpty() );
  QCOMPARE( mStore->mLastCheckedJob, job );
  QCOMPARE( mStore->mErrorCode, job->error() );
  QCOMPARE( mStore->mErrorText, job->errorText() );

  QVERIFY( !job->exec() );

  // test item with payload
  item.setPayload<KMime::Message::Ptr>( KMime::Message::Ptr() );
  job = mStore->createItem( item, collection );
  QVERIFY( job != 0 );
  QCOMPARE( job->error(), 0 );
  QVERIFY( job->errorText().isEmpty() );
  QCOMPARE( mStore->mLastCheckedJob, job );

  // collections don't exist in the store, so processing still fails
  QVERIFY( !job->exec() );
}

QTEST_KDEMAIN( TemplateMethodsTest, NoGUI )

#include "templatemethodstest.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
