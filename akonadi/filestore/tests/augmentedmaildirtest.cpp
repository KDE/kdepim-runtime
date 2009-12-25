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

#include "augmentedmaildirtest.h"

#include "augmentedmaildirstore.h"

#include "collectionfetchjob.h"
#include "itemcreatejob.h"
#include "itemdeletejob.h"
#include "itemfetchjob.h"
#include "itemmodifyjob.h"
#include "storecompactjob.h"

#include <libmaildir/maildir.h>

#include <kmime/kmime_message.h>

#include <akonadi/entitydisplayattribute.h>
#include <akonadi/itemfetchscope.h>

#include <KIO/DeleteJob>
#include <KTempDir>

#include <QtTest/QSignalSpy>
#include <qtest_kde.h>

QTEST_KDEMAIN( AugmentedMailDirTest, NoGUI )

using namespace Akonadi::FileStore;

AugmentedMailDirTest::AugmentedMailDirTest()
  : mStore( 0 ), mDir( 0 )
{
  // for monitoring signals
  qRegisterMetaType<Akonadi::Collection::List>();
  qRegisterMetaType<Akonadi::Item::List>();
}

AugmentedMailDirTest::~AugmentedMailDirTest()
{
  delete mStore;
  KIO::Job *job = KIO::del( KUrl::fromPath( mDirName ), KIO::HideProgressInfo );
  job->exec();
  delete mDir;
}

void AugmentedMailDirTest::createMailsInDir( const QString &dirName  )
{
  KPIM::Maildir mailDir( dirName );
  QVERIFY( mailDir.create() );

  const QString dirTemplate = QLatin1String( "%1/akonadi" );
  QDir dir( dirName );
  QVERIFY( dir.mkpath( dirTemplate.arg( mailDir.path() ) ) );

  const QString attributeFileTemplate = dirTemplate + QLatin1String( "/%2.attributes" );
  const QString flagFileTemplate = dirTemplate + QLatin1String( "/%12.flags" );

  {
    KMime::Message::Ptr messagePtr( new KMime::Message() );
    messagePtr->from()->from7BitString( "augmentedmaildirtest@localhost" );
    messagePtr->to()->from7BitString( "test@example.com" );
    messagePtr->subject()->from7BitString( "Subject 1" );
    messagePtr->setBody( "Body of message 1" );
    messagePtr->messageID()->generate( "localhost.local" );
    QByteArray messageId = messagePtr->messageID()->identifier();
    QVERIFY( !messageId.isEmpty() );
    messagePtr->assemble();

    const QString entryName = mailDir.addEntry( messagePtr->encodedContent() );
    QVERIFY( !entryName.isEmpty() );

    QSet<QByteArray> flags;
    flags << "\\Seen";

    QFile *file = new QFile( flagFileTemplate.arg( mailDir.path() ).arg( entryName ) );
    QVERIFY( file->open( QIODevice::WriteOnly ) );

    QDataStream *stream = new QDataStream( file );
    *stream << flags;

    delete stream;
    delete file;

    Akonadi::EntityDisplayAttribute *displayAttribute = new Akonadi::EntityDisplayAttribute();
    displayAttribute->setDisplayName( QLatin1String( "Test Message 1" ) );

    file = new QFile( attributeFileTemplate.arg( mailDir.path() ).arg( entryName ) );
    QVERIFY( file->open( QIODevice::WriteOnly ) );

    stream = new QDataStream( file );
    *stream << displayAttribute->type() << displayAttribute->serialized();

    delete displayAttribute;
    delete stream;
    delete file;
  }

  {
    KMime::Message::Ptr messagePtr( new KMime::Message() );
    messagePtr->from()->from7BitString( "test@example.com" );
    messagePtr->to()->from7BitString( "augmentedmaildirtest@localhost" );
    messagePtr->subject()->from7BitString( "Subject 2" );
    messagePtr->setBody( "Body of message 2" );
    messagePtr->messageID()->generate( "localhost.local" );
    QByteArray messageId = messagePtr->messageID()->identifier();
    QVERIFY( !messageId.isEmpty() );
    messagePtr->assemble();

    const QString entryName = mailDir.addEntry( messagePtr->encodedContent() );
    QVERIFY( !entryName.isEmpty() );

    QSet<QByteArray> flags;

    QFile *file = new QFile( flagFileTemplate.arg( mailDir.path() ).arg( entryName ) );
    QVERIFY( file->open( QIODevice::WriteOnly ) );

    QDataStream *stream = new QDataStream( file );
    *stream << flags;

    delete stream;
    delete file;

    Akonadi::EntityDisplayAttribute *displayAttribute = new Akonadi::EntityDisplayAttribute();
    displayAttribute->setDisplayName( QLatin1String( "Test Message 2" ) );

    file = new QFile( attributeFileTemplate.arg( mailDir.path() ).arg( entryName ) );
    QVERIFY( file->open( QIODevice::WriteOnly ) );

    stream = new QDataStream( file );
    *stream << displayAttribute->type() << displayAttribute->serialized();

    delete displayAttribute;
    delete stream;
    delete file;
  }

  {
    KMime::Message::Ptr messagePtr( new KMime::Message() );
    messagePtr->from()->from7BitString( "test@example.com" );
    messagePtr->to()->from7BitString( "augmentedmaildirtest@localhost" );
    messagePtr->subject()->from7BitString( "Subject 3" );
    messagePtr->setBody( "Body of message 3" );
    messagePtr->messageID()->generate( "localhost.local" );
    QByteArray messageId = messagePtr->messageID()->identifier();
    QVERIFY( !messageId.isEmpty() );
    messagePtr->assemble();

    const QString entryName = mailDir.addEntry( messagePtr->encodedContent() );
    QVERIFY( !entryName.isEmpty() );

    QSet<QByteArray> flags;
    flags << "\\Seen" << "\\Answered";

    QFile *file = new QFile( flagFileTemplate.arg( mailDir.path() ).arg( entryName ) );
    QVERIFY( file->open( QIODevice::WriteOnly ) );

    QDataStream *stream = new QDataStream( file );
    *stream << flags;

    delete stream;
    delete file;

    Akonadi::EntityDisplayAttribute *displayAttribute = new Akonadi::EntityDisplayAttribute();
    displayAttribute->setDisplayName( QLatin1String( "Test Message 3" ) );

    file = new QFile( attributeFileTemplate.arg( mailDir.path() ).arg( entryName ) );
    QVERIFY( file->open( QIODevice::WriteOnly ) );

    stream = new QDataStream( file );
    *stream << displayAttribute->type() << displayAttribute->serialized();

    delete displayAttribute;
    delete stream;
    delete file;
  }
}

