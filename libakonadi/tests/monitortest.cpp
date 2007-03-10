/*
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>

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

#include "monitortest.h"

#include <libakonadi/monitor.h>
#include <libakonadi/collectioncreatejob.h>
#include <libakonadi/collectiondeletejob.h>
#include <libakonadi/collectionlistjob.h>
#include <libakonadi/control.h>
#include <libakonadi/itemappendjob.h>
#include <libakonadi/itemstorejob.h>
#include <libakonadi/itemdeletejob.h>

#include <QtGui/QApplication>
#include <QtTest/QSignalSpy>
#include <qtest_kde.h>

using namespace Akonadi;

QTEST_KDEMAIN( MonitorTest, NoGUI )

static Collection res3;

void MonitorTest::initTestCase()
{
  Control::start();

  // get the collections we run the tests on
  CollectionListJob *job = new CollectionListJob( Collection::root(), CollectionListJob::Recursive );
  QVERIFY( job->exec() );
  Collection::List list = job->collections();
  foreach ( const Collection col, list )
    if ( col.name() == "res3" )
      res3 = col;
}

void MonitorTest::testMonitor()
{
  Monitor *monitor = new Monitor( this );
  monitor->monitorCollection( "res3", true );

  // monitor signals
  qRegisterMetaType<Akonadi::DataReference>("Akonadi::DataReference");
  QSignalSpy caspy( monitor, SIGNAL(collectionAdded(QString)) );
  QSignalSpy cmspy( monitor, SIGNAL(collectionChanged(QString)) );
  QSignalSpy crspy( monitor, SIGNAL(collectionRemoved(QString)) );
  QSignalSpy iaspy( monitor, SIGNAL(itemAdded(Akonadi::DataReference)) );
  QSignalSpy imspy( monitor, SIGNAL(itemChanged(Akonadi::DataReference)) );
  QSignalSpy irspy( monitor, SIGNAL(itemRemoved(Akonadi::DataReference)) );

  QVERIFY( caspy.isValid() );
  QVERIFY( cmspy.isValid() );
  QVERIFY( crspy.isValid() );
  QVERIFY( iaspy.isValid() );
  QVERIFY( imspy.isValid() );
  QVERIFY( irspy.isValid() );

  // create a collection
  CollectionCreateJob *create = new CollectionCreateJob( res3, "monitor", this );
  QVERIFY( create->exec() );
  Collection monitorCol = create->collection();
  QTest::qWait(1000); // make sure the DBus signal has been processed

  QCOMPARE( caspy.count(), 1 );
  QList<QVariant> arg = caspy.takeFirst();
  QCOMPARE( arg.at(0).toByteArray(), QByteArray( "res3/monitor" ) );

  QVERIFY( cmspy.isEmpty() );
  QVERIFY( crspy.isEmpty() );
  QVERIFY( iaspy.isEmpty() );
  QVERIFY( imspy.isEmpty() );
  QVERIFY( irspy.isEmpty() );

  // add an item
  ItemAppendJob *append = new ItemAppendJob( monitorCol, "message/rfc822", this );
  QVERIFY( append->exec() );
  QTest::qWait(1000);

  QCOMPARE( cmspy.count(), 1 );
  arg = cmspy.takeFirst();
  QCOMPARE( arg.at(0).toByteArray(), QByteArray( "res3/monitor" ) );

  QCOMPARE( iaspy.count(), 1 );
  arg = iaspy.takeFirst();
  DataReference ref = qvariant_cast<DataReference>( arg.at(0) );
  QVERIFY( !ref.isNull() );

  QVERIFY( caspy.isEmpty() );
  QVERIFY( crspy.isEmpty() );
  QVERIFY( imspy.isEmpty() );
  QVERIFY( irspy.isEmpty() );

  // modify an item
  ItemStoreJob *store = new ItemStoreJob( ref, this );
  store->setData( QByteArray( "some new content" ) );
  QVERIFY( store->exec() );
  QTest::qWait(1000);

  QCOMPARE( cmspy.count(), 1 );
  arg = cmspy.takeFirst();
  QCOMPARE( arg.at(0).toByteArray(), QByteArray( "res3/monitor" ) );

  QCOMPARE( imspy.count(), 1 );
  arg = imspy.takeFirst();
  DataReference ref2 = qvariant_cast<DataReference>( arg.at(0) );
  QCOMPARE( ref, ref2 );

  QVERIFY( caspy.isEmpty() );
  QVERIFY( crspy.isEmpty() );
  QVERIFY( iaspy.isEmpty() );
  QVERIFY( irspy.isEmpty() );

  // delete an item
  ItemDeleteJob *del = new ItemDeleteJob( ref, this );
  QVERIFY( del->exec() );
  QTest::qWait(1000);

  QCOMPARE( cmspy.count(), 1 );
  arg = cmspy.takeFirst();
  QCOMPARE( arg.at(0).toByteArray(), QByteArray( "res3/monitor" ) );
  cmspy.clear();

  QCOMPARE( irspy.count(), 1 );
  arg = irspy.takeFirst();
  ref2 = qvariant_cast<DataReference>( arg.at(0) );
  QCOMPARE( ref, ref2 );

  QVERIFY( caspy.isEmpty() );
  QVERIFY( crspy.isEmpty() );
  QVERIFY( iaspy.isEmpty() );
  QVERIFY( imspy.isEmpty() );
  imspy.clear();

  // delete a collection
  CollectionDeleteJob *cdel = new CollectionDeleteJob( monitorCol, this );
  QVERIFY( cdel->exec() );
  QTest::qWait(1000);

  QCOMPARE( crspy.count(), 1 );
  arg = crspy.takeFirst();
  QCOMPARE( arg.at(0).toByteArray(), QByteArray( "res3/monitor" ) );

  QVERIFY( caspy.isEmpty() );
  QVERIFY( cmspy.isEmpty() );
  QVERIFY( iaspy.isEmpty() );
  QVERIFY( imspy.isEmpty() );
  QVERIFY( irspy.isEmpty() );
}

#include "monitortest.moc"
