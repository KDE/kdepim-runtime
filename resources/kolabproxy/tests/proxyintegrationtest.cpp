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
#include <Akonadi/AgentInstanceCreateJob>
#include <Akonadi/CollectionFetchJob>
#include <Akonadi/CollectionFetchScope>
#include <Akonadi/AttributeFactory>
#include <QDBusInterface>
#include <collectionannotationsattribute.h>

using namespace Akonadi;

class ProxyIntegrationTest : public QObject
{
  Q_OBJECT
private slots:
    void initTestCase() {
        AkonadiTest::checkTestIsIsolated();
        AttributeFactory::registerAttribute<CollectionAnnotationsAttribute>();
    }

    static Collection createCollection(const QString &name, const QByteArray &annotation)
    {
        Collection col;
        col.setName(name);
        QMap<QByteArray, QByteArray> annotations;
        annotations.insert("/vendor/kolab/folder-type", annotation);
        col.addAttribute(new CollectionAnnotationsAttribute(annotations));
        col.setRights(Collection::CanCreateItem | Collection::CanChangeItem | Collection::CanDeleteItem | Collection::CanCreateCollection | Collection::CanChangeCollection | Collection::CanDeleteCollection);
        return col;
    }

    void setupKolabProxy() {
        const AgentType type = AgentManager::self()->type("akonadi_kolabproxy_resource");
        AgentInstanceCreateJob *agentCreateJob = new AgentInstanceCreateJob(type);
        AKVERIFYEXEC(agentCreateJob);
        AgentInstance instance = agentCreateJob->instance();

        //Wait for kolabproxy to create all folders
        QTest::qWait(1000);

        //The below is supposed to allow us to wait on the synchronization to complete, but that somehow crashes
// //             QDBusInterface *interface = new QDBusInterface(
// //                 QString::fromLatin1( "org.freedesktop.Akonadi.Resource.%1" ).arg( instance.identifier() ),
// //                 "/", "org.freedesktop.Akonadi.Resource", QDBusConnection::sessionBus(), this );
//         QDBusInterface *interface = new QDBusInterface(
//             ServerManager::agentServiceName(ServerManager::Resource, instance.identifier()),
//             "/", "org.freedesktop.Akonadi.Resource", QDBusConnection::sessionBus(), this);
//
//         QVERIFY( interface->isValid() );
//         instance.synchronize();
//         QVERIFY(QTest::kWaitForSignal(interface, SIGNAL(synchronized()), 5 * 1000));

        CollectionFetchJob *fetchJob = new CollectionFetchJob(Collection::root(), CollectionFetchJob::Recursive);
        fetchJob->fetchScope().setResource(instance.identifier());
        AKVERIFYEXEC(fetchJob);

        QList<Collection> expectedCollections;
        expectedCollections << createCollection("Calendar", "event");
        expectedCollections << createCollection("Tasks", "task");
        expectedCollections << createCollection("Journal", "journal");
        expectedCollections << createCollection("Notes", "note");
        expectedCollections << createCollection("Contact", "contact");

        foreach (const Collection &col, fetchJob->collections()) {
            if (!col.attribute<CollectionAnnotationsAttribute>()) {
                continue;
            }
            for (int i = 0; i < expectedCollections.size(); i++) {
                const Collection &expectedCol = expectedCollections.at(i);
//                 kDebug() << col.name() << expectedCol.name();
//                 kDebug() << col.attribute<CollectionAnnotationsAttribute>()->annotations();
//                 kDebug() << expectedCol.attribute<CollectionAnnotationsAttribute>()->annotations();
                if (col.name() == expectedCol.name()) {
                    kDebug() << " found " << col.name();
                    QCOMPARE(col.attribute<CollectionAnnotationsAttribute>()->annotations(), expectedCol.attribute<CollectionAnnotationsAttribute>()->annotations());
                    QCOMPARE(col.rights(), expectedCol.rights());
                    expectedCollections.removeAt(i);
                    break;
                }
            }
        }
        QCOMPARE(expectedCollections.size(), 0);
    }

};

QTEST_AKONADIMAIN( ProxyIntegrationTest, NoGUI )

#include "proxyintegrationtest.moc"