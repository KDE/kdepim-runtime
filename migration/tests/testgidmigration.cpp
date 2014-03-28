/*
    Copyright 2013 Christian Mollekopf <mollekopf@kolabsys.com>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/
#include "testgidmigration.h"

#include <akonadi/item.h>
#include <Akonadi/ItemFetchJob>
#include <akonadi/itemfetchscope.h>
#include <Akonadi/EntityTreeModel>
#include <Akonadi/Control>
#include <akonadi/qtest_akonadi.h>
#include <akonadi/collectionpathresolver_p.h>

#include "gid/gidmigrationjob.h"


bool TestSerializer::deserialize(Akonadi::Item &item, const QByteArray &label, QIODevice &data, int /* version */ )
{
    if (label != Akonadi::Item::FullPayload) {
        return false;
    }
    item.setPayload(data.readAll());
    return true;
}

void TestSerializer::serialize(const Akonadi::Item &item, const QByteArray &label, QIODevice &data, int &/* version */)
{
    Q_ASSERT(label == Akonadi::Item::FullPayload);
    data.write(item.payload<QByteArray>());
}

QString TestSerializer::extractGid(const Akonadi::Item &item) const
{
    if (item.gid().isEmpty()) {
        return item.url().url();
    }
    return item.gid();
}


void TestGidMigration::allItemsHaveGid(const Collection &col, bool haveGid, bool *success)
{
    *success = false;

    ItemFetchJob *fetchJob = new ItemFetchJob(col);
    fetchJob->fetchScope().fetchFullPayload();
    fetchJob->fetchScope().setFetchGid(true);
    AKVERIFYEXEC(fetchJob);
    QCOMPARE(fetchJob->items().count(), 15);

    Q_FOREACH (const Item &item, fetchJob->items()) {
        if (haveGid) {
            QVERIFY2(!item.gid().isEmpty(), QString::fromLatin1("Item %1 (%2) does not have GID!").arg(item.id()).arg(item.remoteId()).toUtf8().constData());
        } else {
            QVERIFY2(item.gid().isEmpty(), QString::fromLatin1("Item %1 (%2) has GID!").arg(item.id()).arg(item.remoteId()).toUtf8().constData());
        }
    }

    *success = true;
}

TestGidMigration::TestGidMigration(QObject *parent)
:   QObject(parent)
{

}

void TestGidMigration::initTestCase()
{
    AkonadiTest::checkTestIsIsolated();
}

void TestGidMigration::init()
{
    AkonadiTest::setAllResourcesOffline();
    Akonadi::AgentInstance agent = Akonadi::AgentManager::self()->instance(QLatin1String("akonadi_knut_resource_0"));
    QVERIFY(agent.isValid());
    agent.setIsOnline(true);

    Akonadi::ItemSerializerPlugin::overridePluginLookup(new TestSerializer);
}

void TestGidMigration::testMigration()
{
    Akonadi::CollectionPathResolver *resolver = new CollectionPathResolver(QLatin1String("res1/foo"), this);
    AKVERIFYEXEC(resolver);
    const Entity::Id colId = resolver->collection();
    bool allItemsHaveGidOk = false;

    allItemsHaveGid(Collection(colId), false, &allItemsHaveGidOk);
    QVERIFY(allItemsHaveGidOk);

    GidMigrationJob *migrationJob = new GidMigrationJob(QStringList() << QLatin1String("application/octet-stream"), this);
    AKVERIFYEXEC(migrationJob);

    allItemsHaveGid(Collection(colId), true, &allItemsHaveGidOk);
    QVERIFY(allItemsHaveGidOk);
}

QTEST_AKONADIMAIN(TestGidMigration, NoGUI)
