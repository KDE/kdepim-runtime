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
#include <collectionannotationsattribute.h>
#include <kolabdefinitions.h> //libkolab

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
        annotations.insert("/vendor/kolab/folder-type", annotation);
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

    void ensureRecognizeNonMailFolder() {
        QCOMPARE(Kolab::folderTypeFromString(KOLAB_FOLDER_TYPE_EVENT), Kolab::EventType);
        QCOMPARE(Kolab::folderTypeFromString(KOLAB_FOLDER_TYPE_EVENT KOLAB_FOLDER_TYPE_DEFAULT_SUFFIX), Kolab::EventType);
        QCOMPARE(Kolab::folderTypeFromString(KOLAB_FOLDER_TYPE_TASK), Kolab::TaskType);
        QCOMPARE(Kolab::folderTypeFromString(KOLAB_FOLDER_TYPE_JOURNAL), Kolab::JournalType);
        QCOMPARE(Kolab::folderTypeFromString(KOLAB_FOLDER_TYPE_NOTE), Kolab::NoteType);
        QCOMPARE(Kolab::folderTypeFromString("freebusy"), Kolab::FreebusyType);
        QCOMPARE(Kolab::folderTypeFromString("configuration"), Kolab::ConfigurationType);
        QCOMPARE(Kolab::folderTypeFromString("file"), Kolab::FileType);
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
            if (Kolab::folderTypeFromString(col.attribute<CollectionAnnotationsAttribute>()->annotations().value(KOLAB_FOLDER_TYPE_ANNOTATION)) != Kolab::MailType) {
                //Kolab folder
                QVERIFY(col.attribute<Akonadi::EntityHiddenAttribute>());
            }
        }
    }

};

QTEST_AKONADIMAIN( ProxyIntegrationTest, NoGUI )

#include "proxyintegrationtest.moc"
