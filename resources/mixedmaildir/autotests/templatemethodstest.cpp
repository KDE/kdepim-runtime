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

#include <cachepolicy.h>
#include <akonadi/kmime/messageparts.h>

#include <kmime/kmime_message.h>

#include <KRandom>
#include <QTemporaryDir>
#include <QRandomGenerator>

#include <QTest>

using namespace Akonadi;

class TestStore : public MixedMaildirStore
{
    Q_OBJECT

public:
    TestStore() : mLastCheckedJob(nullptr)
        , mErrorCode(0)
    {
    }

public:
    Collection mTopLevelCollection;

    mutable FileStore::Job *mLastCheckedJob = nullptr;
    mutable int mErrorCode;
    mutable QString mErrorText;

protected:
    void setTopLevelCollection(const Collection &collection) override
    {
        MixedMaildirStore::setTopLevelCollection(collection);
    }

    void checkCollectionMove(FileStore::CollectionMoveJob *job, int &errorCode, QString &errorText) const override
    {
        MixedMaildirStore::checkCollectionMove(job, errorCode, errorText);

        mLastCheckedJob = job;
        mErrorCode = errorCode;
        mErrorText = errorText;
    }

    void checkItemCreate(FileStore::ItemCreateJob *job, int &errorCode, QString &errorText) const override
    {
        MixedMaildirStore::checkItemCreate(job, errorCode, errorText);

        mLastCheckedJob = job;
        mErrorCode = errorCode;
        mErrorText = errorText;
    }
};

class TemplateMethodsTest : public QObject
{
    Q_OBJECT

public:
    TemplateMethodsTest() : QObject()
        , mStore(nullptr)
    {
    }

    ~TemplateMethodsTest()
    {
        delete mStore;
    }

private:
    QTemporaryDir mDir;
    TestStore *mStore = nullptr;

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
    QVERIFY(mDir.isValid());
    QVERIFY(QDir(mDir.path()).exists());
}

void TemplateMethodsTest::testSetTopLevelCollection()
{
    const QString file = KRandom::randomString(10);
    const QString path = mDir.path() + file;

    mStore->setPath(path);

    // check the adjustments on the top level collection
    const Collection collection = mStore->topLevelCollection();
    QCOMPARE(collection.contentMimeTypes(), QStringList() << Collection::mimeType());
    QCOMPARE(collection.rights(), Collection::CanCreateCollection
             |Collection::CanChangeCollection
             |Collection::CanDeleteCollection);
    const CachePolicy cachePolicy = collection.cachePolicy();
    QVERIFY(!cachePolicy.inheritFromParent());
    QCOMPARE(cachePolicy.localParts(), QStringList() << QLatin1String(MessagePart::Envelope));
    QVERIFY(cachePolicy.syncOnDemand());
}

void TemplateMethodsTest::testMoveCollection()
{
    mStore->setPath(mDir.path());

    FileStore::CollectionMoveJob *job = nullptr;

    // test moving into itself
    Collection collection(QRandomGenerator::global()->generate());
    collection.setParentCollection(mStore->topLevelCollection());
    collection.setRemoteId(QStringLiteral("collection"));
    job = mStore->moveCollection(collection, collection);
    QVERIFY(job != nullptr);
    QCOMPARE(job->error(), (int)FileStore::Job::InvalidJobContext);
    QVERIFY(!job->errorText().isEmpty());
    QCOMPARE(mStore->mLastCheckedJob, job);
    QCOMPARE(mStore->mErrorCode, job->error());
    QCOMPARE(mStore->mErrorText, job->errorText());

    QVERIFY(!job->exec());

    // test moving into child
    Collection childCollection(collection.id() + 1);
    childCollection.setParentCollection(collection);
    childCollection.setRemoteId(QStringLiteral("child"));
    job = mStore->moveCollection(collection, childCollection);
    QVERIFY(job != nullptr);
    QCOMPARE(job->error(), (int)FileStore::Job::InvalidJobContext);
    QVERIFY(!job->errorText().isEmpty());
    QCOMPARE(mStore->mLastCheckedJob, job);
    QCOMPARE(mStore->mErrorCode, job->error());
    QCOMPARE(mStore->mErrorText, job->errorText());

    QVERIFY(!job->exec());

    // test moving into grand child child
    Collection grandChildCollection(collection.id() + 2);
    grandChildCollection.setParentCollection(childCollection);
    grandChildCollection.setRemoteId(QStringLiteral("grandchild"));
    job = mStore->moveCollection(collection, grandChildCollection);
    QVERIFY(job != nullptr);
    QCOMPARE(job->error(), (int)FileStore::Job::InvalidJobContext);
    QVERIFY(!job->errorText().isEmpty());
    QCOMPARE(mStore->mLastCheckedJob, job);
    QCOMPARE(mStore->mErrorCode, job->error());
    QCOMPARE(mStore->mErrorText, job->errorText());

    QVERIFY(!job->exec());

    // test moving into unrelated collection
    Collection otherCollection(collection.id() + QRandomGenerator::global()->generate());
    otherCollection.setParentCollection(mStore->topLevelCollection());
    otherCollection.setRemoteId(QStringLiteral("other"));
    job = mStore->moveCollection(collection, otherCollection);
    QVERIFY(job != nullptr);
    QCOMPARE(job->error(), 0);
    QVERIFY(job->errorText().isEmpty());
    QCOMPARE(mStore->mLastCheckedJob, job);

    // collections don't exist in the store, so processing still fails
    QVERIFY(!job->exec());
}

void TemplateMethodsTest::testCreateItem()
{
    mStore->setPath(mDir.path());

    Collection collection(QRandomGenerator::global()->generate());
    collection.setParentCollection(mStore->topLevelCollection());
    collection.setRemoteId(QStringLiteral("collection"));

    FileStore::ItemCreateJob *job = nullptr;

    // test item without payload
    Item item(KMime::Message::mimeType());
    job = mStore->createItem(item, collection);
    QVERIFY(job != nullptr);
    QCOMPARE(job->error(), (int)FileStore::Job::InvalidJobContext);
    QVERIFY(!job->errorText().isEmpty());
    QCOMPARE(mStore->mLastCheckedJob, job);
    QCOMPARE(mStore->mErrorCode, job->error());
    QCOMPARE(mStore->mErrorText, job->errorText());

    QVERIFY(!job->exec());

    // test item with payload
    item.setPayloadFromData("Subject: dummy payload\n\nwith some content");
    job = mStore->createItem(item, collection);
    QVERIFY(job != nullptr);
    QCOMPARE(job->error(), 0);
    QVERIFY(job->errorText().isEmpty());
    QCOMPARE(mStore->mLastCheckedJob, job);

    // collections don't exist in the store, so processing still fails
    QVERIFY(!job->exec());
}

QTEST_MAIN(TemplateMethodsTest)

#include "templatemethodstest.moc"
