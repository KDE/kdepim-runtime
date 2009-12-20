/*  This file is part of the KDE project
    Copyright (C) 2009 Kevin Krammer <kevin.krammer@gmx.at>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "mappedvcardtest.h"

#include "mappedvcardstore.h"

#include "collectionfetchjob.h"
#include "itemcreatejob.h"
#include "itemdeletejob.h"
#include "itemfetchjob.h"
#include "itemmodifyjob.h"
#include "storecompactjob.h"

#include <kabc/addressee.h>
#include <kabc/vcardconverter.h>

#include <akonadi/itemfetchscope.h>

#include <KTemporaryFile>

#include <QtTest/QSignalSpy>
#include <qtest_kde.h>

QTEST_KDEMAIN( MappedVCardTest, NoGUI )

using namespace Akonadi::FileStore;

MappedVCardTest::MappedVCardTest()
  : mStore( 0 ), mFile( 0 ), mBenchmarkFile( 0 )
{
}

MappedVCardTest::~MappedVCardTest()
{
  delete mStore;
  delete mFile;
  delete mBenchmarkFile;
}

void MappedVCardTest::createBenchmarkFile()
{
  // only create once
  if ( mBenchmarkFile != 0 ) {
    return;
  }

  KABC::Addressee::List addresseeList;
  for ( int count = 0; count < 10000; ++count ) {
    KABC::Addressee addressee;
    addressee.setNameFromString( QString::fromUtf8( "Adressee, %1" ).arg( count ) );

    addresseeList << addressee;
  }

  KABC::VCardConverter converter;

  mBenchmarkFile = new KTemporaryFile();
  mBenchmarkFile->open();
  mBenchmarkFileName = mBenchmarkFile->fileName();
  mBenchmarkFile->write( converter.createVCards( addresseeList ) );
  mBenchmarkFile->close();
}

void MappedVCardTest::init()
{
  delete mStore;
  mStore = new MappedVCardStore();

  KABC::Addressee::List addresseeList;

  KABC::Addressee addressee1;
  addressee1.setNameFromString( QLatin1String( "Einstein, Albert" ) );
  addresseeList << addressee1;

  KABC::Addressee addressee2;
  addressee2.setNameFromString( QLatin1String( "Planck, Max" ) );
  addresseeList << addressee2;

  KABC::Addressee addressee3;
  addressee3.setNameFromString( QLatin1String( "Pauli, Wolfgang" ) );
  addresseeList << addressee3;

  KABC::VCardConverter converter;
  QByteArray data = converter.createVCards( addresseeList );

  delete mFile;
  mFile = new KTemporaryFile();
  mFile->open();
  mFileName = mFile->fileName();
  mFile->write( data );
  mFile->close();
}

void MappedVCardTest::testFetching()
{
  // for monitoring signals
  qRegisterMetaType<Akonadi::Collection::List>();
  qRegisterMetaType<Akonadi::Item::List>();

  // store does not have a file yet, fetching must fail with errors
  CollectionFetchJob *colFetch = mStore->fetchCollections();
  QVERIFY( colFetch != 0 );

  QSignalSpy *spy = new QSignalSpy( colFetch, SIGNAL( collectionsReceived( const Akonadi::Collection::List& ) ) );
  QVERIFY( spy->isValid() );

  bool result = colFetch->exec();

  QVERIFY( !result );
  QVERIFY( colFetch->error() == Job::InvalidStoreState );
  QVERIFY( spy->isEmpty() );
  delete spy;

  ItemFetchJob *itemFetch = mStore->fetchItems( mStore->topLevelCollection() );
  QVERIFY( itemFetch != 0 );

  spy = new QSignalSpy( itemFetch, SIGNAL( itemsReceived( const Akonadi::Item::List& ) ) );
  QVERIFY( spy->isValid() );

  result = itemFetch->exec();
  QVERIFY( !result );
  QVERIFY( itemFetch->error() == Job::InvalidStoreState );
  QVERIFY( spy->isEmpty() );
  delete spy;

  // set a file name, fetching must work now
  mStore->setFileName( mFileName );

  colFetch = mStore->fetchCollections();
  QVERIFY( colFetch != 0 );

  spy = new QSignalSpy( colFetch, SIGNAL( collectionsReceived( const Akonadi::Collection::List& ) ) );
  QVERIFY( spy->isValid() );

  result = colFetch->exec();

  QVERIFY( result );
  QCOMPARE( spy->count(), 1 );
  delete spy;

  const Akonadi::Collection::List collections = colFetch->collections();
  QCOMPARE( collections.count(), 1 );

  const Akonadi::Collection collection = collections[0];
  QVERIFY( collection == mStore->topLevelCollection() );

  itemFetch = mStore->fetchItems( mStore->topLevelCollection() );
  QVERIFY( itemFetch != 0 );
  QVERIFY( itemFetch->collection() == mStore->topLevelCollection() );

  spy = new QSignalSpy( itemFetch, SIGNAL( itemsReceived( const Akonadi::Item::List& ) ) );
  QVERIFY( spy->isValid() );

  result = itemFetch->exec();
  QVERIFY( result );
  QCOMPARE( spy->count(), 1 );
  delete spy;

  Akonadi::Item::List items = itemFetch->items();
  QCOMPARE( items.count(), 3 );

  foreach ( const Akonadi::Item &item, items ) {
    QCOMPARE( item.mimeType(), KABC::Addressee::mimeType() );

    itemFetch = mStore->fetchItem( item );
    QVERIFY( itemFetch != 0 );
    QVERIFY( itemFetch->item() == item );

    spy = new QSignalSpy( itemFetch, SIGNAL( itemsReceived( const Akonadi::Item::List& ) ) );
    QVERIFY( spy->isValid() );

    result = itemFetch->exec();
    QVERIFY( result );
    QCOMPARE( spy->count(), 1 );
    delete spy;

    Akonadi::Item::List items2 = itemFetch->items();
    QCOMPARE( items2.count(), 1 );

    const Akonadi::Item item2 = items2[0];
    QCOMPARE( item.remoteId(), item2.remoteId() );
    QCOMPARE( item2.mimeType(), KABC::Addressee::mimeType() );
  }

  itemFetch = mStore->fetchItems( mStore->topLevelCollection() );
  QVERIFY( itemFetch != 0 );

  itemFetch->fetchScope().fetchFullPayload();

  result = itemFetch->exec();
  QVERIFY( result );

  items = itemFetch->items();
  QCOMPARE( items.count(), 3 );

  foreach ( const Akonadi::Item &item, items ) {
    QCOMPARE( item.mimeType(), KABC::Addressee::mimeType() );
    QVERIFY( item.hasPayload<KABC::Addressee>() );

    itemFetch = mStore->fetchItem( item );
    QVERIFY( itemFetch != 0 );
    QVERIFY( itemFetch->item() == item );

    itemFetch->fetchScope().fetchFullPayload();

    result = itemFetch->exec();
    QVERIFY( result );

    Akonadi::Item::List items2 = itemFetch->items();
    QCOMPARE( items2.count(), 1 );

    const Akonadi::Item item2 = items2[0];
    QCOMPARE( item.remoteId(), item2.remoteId() );
    QCOMPARE( item2.mimeType(), KABC::Addressee::mimeType() );
    QVERIFY( item2.hasPayload<KABC::Addressee>() );
    QVERIFY( item.payload<KABC::Addressee>() == item2.payload<KABC::Addressee>() );
  }
}

void MappedVCardTest::testCreate()
{
  KABC::Addressee addressee;
  addressee.setNameFromString( QLatin1String( "Heisenberg, Werner" ) );

  Akonadi::Item item( KABC::Addressee::mimeType() );
  item.setPayload<KABC::Addressee>( addressee );

  // store does not have a file yet, create must fail with error
  ItemCreateJob *itemCreate = mStore->createItem( item, mStore->topLevelCollection() );
  QVERIFY( itemCreate != 0 );

  bool result = itemCreate->exec();

  QVERIFY( !result );
  QVERIFY( itemCreate->error() == Job::InvalidStoreState );

  // set a file name, create must work now
  mStore->setFileName( mFileName );

  itemCreate = mStore->createItem( item, mStore->topLevelCollection() );
  QVERIFY( itemCreate != 0 );

  result = itemCreate->exec();

  QVERIFY( result );

  Akonadi::Item item2 = itemCreate->item();
  QVERIFY( !item2.remoteId().isEmpty() );
  QVERIFY( item2.hasPayload<KABC::Addressee>() );
  QVERIFY( item2.payload<KABC::Addressee>() == addressee );

  // check that it can be fetched
  item = Akonadi::Item();
  item.setRemoteId( item2.remoteId() );
  ItemFetchJob *itemFetch = mStore->fetchItem( item );
  QVERIFY( itemFetch != 0 );
  itemFetch->fetchScope().fetchFullPayload();

  result = itemFetch->exec();

  QVERIFY( result );

  Akonadi::Item::List items = itemFetch->items();
  QCOMPARE( items.count(), 1 );

  Akonadi::Item item3 = items[0];
  QVERIFY( item3.hasPayload<KABC::Addressee>() );
  QVERIFY( item3.payload<KABC::Addressee>() == addressee );

  // delete the store and create a new one for the same file. new entry must still be available
  delete mStore;
  mStore = new MappedVCardStore();

  // set a file name, create must work now
  mStore->setFileName( mFileName );

  itemFetch = mStore->fetchItem( item );
  QVERIFY( itemFetch != 0 );
  itemFetch->fetchScope().fetchFullPayload();

  result = itemFetch->exec();

  QVERIFY( result );

  items = itemFetch->items();
  QCOMPARE( items.count(), 1 );

  item3 = items[0];
  QVERIFY( item3.hasPayload<KABC::Addressee>() );
  QVERIFY( item3.payload<KABC::Addressee>() == addressee );

  itemFetch = mStore->fetchItems( mStore->topLevelCollection() );
  QVERIFY( itemFetch != 0 );
  itemFetch->fetchScope().fetchFullPayload();

  result = itemFetch->exec();

  QVERIFY( result );

  items = itemFetch->items();
  QCOMPARE( items.count(), 4 );
}

void MappedVCardTest::testModify()
{
  mStore->setFileName( mFileName );

  ItemFetchJob *itemFetch = mStore->fetchItems( mStore->topLevelCollection() );
  QVERIFY( itemFetch != 0 );
  itemFetch->fetchScope().fetchFullPayload();

  bool result = itemFetch->exec();
  QVERIFY( result );

  Akonadi::Item::List items = itemFetch->items();
  QCOMPARE( items.count(), 3 );

  Akonadi::Item item = items[0];
  QVERIFY( item.hasPayload<KABC::Addressee>() );

  KABC::Addressee addressee = item.payload<KABC::Addressee>();

  Akonadi::Item item2 = item;
  addressee.setNameFromString( QLatin1String( "Foo, Bar" ) );
  item2.setPayload<KABC::Addressee>( addressee );

  const qint64 originalSize = mFile->size();

  ItemModifyJob *itemModify = mStore->modifyItem( item2 );
  QVERIFY( itemModify != 0 );

  result = itemModify->exec();
  QVERIFY( result );

  QCOMPARE( mFile->size(), originalSize );

  delete mStore;
  mStore = new MappedVCardStore();
  mStore->setFileName( mFileName );

  itemFetch = mStore->fetchItems( mStore->topLevelCollection() );
  QVERIFY( itemFetch != 0 );
  itemFetch->fetchScope().fetchFullPayload();

  result = itemFetch->exec();
  QVERIFY( result );

  Akonadi::Item::List items2 = itemFetch->items();
  QCOMPARE( items2.count(), 3 );

  Akonadi::Item item3;
  item3.setRemoteId( item.remoteId() );

  itemFetch = mStore->fetchItem( item3 );
  QVERIFY( itemFetch != 0 );
  itemFetch->fetchScope().fetchFullPayload();

  result = itemFetch->exec();
  QVERIFY( result );

  Akonadi::Item::List items3 = itemFetch->items();
  QCOMPARE( items3.count(), 1 );

  QVERIFY( items3[0].hasPayload<KABC::Addressee>() );

  KABC::Addressee addressee2 = items3[0].payload<KABC::Addressee>();
  QVERIFY( addressee2 == addressee );

  item2.setPayload<KABC::Addressee>( item.payload<KABC::Addressee>() );

  itemModify = mStore->modifyItem( item2 );
  QVERIFY( itemModify != 0 );

  result = itemModify->exec();
  QVERIFY( result );

  QCOMPARE( mFile->size(), originalSize );
}

void MappedVCardTest::testDelete()
{
  mStore->setFileName( mFileName );

  ItemFetchJob *itemFetch = mStore->fetchItems( mStore->topLevelCollection() );
  QVERIFY( itemFetch != 0 );
  itemFetch->fetchScope().fetchFullPayload();

  bool result = itemFetch->exec();
  QVERIFY( result );

  Akonadi::Item::List items = itemFetch->items();
  QCOMPARE( items.count(), 3 );

  Akonadi::Item item = items[0];
  QVERIFY( item.hasPayload<KABC::Addressee>() );

  const qint64 originalSize = mFile->size();

  ItemDeleteJob *itemDelete = mStore->deleteItem( item );
  QVERIFY( itemDelete != 0 );

  result = itemDelete->exec();
  QVERIFY( result );

  QCOMPARE( mFile->size(), originalSize );

  Akonadi::Item item3;
  item3.setRemoteId( item.remoteId() );

  itemFetch = mStore->fetchItem( item3 );
  QVERIFY( itemFetch != 0 );
  itemFetch->fetchScope().fetchFullPayload();

  result = itemFetch->exec();
  QVERIFY( !result );

  delete mStore;
  mStore = new MappedVCardStore();
  mStore->setFileName( mFileName );

  itemFetch = mStore->fetchItems( mStore->topLevelCollection() );
  QVERIFY( itemFetch != 0 );
  itemFetch->fetchScope().fetchFullPayload();

  result = itemFetch->exec();
  QVERIFY( result );

  Akonadi::Item::List items2 = itemFetch->items();
  QCOMPARE( items2.count(), 2 );

  itemFetch = mStore->fetchItem( item3 );
  QVERIFY( itemFetch != 0 );
  itemFetch->fetchScope().fetchFullPayload();

  result = itemFetch->exec();
  QVERIFY( !result );

  ItemCreateJob *itemCreate = mStore->createItem( item, mStore->topLevelCollection() );
  QVERIFY( itemCreate != 0 );

  result = itemCreate->exec();
  QVERIFY( result );

  itemFetch = mStore->fetchItem( item3 );
  QVERIFY( itemFetch != 0 );
  itemFetch->fetchScope().fetchFullPayload();

  result = itemFetch->exec();
  QVERIFY( result );

  items2 = itemFetch->items();
  QCOMPARE( items2.count(), 1 );

  QVERIFY( items2[0].hasPayload<KABC::Addressee>() );
  QVERIFY( items2[0].payload<KABC::Addressee>() == item.payload<KABC::Addressee>() );
}

void MappedVCardTest::testCompact()
{
  // must fail on closed store
  StoreCompactJob* compactJob = mStore->compactStore();
  QVERIFY( compactJob != 0 );

  bool result = compactJob->exec();
  QVERIFY( !result );

  mStore->setFileName( mFileName );

  ItemFetchJob *itemFetch = mStore->fetchItems( mStore->topLevelCollection() );
  QVERIFY( itemFetch != 0 );
  itemFetch->fetchScope().fetchFullPayload();

  result = itemFetch->exec();
  QVERIFY( result );

  Akonadi::Item::List items = itemFetch->items();
  QCOMPARE( items.count(), 3 );

  Akonadi::Item item = items[0];
  QVERIFY( item.hasPayload<KABC::Addressee>() );

  const qint64 originalSize = mFile->size();

  ItemDeleteJob *itemDelete = mStore->deleteItem( item );
  QVERIFY( itemDelete != 0 );

  result = itemDelete->exec();
  QVERIFY( result );

  QCOMPARE( mFile->size(), originalSize );

  compactJob = mStore->compactStore();

  result = compactJob->exec();
  QVERIFY( result );

  // mFile->size() isn't updated properly
  QVERIFY( QFileInfo( mFileName ).size() < originalSize );

  ItemCreateJob *createJob = mStore->createItem( item, mStore->topLevelCollection() );

  result = createJob->exec();
  QVERIFY( result );

  QCOMPARE( mFile->size(), originalSize );
}

void MappedVCardTest::benchmarkFetchAllItemsBasic()
{
  createBenchmarkFile();

  delete mStore;
  mStore = new MappedVCardStore();
  mStore->setFileName( mBenchmarkFileName );

  QBENCHMARK {
    ItemFetchJob *job = mStore->fetchItems( mStore->topLevelCollection() );
    job->exec();
  }
}

void MappedVCardTest::benchmarkFetchAllItemsFull()
{
  createBenchmarkFile();

  delete mStore;
  mStore = new MappedVCardStore();
  mStore->setFileName( mBenchmarkFileName );

  QBENCHMARK {
    ItemFetchJob *job = mStore->fetchItems( mStore->topLevelCollection() );
    job->fetchScope().fetchFullPayload();
    job->exec();
  }
}

void MappedVCardTest::benchmarkTraditionalParsing()
{
  createBenchmarkFile();

  mBenchmarkFile->open();

  KABC::VCardConverter converter;
  KABC::Addressee::List addresseeList;

  QBENCHMARK {
     mBenchmarkFile->seek( 0 );
     addresseeList = converter.parseVCards( mBenchmarkFile->readAll() );
  }
}

#include "mappedvcardtest.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
