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


static bool allItemsHaveGid(const Collection &col)
{
    kDebug() << col.id();
    ItemFetchJob *fetchJob = new ItemFetchJob(col);
    fetchJob->fetchScope().fetchFullPayload();
    fetchJob->fetchScope().setFetchGid(true);
    Q_ASSERT(fetchJob->exec());
    foreach (const Item &item, fetchJob->items()) {
        kDebug() << item.id() << item.gid();
        if (item.gid().isEmpty()) {
            return false;
        }
    }
    return true;
}

TestGidMigration::TestGidMigration(QObject *parent)
:   QObject(parent)
{

}

void TestGidMigration::initTestCase()
{
    AkonadiTest::checkTestIsIsolated();

    AkonadiTest::setAllResourcesOffline();
    Akonadi::AgentInstance agent = Akonadi::AgentManager::self()->instance("akonadi_knut_resource_0");
    QVERIFY(agent.isValid());
    agent.setIsOnline(true);

    Akonadi::ItemSerializerPlugin::overridePluginLookup(new TestSerializer);
}

void TestGidMigration::testMigration()
{
    Akonadi::CollectionPathResolver *resolver = new CollectionPathResolver(QLatin1String("res1/foo"), this);
    AKVERIFYEXEC(resolver);
    const int colId = resolver->collection();

    QVERIFY(!allItemsHaveGid(Collection(colId)));

    GidMigrationJob *migrationJob = new GidMigrationJob(QStringList() << QLatin1String("application/octet-stream"), this);
    AKVERIFYEXEC(migrationJob);

    QVERIFY(allItemsHaveGid(Collection(colId)));
}

QTEST_AKONADIMAIN(TestGidMigration, NoGUI)
