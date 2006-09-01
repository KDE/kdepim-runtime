/*
    Copyright (c) 2006 Volker Krause <volker.krause@rwth-aachen.de>

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
#include <libakonadi/itemappendjob.h>
#include <libakonadi/itemstorejob.h>
#include <libakonadi/itemdeletejob.h>

#include <QSignalSpy>
#include <qtest_kde.h>

using namespace PIM;

QTEST_KDEMAIN( MonitorTest, NoGUI )

void MonitorTest::testMonitor()
{
  Monitor *monitor = new Monitor( this );
  monitor->monitorCollection( "res3", true );

  // monitor signals
  qRegisterMetaType<PIM::DataReference>("PIM::DataReference");
  QSignalSpy caspy( monitor, SIGNAL(collectionAdded(QByteArray)) );
  QSignalSpy cmspy( monitor, SIGNAL(collectionChanged(QByteArray)) );
  QSignalSpy crspy( monitor, SIGNAL(collectionRemoved(QByteArray)) );
  QSignalSpy iaspy( monitor, SIGNAL(itemAdded(PIM::DataReference)) );
  QSignalSpy imspy( monitor, SIGNAL(itemChanged(PIM::DataReference)) );
  QSignalSpy irspy( monitor, SIGNAL(itemRemoved(PIM::DataReference)) );

  QVERIFY( caspy.isValid() );
  QVERIFY( cmspy.isValid() );
  QVERIFY( crspy.isValid() );
  QVERIFY( iaspy.isValid() );
  QVERIFY( imspy.isValid() );
  QVERIFY( irspy.isValid() );

  // create a collection
  CollectionCreateJob *create = new CollectionCreateJob( "res3/monitor", this );
  QVERIFY( create->exec() );

  QCOMPARE( caspy.count(), 1 );
  QList<QVariant> arg = caspy.takeFirst();
  QCOMPARE( arg.at(0).toByteArray(), QByteArray( "res3/monitor" ) );

  QVERIFY( cmspy.isEmpty() );
  QVERIFY( crspy.isEmpty() );
  QVERIFY( iaspy.isEmpty() );
  QVERIFY( imspy.isEmpty() );
  QVERIFY( irspy.isEmpty() );

  // add an item
  ItemAppendJob *append = new ItemAppendJob( "res3/monitor", QByteArray(), "message/rfc822", this );
  QVERIFY( append->exec() );

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

  // ### no yet implemented
  /*QCOMPARE( cmspy.count(), 1 );
  arg = cmspy.takeFirst();
  QCOMPARE( arg.at(0).toByteArray(), QByteArray( "res3/monitor" ) );

  QCOMPARE( imspy.count(), 1 );
  arg = imspy.takeFirst();
  DataReference ref2 = qvariant_cast<DataReference>( arg.at(0) );
  QCOMPARE( ref, ref2 );*/

  QVERIFY( caspy.isEmpty() );
  QVERIFY( crspy.isEmpty() );
  QVERIFY( iaspy.isEmpty() );
  QVERIFY( irspy.isEmpty() );

  // delete an item
  ItemDeleteJob *del = new ItemDeleteJob( ref, this );
  QVERIFY( del->exec() );

  // ### not yet implemented
  /*QCOMPARE( cmspy.count(), 1 );
  arg = cmspy.takeFirst();
  QCOMPARE( arg.at(0).toByteArray(), QByteArray( "res3/monitor" ) );

  QCOMPARE( irspy.count(), 1 );
  arg = imspy.takeFirst();
  DataReference ref2 = qvariant_cast<DataReference>( arg.at(0) );
  QCOMPARE( ref, ref2 );*/

  QVERIFY( caspy.isEmpty() );
  QVERIFY( crspy.isEmpty() );
  QVERIFY( iaspy.isEmpty() );
  QVERIFY( imspy.isEmpty() );

  // delete a collection
  CollectionDeleteJob *cdel = new CollectionDeleteJob( "res3/monitor", this );
  QVERIFY( cdel->exec() );

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
