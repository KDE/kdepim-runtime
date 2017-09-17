/*
    Copyright 2009 Constantin Berzan <exit3219@gmail.com>

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

#include "synctest.h"

#include "maildirsettings.h" // generated

#include <QDBusInterface>

#include <QDebug>

#include <AkonadiCore/AgentInstance>
#include <AkonadiCore/AgentManager>
#include <AkonadiCore/ServerManager>
#include <AkonadiCore/Control>
#include <qtest_akonadi.h>
#include <QSignalSpy>
#include <AgentInstanceCreateJob>

#define TIMES 100 // How many times to sync.
#define TIMEOUT 10 // How many seconds to wait before declaring the resource dead.

using namespace Akonadi;

void SyncTest::initTestCase()
{
    AkonadiTest::checkTestIsIsolated();
    AgentType maildirType = AgentManager::self()->type(QStringLiteral("akonadi_maildir_resource"));
    AgentInstanceCreateJob *agentCreateJob = new AgentInstanceCreateJob(maildirType);
    QVERIFY(agentCreateJob->exec());
    mMaildirIdentifier = agentCreateJob->instance().identifier();

    QString service = QStringLiteral("org.freedesktop.Akonadi.Resource.") + mMaildirIdentifier;
    if (Akonadi::ServerManager::hasInstanceIdentifier()) {
        service += QLatin1Char('.') + Akonadi::ServerManager::instanceIdentifier();
    }

    OrgKdeAkonadiMaildirSettingsInterface interface(service, QStringLiteral("/"), QDBusConnection::sessionBus());
    QVERIFY(interface.isValid());
    const QString mailPath = QFINDTESTDATA("maildir");
    QVERIFY(!mailPath.isEmpty());
    QVERIFY(QFile::exists(mailPath));
    interface.setPath(mailPath);
}

void SyncTest::testSync()
{
    AgentInstance instance = AgentManager::self()->instance(mMaildirIdentifier);
    QVERIFY(instance.isValid());

    for (int i = 0; i < TIMES; ++i) {
        QDBusInterface interface(
                Akonadi::ServerManager::agentServiceName(Akonadi::ServerManager::Resource, mMaildirIdentifier),
                QStringLiteral("/"), QStringLiteral("org.freedesktop.Akonadi.Resource"), QDBusConnection::sessionBus(), this);
        QVERIFY(interface.isValid());
        QElapsedTimer t;
        t.start();
        instance.synchronize();
        QSignalSpy spy(&interface, SIGNAL(synchronized()));
        QVERIFY(spy.isValid());
        QVERIFY(spy.wait(TIMEOUT * 1000));
        qDebug() << "Sync attempt" << i << "in" << t.elapsed() << "ms.";
    }
}

QTEST_AKONADIMAIN(SyncTest)