void AugmentedMailDirTest::init()
{
  delete mStore;
  mStore = new AugmentedMailDirStore();

  if ( !mDirName.isEmpty() ) {
    KIO::Job *job = KIO::del( KUrl::fromPath( mDirName ), KIO::HideProgressInfo );
    job->exec();
  }
  delete mDir;

  mDir = new KTempDir();
  mDirName = mDir->name();
  QVERIFY( mDir->exists() );

  // create top level maildir
  createMailsInDir( mDirName );

  KPIM::Maildir topLevelMailDir( mDirName );
  QVERIFY( topLevelMailDir.isValid() );

  QString subFolder = topLevelMailDir.addSubFolder( QLatin1String( "Child Folder 1" ) );
  createMailsInDir( subFolder );
  KPIM::Maildir firstLevelMailDir( subFolder );
  QVERIFY( firstLevelMailDir.isValid() );

  subFolder = topLevelMailDir.addSubFolder( QLatin1String( "Child Folder 2" ) );
  firstLevelMailDir = KPIM::Maildir( subFolder );
  QVERIFY( firstLevelMailDir.isValid() );

  subFolder = firstLevelMailDir.addSubFolder( QLatin1String( "Child Folder 2.1" ) );
  KPIM::Maildir secondLevelMailDir( subFolder );
  QVERIFY( secondLevelMailDir.isValid() );

  subFolder = firstLevelMailDir.addSubFolder( QLatin1String( "Child Folder 2.2" ) );
  createMailsInDir( subFolder );
  secondLevelMailDir = KPIM::Maildir( subFolder );
  QVERIFY( secondLevelMailDir.isValid() );

  subFolder = topLevelMailDir.addSubFolder( QLatin1String( "Child Folder 3" ) );
  createMailsInDir( subFolder );
  firstLevelMailDir = KPIM::Maildir( subFolder );
  QVERIFY( firstLevelMailDir.isValid() );

  subFolder = firstLevelMailDir.addSubFolder( QLatin1String( "Child Folder 3.1" ) );
  secondLevelMailDir = KPIM::Maildir( subFolder );
  QVERIFY( secondLevelMailDir.isValid() );

  subFolder = firstLevelMailDir.addSubFolder( QLatin1String( "Child Folder 3.2" ) );
  createMailsInDir( subFolder );
  secondLevelMailDir = KPIM::Maildir( subFolder );
  QVERIFY( secondLevelMailDir.isValid() );
}

