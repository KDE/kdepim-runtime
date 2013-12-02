/*
    Copyright (c) 2013 Christian Mollekopf <mollekopf@kolabsys.com>

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

#include <QObject>
#include <qtest_kde.h>
#include <akonadi/qtest_akonadi.h>
#include <akonadi/servermanager.h>
#include <akonadi/collectionpathresolver_p.h>
#include <Akonadi/Item>
#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemFetchScope>
#include <Akonadi/AgentInstanceCreateJob>
#include <Akonadi/CollectionFetchJob>
#include <Akonadi/CollectionFetchScope>
#include <Akonadi/AttributeFactory>
#include <Akonadi/EntityHiddenAttribute>
#include <Akonadi/ItemCreateJob>
#include <Akonadi/ItemDeleteJob>
#include <KCalCore/Event>
#include <KMime/Message>
#include <QDBusInterface>
#include <collectionannotationsattribute.h>
#include <kolabdefinitions.h> //libkolab
#include <kolabobject.h>

#include "../kolabdefs.h"

using namespace Akonadi;

/*
 * Tests clientside actions against the kolab proxy, such as creating an item.
 */
class ClientSideTest : public QObject
{
    Q_OBJECT

    AgentInstance mInstance;
    Akonadi::Collection imapCollection;
    Akonadi::Collection kolabCollection;

    void cleanup() {
        //cleanup
        Akonadi::ItemDeleteJob *deleteJob = new Akonadi::ItemDeleteJob(imapCollection);
        AKVERIFYEXEC(deleteJob);
        Akonadi::ItemDeleteJob *deleteJob2 = new Akonadi::ItemDeleteJob(kolabCollection);
        AKVERIFYEXEC(deleteJob2);
        QTest::qWait(100);
    }

private slots:
    void initTestCase() {
        AkonadiTest::checkTestIsIsolated();
        AttributeFactory::registerAttribute<CollectionAnnotationsAttribute>();

        const AgentType type = AgentManager::self()->type("akonadi_kolabproxy_resource");
        AgentInstanceCreateJob *agentCreateJob = new AgentInstanceCreateJob(type);
        AKVERIFYEXEC(agentCreateJob);
        mInstance = agentCreateJob->instance();

        //Wait for kolabproxy to create all folders
        QTest::qWait(1000);

        Akonadi::CollectionPathResolver *resolver = new CollectionPathResolver(QLatin1String("res1/Calendar"), this);
        AKVERIFYEXEC(resolver);
        imapCollection = Akonadi::Collection( resolver->collection() );
        QVERIFY(imapCollection.isValid());

        Akonadi::CollectionFetchJob *fetchJob = new Akonadi::CollectionFetchJob(Collection::root(), CollectionFetchJob::Recursive);
        fetchJob->fetchScope().setResource(mInstance.identifier());
        AKVERIFYEXEC(fetchJob);
        foreach (const Collection &col, fetchJob->collections()) {
            if (col.name().contains("Calendar")) {
                kolabCollection = col;
            }
        }
        QVERIFY(kolabCollection.isValid());
    }

    void testItemCreate()
    {
        KCalCore::Event::Ptr event(new KCalCore::Event());
        event->setDtStart(KDateTime(QDate(2013,10,10)));
        Akonadi::Item createdItem;
        {
            Akonadi::Item item(event->mimeType());
            item.setPayload(event);
            Akonadi::ItemCreateJob *createJob = new Akonadi::ItemCreateJob(item, kolabCollection);
            AKVERIFYEXEC(createJob);
            createdItem = createJob->item();
            QVERIFY(createdItem.isValid());
        }

        QTest::qWait(100);

        {
            Akonadi::ItemFetchJob *fetchJob = new Akonadi::ItemFetchJob(imapCollection);
            AKVERIFYEXEC(fetchJob);
            QCOMPARE(fetchJob->items().size(), 1);
//             const Akonadi::Item item = fetchJob->items().first();
//             QCOMPARE(item.id(), createdItem.remoteId().toLongLong());
        }
        cleanup();
    }

    void testItemCreateFailure()
    {
        KCalCore::Event::Ptr event(new KCalCore::Event());
        //an event requires an start date, so this will fail
        {
            Akonadi::Item item(event->mimeType());
            item.setPayload(event);
            Akonadi::ItemCreateJob *createJob = new Akonadi::ItemCreateJob(item, kolabCollection);
            AKVERIFYEXEC(createJob);
            //TODO akonadi currently doesn't support failing itemcreatejobs if the resource fails to store the item.
            // the item will simply remain dirty in the akonadi server
//             QVERIFY(createJob->error());
        }
        QTest::qWait(100);
        //Ensure the item has been removed by the kolabproxy
        {
            Akonadi::ItemFetchJob *fetchJob = new Akonadi::ItemFetchJob(kolabCollection);
            AKVERIFYEXEC(fetchJob);
            QCOMPARE(fetchJob->items().size(), 0);
        }
    }

};

QTEST_AKONADIMAIN( ClientSideTest, NoGUI )

#include "clientsidetest.moc"
