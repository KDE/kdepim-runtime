
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
#include <KCalCore/Event>
#include <KMime/Message>
#include <QDBusInterface>
#include <collectionannotationsattribute.h>
#include <kolabdefinitions.h> //libkolab
#include <kolabobject.h>

#include "../kolabdefs.h"

using namespace Akonadi;

class ImapSignalTest : public QObject
{
    Q_OBJECT

    AgentInstance mInstance;

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
    }

    void itemAddedSignal() {
        KCalCore::Event::Ptr event(new KCalCore::Event);
        event->setSummary("summary1");
        event->setDtStart(KDateTime(QDate(2013,02,01), QTime(1,1), KDateTime::ClockTime));

        Akonadi::CollectionPathResolver *resolver = new CollectionPathResolver(QLatin1String("res1/Calendar"), this);
        AKVERIFYEXEC(resolver);
        const Akonadi::Collection &imapCollection = Akonadi::Collection( resolver->collection() );

        //Create item in imap resource
        {
            const KMime::Message::Ptr &message = Kolab::KolabObjectWriter::writeEvent(event, Kolab::KolabV3, "Proxytest", QLatin1String("UTC") );
            Q_ASSERT(message);
            Akonadi::Item imapItem1;
            imapItem1.setMimeType( QLatin1String("message/rfc822") );
            imapItem1.setPayload( message );
            Akonadi::ItemCreateJob *createJob = new Akonadi::ItemCreateJob(imapItem1, imapCollection, this);
            AKVERIFYEXEC(createJob);
            //Wait for kolab item to get created
            QTest::qWait(100);
        }

        //ensure kolab equivalent gets created
        {
            Akonadi::Item expectedItem;
            expectedItem.setGid(event->instanceIdentifier());
            Akonadi::ItemFetchJob *fetchJob = new Akonadi::ItemFetchJob(expectedItem);
            AKVERIFYEXEC(fetchJob);
            QCOMPARE(fetchJob->items().size(), 1);
//             QCOMPARE(fetchJob->items().first().parentCollection().id() );
        }

        //create item again in imap resource (same gid), but with different content
        Akonadi::Item recreatdImapItem;
        {
            event->setSummary("summary2");
            const KMime::Message::Ptr &message = Kolab::KolabObjectWriter::writeEvent(event, Kolab::KolabV3, "Proxytest", QLatin1String("UTC") );
            Akonadi::Item imapItem1;
            imapItem1.setMimeType( QLatin1String("message/rfc822") );
            imapItem1.setPayload( message );
            Akonadi::ItemCreateJob *createJob = new Akonadi::ItemCreateJob(imapItem1, imapCollection, this);
            AKVERIFYEXEC(createJob);
            recreatdImapItem = createJob->item();
            //Wait for kolab item to get created
            QTest::qWait(100);
        }

        //ensure only one item with gid exists, and it's content and rid has been updated as expected
        {
            Akonadi::Item expectedItem;
            expectedItem.setGid(event->instanceIdentifier());
            Akonadi::ItemFetchJob *fetchJob = new Akonadi::ItemFetchJob(expectedItem);
            fetchJob->fetchScope().setFetchRemoteIdentification(true);
            fetchJob->fetchScope().fetchFullPayload(true);
            AKVERIFYEXEC(fetchJob);
            QCOMPARE(fetchJob->items().size(), 1);
            const Akonadi::Item item = fetchJob->items().first();
            QCOMPARE(item.remoteId().toLongLong(), recreatdImapItem.id());
            QVERIFY(item.hasPayload<KCalCore::Incidence::Ptr>());
            QCOMPARE(item.payload<KCalCore::Incidence::Ptr>()->summary(), event->summary());
        }

        //test retrieve items with duplicates
        {
            Akonadi::Collection kolabCollection;
            {
                Akonadi::Item expectedItem;
                expectedItem.setGid(event->instanceIdentifier());
                Akonadi::ItemFetchJob *fetchJob = new Akonadi::ItemFetchJob(expectedItem);
                fetchJob->fetchScope().setAncestorRetrieval(ItemFetchScope::Parent);
                AKVERIFYEXEC(fetchJob);
                QCOMPARE(fetchJob->items().size(), 1);
                kolabCollection = fetchJob->items().first().parentCollection();
            }

            mInstance.synchronize(); //triggers retireve items
            QTest::qWait(100);
            {
                Akonadi::ItemFetchJob *fetchJob = new Akonadi::ItemFetchJob(kolabCollection);
                fetchJob->fetchScope().setFetchRemoteIdentification(true);
                fetchJob->fetchScope().fetchFullPayload(true);
                AKVERIFYEXEC(fetchJob);
                QCOMPARE(fetchJob->items().size(), 1);
                const Akonadi::Item item = fetchJob->items().first();
                QCOMPARE(item.remoteId().toLongLong(), recreatdImapItem.id());
                QVERIFY(item.hasPayload<KCalCore::Incidence::Ptr>());
                QCOMPARE(item.payload<KCalCore::Incidence::Ptr>()->summary(), event->summary());
            }
        }

    }

};

QTEST_AKONADIMAIN( ImapSignalTest, NoGUI )

#include "imapsignaltest.moc"