void AugmentedMailDirTest::testFetching()
{
  // store does not have a file yet, fetching must fail with errors
  CollectionFetchJob *colFetch = mStore->fetchCollections( mStore->topLevelCollection(), CollectionFetchJob::Base );
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
  mStore->setFileName( mDirName );

  colFetch = mStore->fetchCollections( mStore->topLevelCollection(), CollectionFetchJob::Base );
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
    QCOMPARE( item.mimeType(), KMime::Message::mimeType() );

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
    QCOMPARE( item2.mimeType(), KMime::Message::mimeType() );
  }

  itemFetch = mStore->fetchItems( mStore->topLevelCollection() );
  QVERIFY( itemFetch != 0 );

  itemFetch->fetchScope().fetchFullPayload();
  itemFetch->fetchScope().fetchAllAttributes();

  result = itemFetch->exec();
  QVERIFY( result );

  items = itemFetch->items();
  QCOMPARE( items.count(), 3 );

  foreach ( const Akonadi::Item &item, items ) {
    QCOMPARE( item.mimeType(), KMime::Message::mimeType() );
    QVERIFY( item.hasPayload<KMime::Message::Ptr>() );
    QVERIFY( item.hasAttribute<Akonadi::EntityDisplayAttribute>() );
    QVERIFY( !item.attribute<Akonadi::EntityDisplayAttribute>()->displayName().isEmpty() );

    itemFetch = mStore->fetchItem( item );
    QVERIFY( itemFetch != 0 );
    QVERIFY( itemFetch->item() == item );

    itemFetch->fetchScope().fetchFullPayload();
    itemFetch->fetchScope().fetchAllAttributes();

    result = itemFetch->exec();
    QVERIFY( result );

    Akonadi::Item::List items2 = itemFetch->items();
    QCOMPARE( items2.count(), 1 );

    const Akonadi::Item item2 = items2[0];
    QCOMPARE( item.remoteId(), item2.remoteId() );
    QCOMPARE( item2.mimeType(), KMime::Message::mimeType() );
    QVERIFY( item2.hasPayload<KMime::Message::Ptr>() );
    QVERIFY( item.payload<KMime::Message::Ptr>()->decodedContent() ==
             item2.payload<KMime::Message::Ptr>()->decodedContent() );
    QVERIFY( item2.hasAttribute<Akonadi::EntityDisplayAttribute>() );
    QVERIFY( item.attribute<Akonadi::EntityDisplayAttribute>()->displayName() ==
             item2.attribute<Akonadi::EntityDisplayAttribute>()->displayName() );
  }

  colFetch = mStore->fetchCollections( mStore->topLevelCollection(), CollectionFetchJob::FirstLevel );
  QVERIFY( colFetch != 0 );

  result = colFetch->exec();
  QVERIFY( result );

  const Akonadi::Collection::List firstLevelCollections = colFetch->collections();
  QCOMPARE( firstLevelCollections.count(), 3 );

  colFetch = mStore->fetchCollections( firstLevelCollections[ 0 ], CollectionFetchJob::FirstLevel );
  QVERIFY( colFetch != 0 );

  result = colFetch->exec();
  QVERIFY( result );

  Akonadi::Collection::List secondLevelCollections = colFetch->collections();
  QCOMPARE( secondLevelCollections.count(), 0 );

  colFetch = mStore->fetchCollections( firstLevelCollections[ 1 ], CollectionFetchJob::FirstLevel );
  QVERIFY( colFetch != 0 );

  result = colFetch->exec();
  QVERIFY( result );

  secondLevelCollections = colFetch->collections();
  QCOMPARE( secondLevelCollections.count(), 2 );

  colFetch = mStore->fetchCollections( firstLevelCollections[ 2 ], CollectionFetchJob::FirstLevel );
  QVERIFY( colFetch != 0 );

  result = colFetch->exec();
  QVERIFY( result );

  secondLevelCollections = colFetch->collections();
  QCOMPARE( secondLevelCollections.count(), 2 );

  colFetch = mStore->fetchCollections( firstLevelCollections[ 2 ], CollectionFetchJob::Base );
  QVERIFY( colFetch != 0 );

  result = colFetch->exec();
  QVERIFY( result );

  QCOMPARE( colFetch->collections().count(), 1 );
  Akonadi::Collection thirdChildCollection = colFetch->collections()[ 0 ];
  QVERIFY( thirdChildCollection == firstLevelCollections[ 2 ] );

  colFetch = mStore->fetchCollections( mStore->topLevelCollection(), CollectionFetchJob::Recursive );
  QVERIFY( colFetch != 0 );

  result = colFetch->exec();
  QVERIFY( result );

  Akonadi::Collection::List allChildCollections = colFetch->collections();
  QCOMPARE( allChildCollections.count(), 7 );

  colFetch = mStore->fetchCollections( firstLevelCollections[ 2 ], CollectionFetchJob::Recursive );
  QVERIFY( colFetch != 0 );

  result = colFetch->exec();
  QVERIFY( result );

  allChildCollections = colFetch->collections();
  QCOMPARE( allChildCollections.count(), 2 );
  for ( int i = 0; i < 2; ++i ) {
    QVERIFY( allChildCollections[ i ].remoteId() == secondLevelCollections[ i ].remoteId() );
  }
}

