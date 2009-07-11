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

#include <QDBusInterface>
#include <QTime>

#include <KDebug>

#include <Akonadi/AgentInstance>
#include <Akonadi/AgentManager>
#include <Akonadi/Control>
#include <akonadi/qtest_akonadi.h>

#define TIMES 100 // How many times to sync.
#define TIMEOUT 10 // How many seconds to wait before declaring the resource dead.

using namespace Akonadi;

void SyncTest::initTestCase()
{
  QVERIFY( Control::start() );
  QTest::qWait( 1000 );
}

void SyncTest::testSync()
{
  AgentInstance instance = AgentManager::self()->instance( "akonadi_maildir_resource_0" );
  QVERIFY( instance.isValid() );

  for( int i = 0; i < 100; i++ ) {
    QDBusInterface *interface = new QDBusInterface(
        QString::fromLatin1( "org.freedesktop.Akonadi.Resource.%1").arg( instance.identifier() ),
        "/", "org.freedesktop.Akonadi.Resource", QDBusConnection::sessionBus(), this );
    QVERIFY( interface->isValid() );
    QTime t;
    t.start();
    instance.synchronize();
    QVERIFY( QTest::kWaitForSignal( interface, SIGNAL(synchronized()), TIMEOUT * 1000 ) );
    kDebug() << "Sync attempt" << i << "in" << t.elapsed() << "ms.";
  }
}

QTEST_AKONADIMAIN( SyncTest, NoGUI )

#include "synctest.moc"
