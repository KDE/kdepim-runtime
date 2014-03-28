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
#include <Akonadi/EntityHiddenAttribute>
#include <QDBusInterface>
#include <QSignalSpy>
#include <collectionannotationsattribute.h>
#include <kolabdefinitions.h> //libkolab

#include "testutils.h"

#include "../kolabdefs.h"

using namespace Akonadi;

class ProxyIntegrationTest : public QObject
{
    Q_OBJECT

    static Collection createCollection(const QString &name, const QByteArray &annotation)
    {
        Collection col;
        col.setName(name);
        QMap<QByteArray, QByteArray> annotations;
        Kolab::setFolderTypeAnnotation( annotations, annotation );
        col.addAttribute(new CollectionAnnotationsAttribute(annotations));
        col.setRights(Collection::CanCreateItem | Collection::CanChangeItem | Collection::CanDeleteItem | Collection::CanCreateCollection | Collection::CanChangeCollection | Collection::CanDeleteCollection);
        return col;
    }

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
        QVERIFY(TestUtils::ensurePopulated(mInstance.identifier(), 6));
    }

    void setupKolabProxy() {
        //Check that all collections in the kolab proxy have been created
        CollectionFetchJob *fetchJob = new CollectionFetchJob(Collection::root(), CollectionFetchJob::Recursive);
        fetchJob->fetchScope().setResource(mInstance.identifier());
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

    /**
     * All kolab folders, if handled by kolab proxy or not, should receive an entity hidden attribute
     */
    void ensureHiddenAttribute() {
        CollectionFetchJob *fetchJob = new CollectionFetchJob(Collection::root(), CollectionFetchJob::Recursive);
        fetchJob->fetchScope().setResource("akonadi_knut_resource_0");
        AKVERIFYEXEC(fetchJob);

        foreach (const Collection &col, fetchJob->collections()) {
            if (!col.attribute<CollectionAnnotationsAttribute>()) {
                continue;
            }
            if (Kolab::folderTypeFromString(Kolab::getFolderTypeAnnotation(col.attribute<CollectionAnnotationsAttribute>()->annotations())) != Kolab::MailType) {
                //Kolab folder
                QVERIFY(col.attribute<Akonadi::EntityHiddenAttribute>());
            }
        }
    }

    void testRemoval() {
        Akonadi::AgentInstance instance = AgentManager::self()->instance("akonadi_knut_resource_0");
        AgentManager::self()->removeInstance(instance);

        //Ensure all kolab collections are removed as well
        QTest::qWait(10);
        bool kolabCollectionsAreGone = false;
        Akonadi::Collection::List rootCollections;
        for (int i = 0; i < TIMEOUT/10; i++) {
            Akonadi::CollectionFetchJob *fetchJob = new Akonadi::CollectionFetchJob(Akonadi::Collection::root());
            AKVERIFYEXEC(fetchJob);
            rootCollections.clear();
            foreach (const Akonadi::Collection &col, fetchJob->collections()) {
                if (col.resource().contains("kolabproxy")) {
                    rootCollections << col;
                }
            }
            if (rootCollections.size() == 0) {
                kolabCollectionsAreGone = true;
                break;
            }
            QTest::qWait(10);
        }
        if (!kolabCollectionsAreGone) {
            kDebug() << rootCollections;
        }
        QVERIFY(kolabCollectionsAreGone);
    }

};

QTEST_AKONADIMAIN( ProxyIntegrationTest, NoGUI )

#include "proxyintegrationtest.moc"