void AugmentedMailDirTest::testCreate()
{
  KMime::Message::Ptr messagePtr( new KMime::Message() );
  messagePtr->from()->from7BitString( "augmentedmaildirtest@localhost" );
  messagePtr->subject()->from7BitString( "Testing item create" );
  messagePtr->setBody( "Appending message by using ItemCreateJob" );
  messagePtr->messageID()->generate( "localhost.local" );
  messagePtr->assemble();

  Akonadi::Item item( KMime::Message::mimeType() );
  item.setPayload<KMime::Message::Ptr>( messagePtr );

  Akonadi::Item::Flags flags;
  flags << "\\Draft";
  item.setFlags( flags );

  Akonadi::EntityDisplayAttribute *displayAttribute = item.attribute<Akonadi::EntityDisplayAttribute>(Akonadi::Item::AddIfMissing);
  displayAttribute->setDisplayName( QLatin1String( "Draft message 1" ) );

  // store does not have a file yet, create must fail with error
  ItemCreateJob *itemCreate = mStore->createItem( item, mStore->topLevelCollection() );
  QVERIFY( itemCreate != 0 );

  bool result = itemCreate->exec();

  QVERIFY( !result );
  QVERIFY( itemCreate->error() == Job::InvalidStoreState );

  // set a file name, create must work now
  mStore->setFileName( mDirName );

  itemCreate = mStore->createItem( item, mStore->topLevelCollection() );
  QVERIFY( itemCreate != 0 );

  result = itemCreate->exec();

  QVERIFY( result );

  Akonadi::Item item2 = itemCreate->item();
  QVERIFY( !item2.remoteId().isEmpty() );
  QVERIFY( item2.hasPayload<KMime::Message::Ptr>() );
  QVERIFY( item2.payload<KMime::Message::Ptr>()->encodedContent() == messagePtr->encodedContent() );
  QVERIFY( item2.hasAttribute<Akonadi::EntityDisplayAttribute>() );
  QVERIFY( item2.attribute<Akonadi::EntityDisplayAttribute>()->displayName() ==
           item.attribute<Akonadi::EntityDisplayAttribute>()->displayName() );

  // check that it can be fetched
  item = Akonadi::Item();
  item.setRemoteId( item2.remoteId() );
  ItemFetchJob *itemFetch = mStore->fetchItem( item );
  QVERIFY( itemFetch != 0 );
  itemFetch->fetchScope().fetchFullPayload();
  itemFetch->fetchScope().fetchAllAttributes();

  result = itemFetch->exec();

  QVERIFY( result );

  Akonadi::Item::List items = itemFetch->items();
  QCOMPARE( items.count(), 1 );

  Akonadi::Item item3 = items[0];
  QVERIFY( item3.hasPayload<KMime::Message::Ptr>() );
  QVERIFY( item3.payload<KMime::Message::Ptr>()->encodedContent() == messagePtr->encodedContent() );
  QVERIFY( item3.hasAttribute<Akonadi::EntityDisplayAttribute>() );
  QVERIFY( item3.attribute<Akonadi::EntityDisplayAttribute>()->displayName() ==
           item2.attribute<Akonadi::EntityDisplayAttribute>()->displayName() );

  // delete the store and create a new one for the same file. new entry must still be available
  delete mStore;
  mStore = new AugmentedMailDirStore();

  // set a file name, create must work now
  mStore->setFileName( mDirName );

  itemFetch = mStore->fetchItem( item );
  QVERIFY( itemFetch != 0 );
  itemFetch->fetchScope().fetchFullPayload();
  itemFetch->fetchScope().fetchAllAttributes();

  result = itemFetch->exec();

  QVERIFY( result );

  items = itemFetch->items();
  QCOMPARE( items.count(), 1 );

  item3 = items[0];
  QVERIFY( item3.hasPayload<KMime::Message::Ptr>() );
  QVERIFY( item3.payload<KMime::Message::Ptr>()->encodedContent() == messagePtr->encodedContent() );
  QVERIFY( item3.hasAttribute<Akonadi::EntityDisplayAttribute>() );
  QVERIFY( item3.attribute<Akonadi::EntityDisplayAttribute>()->displayName() ==
           item2.attribute<Akonadi::EntityDisplayAttribute>()->displayName() );

  itemFetch = mStore->fetchItems( mStore->topLevelCollection() );
  QVERIFY( itemFetch != 0 );
  itemFetch->fetchScope().fetchFullPayload();

  result = itemFetch->exec();

  QVERIFY( result );

  items = itemFetch->items();
  QCOMPARE( items.count(), 4 );
}

