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
#include <Akonadi/ItemModifyJob>
#include <KCalCore/Event>
#include <KMime/Message>
#include <QDBusInterface>
#include <collectionannotationsattribute.h>
#include <kolabdefinitions.h> //libkolab
#include <kolabobject.h>

#include "../kolabdefs.h"
#include "testutils.h"

Q_DECLARE_METATYPE(QSet<QByteArray>)

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

public:
    ClientSideTest():
        QObject()
    {
        qRegisterMetaType<Akonadi::Item>();
        qRegisterMetaType<Akonadi::Collection>();
        qRegisterMetaType<QSet<QByteArray> >();
    }

    void cleanup() {
        //cleanup
        Akonadi::ItemDeleteJob *deleteJob = new Akonadi::ItemDeleteJob(imapCollection);
        deleteJob->exec();
        Akonadi::ItemDeleteJob *deleteJob2 = new Akonadi::ItemDeleteJob(kolabCollection);
        deleteJob2->exec();
        QTest::qWait(TIMEOUT);
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
        QVERIFY(TestUtils::ensurePopulated(mInstance.identifier(), 6));

        Akonadi::CollectionPathResolver *resolver = new CollectionPathResolver(QLatin1String("res1/Calendar"), this);
        AKVERIFYEXEC(resolver);
        imapCollection = Akonadi::Collection( resolver->collection() );
        QVERIFY(imapCollection.isValid());

        kolabCollection = TestUtils::findCollection(mInstance.identifier(), "Calendar");
        QVERIFY(kolabCollection.isValid());
    }

    void testItemCreate()
    {
        KDateTime date(QDate(2013,10,10), KDateTime::ClockTime);
        date.setDateOnly(true);

        KCalCore::Event::Ptr event(new KCalCore::Event());
        event->setDtStart(date);
        Akonadi::Item createdItem;
        {
            Akonadi::Item item(event->mimeType());
            item.setPayload(event);
            Akonadi::ItemCreateJob *createJob = new Akonadi::ItemCreateJob(item, kolabCollection);
            QVERIFY(TestUtils::ensure(imapCollection, SIGNAL(itemAdded(Akonadi::Item,Akonadi::Collection)), createJob));
            createdItem = createJob->item();
            QVERIFY(createdItem.isValid());
        }

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
            //Check that the signal is NOT emitted within the timeout
            QVERIFY(!TestUtils::ensure(imapCollection, SIGNAL(itemAdded(Akonadi::Item,Akonadi::Collection)), createJob));
            //TODO akonadi currently doesn't support failing itemcreatejobs if the resource fails to store the item.
            // the item will simply remain dirty in the akonadi server
//             QVERIFY(createJob->error());
        }

        //Ensure the item has been removed by the kolabproxy
        {
            Akonadi::ItemFetchJob *fetchJob = new Akonadi::ItemFetchJob(kolabCollection);
            AKVERIFYEXEC(fetchJob);
            QCOMPARE(fetchJob->items().size(), 0);
        }
    }

    void testItemModify()
    {
        KDateTime date1(QDate(2013,10,10), KDateTime::ClockTime);
        date1.setDateOnly(true);
        KDateTime date2(QDate(2014,10,10), KDateTime::ClockTime);
        date2.setDateOnly(true);

        KCalCore::Event::Ptr event(new KCalCore::Event());
        event->setDtStart(date1);
        Akonadi::Item createdItem;
        {
            Akonadi::Item item(event->mimeType());
            item.setPayload(event);
            Akonadi::ItemCreateJob *createJob = new Akonadi::ItemCreateJob(item, kolabCollection);
            QVERIFY(TestUtils::ensure(imapCollection, SIGNAL(itemAdded(Akonadi::Item,Akonadi::Collection)), createJob));
            createdItem = createJob->item();
            QVERIFY(createdItem.isValid());
        }

        {
            event->setDtStart(date2);
            createdItem.setPayload(event);
            Akonadi::ItemModifyJob *modifyJob = new Akonadi::ItemModifyJob(createdItem);
            QVERIFY(TestUtils::ensure(imapCollection, SIGNAL(itemChanged(Akonadi::Item,QSet<QByteArray>)), modifyJob));
            Akonadi::Item modifiedItem = modifyJob->item();
            QVERIFY(modifiedItem.hasPayload<KCalCore::Event::Ptr>());
            QCOMPARE(modifiedItem.payload<KCalCore::Event::Ptr>()->dtStart().toString(), date2.toString());
        }

        {
            Akonadi::ItemFetchJob *fetchJob = new Akonadi::ItemFetchJob(imapCollection);
            fetchJob->fetchScope().fetchFullPayload(true);
            AKVERIFYEXEC(fetchJob);
            QCOMPARE(fetchJob->items().size(), 1);
            const Akonadi::Item item = fetchJob->items().first();
            QVERIFY(item.hasPayload<KMime::Message::Ptr>());
            Kolab::KolabObjectReader reader(item.payload<KMime::Message::Ptr>());
            QCOMPARE(reader.getEvent()->dtStart().toString(), date2.toString());
        }
        cleanup();
    }

    void testItemModifyFailure()
    {
        KDateTime date1(QDate(2013,10,10), KDateTime::ClockTime);
        date1.setDateOnly(true);

        KCalCore::Event::Ptr event(new KCalCore::Event());
        event->setDtStart(date1);
        Akonadi::Item createdItem;
        {
            Akonadi::Item item(event->mimeType());
            item.setPayload(event);
            Akonadi::ItemCreateJob *createJob = new Akonadi::ItemCreateJob(item, kolabCollection);
            QVERIFY(TestUtils::ensure(imapCollection, SIGNAL(itemAdded(Akonadi::Item,Akonadi::Collection)), createJob));
            createdItem = createJob->item();
            QVERIFY(createdItem.isValid());
        }

        {
            event->setDtStart(KDateTime());
            createdItem.setPayload(event);
            Akonadi::ItemModifyJob *modifyJob = new Akonadi::ItemModifyJob(createdItem);
            AKVERIFYEXEC(modifyJob);
            QTest::qWait(TIMEOUT);
            //FIXME this fails, no idea why
//             QVERIFY(!TestUtils::ensure(imapCollection, SIGNAL(itemChanged(Akonadi::Item,QSet<QByteArray>)), modifyJob));
        }

        //Ensure the change has been reverted for the kolab item
        {
            Akonadi::ItemFetchJob *fetchJob = new Akonadi::ItemFetchJob(kolabCollection);
            fetchJob->fetchScope().fetchFullPayload();
            AKVERIFYEXEC(fetchJob);
            QCOMPARE(fetchJob->items().size(), 1);
            const Akonadi::Item item = fetchJob->items().first();
            QVERIFY(item.hasPayload<KCalCore::Incidence::Ptr>());
            QCOMPARE(item.payload<KCalCore::Incidence::Ptr>()->dtStart().toString(), date1.toString());
        }
        cleanup();
    }

};

QTEST_AKONADIMAIN( ClientSideTest, NoGUI )

#include "clientsidetest.moc"
