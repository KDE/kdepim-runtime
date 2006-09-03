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

#include "itemappendtest.h"
#include <libakonadi/itemappendjob.h>
#include <libakonadi/itemfetchjob.h>
#include <libakonadi/itemdeletejob.h>

#include <QDebug>

using namespace PIM;

#include <qtest_kde.h>

QTEST_KDEMAIN( ItemAppendTest, NoGUI )

void ItemAppendTest::testItemAppend()
{
  // item without remote id
  ItemAppendJob *job = new ItemAppendJob( "res1/foo", QByteArray(), "message/rfc822", this );
  QVERIFY( job->exec() );

  // item with remote id
  job = new ItemAppendJob( "res1/foo", QByteArray(), "message/rfc822", this );
  job->setRemoteId( "remote-id" );
  QVERIFY( job->exec() );

  // append/fetch/delete to collection with a space in the name
  job = new ItemAppendJob( "res2/space folder", QByteArray("some content"), "message/rfc822", this );
  QVERIFY( job->exec() );

  ItemFetchJob *fjob = new ItemFetchJob( "res2/space folder", this );
  QVERIFY( fjob->exec() );
  QCOMPARE( fjob->items().count(), 1 );
  DataReference ref = fjob->items().first()->reference();

  ItemDeleteJob *djob = new ItemDeleteJob( ref, this );
  QVERIFY( djob->exec() );

  fjob = new ItemFetchJob( "res2/space folder", this );
  QVERIFY( fjob->exec() );
  QVERIFY( fjob->items().isEmpty() );
}

void ItemAppendTest::testIllegalAppend()
{
  // adding item to non-existing collection
  ItemAppendJob *job = new ItemAppendJob( "I/dont/exist", QByteArray(), "message/rfc822", this );
  QVERIFY( !job->exec() );

  // adding item with non-existing mimetype
  job = new ItemAppendJob( "res1/foo", QByteArray(), "wrong/type", this );
  QVERIFY( !job->exec() );

  // adding item into a collection which can't handle items of this type
  job = new ItemAppendJob( "res1/foo/bla", QByteArray(), "message/rfc822", this );
  QEXPECT_FAIL( "", "Test not yet implemented in the server.", Continue );
  QVERIFY( !job->exec() );
}

#include "itemappendtest.moc"