void AugmentedMailDirTest::testModify()
{
  mStore->setFileName( mDirName );

  ItemFetchJob *itemFetch = mStore->fetchItems( mStore->topLevelCollection() );
  QVERIFY( itemFetch != 0 );
  itemFetch->fetchScope().fetchFullPayload();
  itemFetch->fetchScope().fetchAllAttributes();

  bool result = itemFetch->exec();
  QVERIFY( result );

  Akonadi::Item::List items = itemFetch->items();
  QCOMPARE( items.count(), 3 );

  Akonadi::Item item = items[0];
  QVERIFY( item.hasPayload<KMime::Message::Ptr>() );

  KMime::Message::Ptr messagePtr = item.payload<KMime::Message::Ptr>();
  const QByteArray originalSubject = messagePtr->subject()->as7BitString( false );

  QVERIFY( item.hasAttribute<Akonadi::EntityDisplayAttribute>() );
  const QString originalDisplayName = item.attribute<Akonadi::EntityDisplayAttribute>()->displayName();

  const Akonadi::Item::Flags originalFlags = item.flags();

  Akonadi::Item item2 = item;
  messagePtr->subject()->from7BitString( "Modified message" );
  messagePtr->assemble();
  item2.setPayload<KMime::Message::Ptr>( messagePtr );

  ItemModifyJob *itemModify = mStore->modifyItem( item2 );
  QVERIFY( itemModify != 0 );

  result = itemModify->exec();
  QVERIFY( result );

  delete mStore;
  mStore = new AugmentedMailDirStore();
  mStore->setFileName( mDirName );

  itemFetch = mStore->fetchItems( mStore->topLevelCollection() );
  QVERIFY( itemFetch != 0 );
  itemFetch->fetchScope().fetchFullPayload();
  itemFetch->fetchScope().fetchAllAttributes();

  result = itemFetch->exec();
  QVERIFY( result );

  Akonadi::Item::List items2 = itemFetch->items();
  QCOMPARE( items2.count(), 3 );

  Akonadi::Item item3;
  item3.setRemoteId( item.remoteId() );

  itemFetch = mStore->fetchItem( item3 );
  QVERIFY( itemFetch != 0 );
  itemFetch->fetchScope().fetchFullPayload();
  itemFetch->fetchScope().fetchAllAttributes();

  result = itemFetch->exec();
  QVERIFY( result );

  Akonadi::Item::List items3 = itemFetch->items();
  QCOMPARE( items3.count(), 1 );

  QVERIFY( items3[0].hasPayload<KMime::Message::Ptr>() );

  KMime::Message::Ptr messagePtr2 = items3[0].payload<KMime::Message::Ptr>();

  QVERIFY( messagePtr2->encodedContent() == messagePtr->encodedContent() );

  QVERIFY( items3[0].hasAttribute<Akonadi::EntityDisplayAttribute>() );
  QCOMPARE( items3[0].attribute<Akonadi::EntityDisplayAttribute>()->displayName(),
            originalDisplayName );
  QCOMPARE( items3[0].flags(), originalFlags );

  messagePtr->subject()->from7BitString( originalSubject );
  messagePtr->assemble();
  item2.setPayload<KMime::Message::Ptr>( messagePtr );

  itemModify = mStore->modifyItem( item2 );
  QVERIFY( itemModify != 0 );

  result = itemModify->exec();
  QVERIFY( result );

  itemFetch = mStore->fetchItem( item3 );
  QVERIFY( itemFetch != 0 );
  itemFetch->fetchScope().fetchFullPayload();
  itemFetch->fetchScope().fetchAllAttributes();

  result = itemFetch->exec();
  QVERIFY( result );

  items3 = itemFetch->items();
  QCOMPARE( items3.count(), 1 );

  QVERIFY( items3[0].hasPayload<KMime::Message::Ptr>() );

  KMime::Message::Ptr messagePtr3 = items3[0].payload<KMime::Message::Ptr>();
  QVERIFY( messagePtr3->subject()->as7BitString( false ) == originalSubject );

  QVERIFY( items3[0].hasAttribute<Akonadi::EntityDisplayAttribute>() );
  QCOMPARE( items3[0].attribute<Akonadi::EntityDisplayAttribute>()->displayName(),
            originalDisplayName );
  QCOMPARE( items3[0].flags(), originalFlags );

  const QString modifiedDisplayName = QLatin1String( "Modified Message" );
  item2.attribute<Akonadi::EntityDisplayAttribute>()->setDisplayName( modifiedDisplayName );

  itemModify = mStore->modifyItem( item2 );
  QVERIFY( itemModify != 0 );
  itemModify->setIgnorePayload( true );

  result = itemModify->exec();
  QVERIFY( result );

  itemFetch = mStore->fetchItem( item3 );
  QVERIFY( itemFetch != 0 );
  itemFetch->fetchScope().fetchAllAttributes();

  result = itemFetch->exec();
  QVERIFY( result );

  items3 = itemFetch->items();
  QCOMPARE( items3.count(), 1 );

  QVERIFY( !items3[0].hasPayload<KMime::Message::Ptr>() );

  QVERIFY( items3[0].hasAttribute<Akonadi::EntityDisplayAttribute>() );
  QCOMPARE( items3[0].attribute<Akonadi::EntityDisplayAttribute>()->displayName(),
            modifiedDisplayName );
  QCOMPARE( items3[0].flags(), originalFlags );

  Akonadi::Item::Flags modifiedFlags = originalFlags;
  modifiedFlags << "\\Deleted";
  QVERIFY( modifiedFlags != originalFlags );

  item2.setFlags( modifiedFlags );
  itemModify = mStore->modifyItem( item2 );
  QVERIFY( itemModify != 0 );
  itemModify->setIgnorePayload( true );

  result = itemModify->exec();
  QVERIFY( result );

  itemFetch = mStore->fetchItem( item3 );
  QVERIFY( itemFetch != 0 );

  result = itemFetch->exec();
  QVERIFY( result );

  items3 = itemFetch->items();
  QCOMPARE( items3.count(), 1 );

  QVERIFY( !items3[0].hasPayload<KMime::Message::Ptr>() );

  QVERIFY( !items3[0].hasAttribute<Akonadi::EntityDisplayAttribute>() );
  QCOMPARE( items3[0].flags(), modifiedFlags );
}

void AugmentedMailDirTest::testDelete()
{
  mStore->setFileName( mDirName );

  ItemFetchJob *itemFetch = mStore->fetchItems( mStore->topLevelCollection() );
  QVERIFY( itemFetch != 0 );
  itemFetch->fetchScope().fetchFullPayload();
  itemFetch->fetchScope().fetchAllAttributes();

  bool result = itemFetch->exec();
  QVERIFY( result );

  Akonadi::Item::List items = itemFetch->items();
  QCOMPARE( items.count(), 3 );

  Akonadi::Item item = items[0];
  QVERIFY( item.hasPayload<KMime::Message::Ptr>() );

  ItemDeleteJob *itemDelete = mStore->deleteItem( item );
  QVERIFY( itemDelete != 0 );

  result = itemDelete->exec();
  QVERIFY( result );

  delete mStore;
  mStore = new AugmentedMailDirStore();
  mStore->setFileName( mDirName );

  itemFetch = mStore->fetchItems( mStore->topLevelCollection() );
  QVERIFY( itemFetch != 0 );
  itemFetch->fetchScope().fetchFullPayload();
  itemFetch->fetchScope().fetchAllAttributes();

  result = itemFetch->exec();
  QVERIFY( result );

  Akonadi::Item::List items2 = itemFetch->items();
  QCOMPARE( items2.count(), 2 );

  Akonadi::Item item2;
  item2.setRemoteId( item.remoteId() );

  itemFetch = mStore->fetchItem( item2 );
  QVERIFY( itemFetch != 0 );
  itemFetch->fetchScope().fetchFullPayload();
  itemFetch->fetchScope().fetchAllAttributes();

  result = itemFetch->exec();
  QVERIFY( !result );

  Akonadi::Item item3( KMime::Message::mimeType() );
  item3.setPayload<KMime::Message::Ptr>( item.payload<KMime::Message::Ptr>() );
  const Akonadi::Attribute::List attributes = item.attributes();
  foreach ( const Akonadi::Attribute *attribute, attributes ) {
    item3.addAttribute( attribute->clone() );
  }
  item3.setFlags( item.flags() );

  ItemCreateJob *itemCreate = mStore->createItem( item3, mStore->topLevelCollection() );
  QVERIFY( itemCreate != 0 );

  result = itemCreate->exec();
  QVERIFY( result );

  item2 = itemCreate->item();
  itemFetch = mStore->fetchItem( item2 );
  QVERIFY( itemFetch != 0 );
  itemFetch->fetchScope().fetchFullPayload();
  itemFetch->fetchScope().fetchAllAttributes();

  result = itemFetch->exec();
  QVERIFY( result );

  items2 = itemFetch->items();
  QCOMPARE( items2.count(), 1 );

  QVERIFY( items2[0].hasPayload<KMime::Message::Ptr>() );
  QVERIFY( items2[0].payload<KMime::Message::Ptr>()->encodedContent() ==
           item.payload<KMime::Message::Ptr>()->encodedContent() );
  QVERIFY( items2[0].hasAttribute<Akonadi::EntityDisplayAttribute>() );
  QVERIFY( items2[0].attribute<Akonadi::EntityDisplayAttribute>()->displayName() ==
           item.attribute<Akonadi::EntityDisplayAttribute>()->displayName() );
  QVERIFY( items2[0].flags() == item.flags() );
}

void AugmentedMailDirTest::testCompact()
{
  // must fail on closed store
  StoreCompactJob* compactJob = mStore->compactStore();
  QVERIFY( compactJob != 0 );

  bool result = compactJob->exec();
  QVERIFY( !result );

  mStore->setFileName( mDirName );

  ItemFetchJob *itemFetch = mStore->fetchItems( mStore->topLevelCollection() );
  QVERIFY( itemFetch != 0 );
  itemFetch->fetchScope().fetchFullPayload();

  result = itemFetch->exec();
  QVERIFY( result );

  Akonadi::Item::List items = itemFetch->items();
  QCOMPARE( items.count(), 3 );

  Akonadi::Item item = items[0];
  QVERIFY( item.hasPayload<KMime::Message::Ptr>() );

  ItemDeleteJob *itemDelete = mStore->deleteItem( item );
  QVERIFY( itemDelete != 0 );

  result = itemDelete->exec();
  QVERIFY( result );

  compactJob = mStore->compactStore();

  result = compactJob->exec();
  QVERIFY( result );

  ItemCreateJob *createJob = mStore->createItem( item, mStore->topLevelCollection() );

  result = createJob->exec();
  QVERIFY( result );
}

#include "augmentedmaildirtest.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
