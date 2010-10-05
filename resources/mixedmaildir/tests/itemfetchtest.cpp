/*  This file is part of the KDE project
    Copyright (C) 2010 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.net
    Author: Kevin Krammer, krake@kdab.com

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

#include "mixedmaildirstore.h"

#include "testdatautil.h"

#include "filestore/itemcreatejob.h"
#include "filestore/itemfetchjob.h"

#include "libmaildir/maildir.h"

#include <kmbox/mbox.h>
#include <kmime/kmime_message.h>

#include <akonadi/kmime/messageparts.h>
#include <akonadi/itemfetchscope.h>

#include <KRandom>
#include <KRandomSequence>
#include <KTempDir>

#include <QSignalSpy>

#include <qtest_kde.h>

using namespace Akonadi;
using namespace KMBox;

static Item::List itemsFromSpy( QSignalSpy *spy ) {
  Item::List items;

  QListIterator<QList<QVariant> > it( *spy );
  while( it.hasNext() ) {
    const QList<QVariant> invocation = it.next();
    Q_ASSERT( invocation.count() == 1 );

    items << invocation.first().value<Item::List>();
  }

  return items;
}

// copied from mail serializer plugin, Copyright (c) 2007 Till Adam <adam@kde.org>
static QSet<QByteArray> messageParts( const KMime::Message::Ptr &msgPtr )
{
  QSet<QByteArray> set;
  // FIXME: we actually want "has any header" here, but the kmime api doesn't offer that yet
  if ( msgPtr->hasContent() || msgPtr->hasHeader( "Message-ID" ) ) {
    set << MessagePart::Envelope << MessagePart::Header;
    if ( !msgPtr->body().isEmpty() || !msgPtr->contents().isEmpty() ) {
      set << MessagePart::Body;
    }
  }
  return set;
}

class ItemFetchTest : public QObject
{
  Q_OBJECT

  public:
    ItemFetchTest()
      : QObject(), mStore( 0 ), mDir( 0 ), mIndexFilePattern( QLatin1String( ".%1.index" ) ) {
      // for monitoring signals
      qRegisterMetaType<Akonadi::Collection::List>();
      qRegisterMetaType<Akonadi::Item::List>();
    }

    ~ItemFetchTest() {
      delete mStore;
      delete mDir;
    }

    QString indexFile( const QString &folder ) const {
      return mIndexFilePattern.arg( folder );
    }

    QString indexFile( const QFileInfo &folderFileInfo ) const {
      return QFileInfo( folderFileInfo.absolutePath(),
                        mIndexFilePattern.arg( folderFileInfo.fileName() ) ).absoluteFilePath();
    }

  private:
    MixedMaildirStore *mStore;
    KTempDir *mDir;

    const QString mIndexFilePattern;

  private Q_SLOTS:
    void init();
    void cleanup();
    void testListingMaildir();
    void testListingMBox();
    void testSingleItemFetchMaildir();
    void testSingleItemFetchMBox();
};

void ItemFetchTest::init()
{
  mStore = new MixedMaildirStore;

  mDir = new KTempDir;
  QVERIFY( mDir->exists() );
}

void ItemFetchTest::cleanup()
{
  delete mStore;
  mStore = 0;
  delete mDir;
  mDir = 0;
}

void ItemFetchTest::testListingMaildir()
{
  QDir topDir( mDir->name() );

  QVERIFY( TestDataUtil::installFolder( QLatin1String( "maildir" ), topDir.path(), QLatin1String( "collection1" ) ) );
  QVERIFY( TestDataUtil::installFolder( QLatin1String( "maildir" ), topDir.path(), QLatin1String( "collection2" ) ) );
  QVERIFY( TestDataUtil::installFolder( QLatin1String( "maildir-tagged" ), topDir.path(), QLatin1String( "collection3" ) ) );
  QVERIFY( TestDataUtil::installFolder( QLatin1String( "dimap" ), topDir.path(), QLatin1String( "collection4" ) ) );
  QVERIFY( TestDataUtil::installFolder( QLatin1String( "maildir-tagged" ), topDir.path(), QLatin1String( "collection5" ) ) );

  KPIM::Maildir topLevelMd( topDir.path(), true );

  KPIM::Maildir md1 = topLevelMd.subFolder( QLatin1String( "collection1" ) );
  QSet<QString> entrySet1 = QSet<QString>::fromList( md1.entryList() );
  QCOMPARE( (int)entrySet1.count(), 4 );

  QFileInfo indexFileInfo1( indexFile( QFileInfo( md1.path() ) ) );
  QVERIFY( QFile::remove( indexFileInfo1.absoluteFilePath() ) );

  KPIM::Maildir md2 = topLevelMd.subFolder( QLatin1String( "collection2" ) );
  QSet<QString> entrySet2 = QSet<QString>::fromList( md2.entryList() );
  QCOMPARE( (int)entrySet2.count(), 4 );

  KPIM::Maildir md3 = topLevelMd.subFolder( QLatin1String( "collection3" ) );
  QSet<QString> entrySet3 = QSet<QString>::fromList( md3.entryList() );
  QCOMPARE( (int)entrySet3.count(), 4 );

  KPIM::Maildir md4 = topLevelMd.subFolder( QLatin1String( "collection4" ) );
  QSet<QString> entrySet4 = QSet<QString>::fromList( md4.entryList() );
  QCOMPARE( (int)entrySet4.count(), 4 );

  KPIM::Maildir md5 = topLevelMd.subFolder( QLatin1String( "collection5" ) );
  QSet<QString> entrySet5 = QSet<QString>::fromList( md5.entryList() );
  QCOMPARE( (int)entrySet5.count(), 4 );

  mStore->setPath( topDir.path() );

  // common variables
  FileStore::ItemFetchJob *job = 0;

  QSignalSpy *spy = 0;
  Item::List items;

  QHash<QString, QVariant> uidHash;
  const QVariant varUidHash = QVariant::fromValue< QHash<QString, QVariant> >( uidHash );
  QHash<QString, QVariant> tagListHash;
  const QVariant varTagListHash = QVariant::fromValue< QHash<QString, QVariant> >( tagListHash );
  QVariant var;

  QSet<QString> entrySet;
  QMap<QByteArray, int> flagCounts;

  // test listing maildir without index
  Collection collection1;
  collection1.setName( QLatin1String( "collection1" ) );
  collection1.setRemoteId( QLatin1String( "collection1" ) );
  collection1.setParentCollection( mStore->topLevelCollection() );

  job = mStore->fetchItems( collection1 );

  spy = new QSignalSpy( job, SIGNAL( itemsReceived( Akonadi::Item::List ) ) );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  items = job->items();
  QCOMPARE( (int)items.count(), 4 );
  QCOMPARE( itemsFromSpy( spy ), items );

  entrySet = entrySet1;
  QVERIFY( entrySet.remove( items[ 0 ].remoteId() ) );
  QVERIFY( entrySet.remove( items[ 1 ].remoteId() ) );
  QVERIFY( entrySet.remove( items[ 2 ].remoteId() ) );
  QVERIFY( entrySet.remove( items[ 3 ].remoteId() ) );

  QCOMPARE( items[ 0 ].parentCollection(), collection1 );
  QCOMPARE( items[ 1 ].parentCollection(), collection1 );
  QCOMPARE( items[ 2 ].parentCollection(), collection1 );
  QCOMPARE( items[ 3 ].parentCollection(), collection1 );

  QCOMPARE( items[ 0 ].flags(), QSet<QByteArray>() );
  QCOMPARE( items[ 1 ].flags(), QSet<QByteArray>() );
  QCOMPARE( items[ 2 ].flags(), QSet<QByteArray>() );
  QCOMPARE( items[ 3 ].flags(), QSet<QByteArray>() );

  var = job->property( "remoteIdToTagList" );
  QVERIFY( !var.isValid() );

  // test listing empty mbox without index
  Q_FOREACH( const QString &entry, entrySet1 ) {
    QVERIFY( md1.removeEntry( entry ) );
  }
  QCOMPARE( md1.entryList().count(), 0 );

  job = mStore->fetchItems( collection1 );

  spy = new QSignalSpy( job, SIGNAL( itemsReceived( Akonadi::Item::List ) ) );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  items = job->items();
  QCOMPARE( (int)items.count(), 0 );
  QCOMPARE( spy->count(), 0 );

  var = job->property( "remoteIdToTagList" );
  QVERIFY( !var.isValid() );

  // test listing maildir with index
  Collection collection2;
  collection2.setName( QLatin1String( "collection2" ) );
  collection2.setRemoteId( QLatin1String( "collection2" ) );
  collection2.setParentCollection( mStore->topLevelCollection() );

  job = mStore->fetchItems( collection2 );

  spy = new QSignalSpy( job, SIGNAL( itemsReceived( Akonadi::Item::List ) ) );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  items = job->items();
  QCOMPARE( (int)items.count(), 4 );
  QCOMPARE( itemsFromSpy( spy ), items );

  entrySet = entrySet2;
  QVERIFY( entrySet.remove( items[ 0 ].remoteId() ) );
  QVERIFY( entrySet.remove( items[ 1 ].remoteId() ) );
  QVERIFY( entrySet.remove( items[ 2 ].remoteId() ) );
  QVERIFY( entrySet.remove( items[ 3 ].remoteId() ) );

  QCOMPARE( items[ 0 ].parentCollection(), collection2 );
  QCOMPARE( items[ 1 ].parentCollection(), collection2 );
  QCOMPARE( items[ 2 ].parentCollection(), collection2 );
  QCOMPARE( items[ 3 ].parentCollection(), collection2 );

  // see data/README
  Q_FOREACH( const Item &item, items ) {
    Q_FOREACH( const QByteArray &flag, item.flags() ) {
      ++flagCounts[ flag ];
    }
  }
  QCOMPARE( flagCounts[ "\\SEEN" ], 2 );
  QCOMPARE( flagCounts[ "\\FLAGGED" ], 1 );
  QCOMPARE( flagCounts[ "$TODO" ], 1 );
  flagCounts.clear();

  var = job->property( "remoteIdToTagList" );
  QVERIFY( !var.isValid() );

  // test listing empty maildir with index
  Q_FOREACH( const QString &entry, entrySet2 ) {
    QVERIFY( md2.removeEntry( entry ) );
  }
  QCOMPARE( md2.entryList().count(), 0 );

  job = mStore->fetchItems( collection2 );

  spy = new QSignalSpy( job, SIGNAL( itemsReceived( Akonadi::Item::List ) ) );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  items = job->items();
  QCOMPARE( (int)items.count(), 0 );
  QCOMPARE( spy->count(), 0 );

  var = job->property( "remoteIdToTagList" );
  QVERIFY( !var.isValid() );

  // test listing maildir with index which has tags
  Collection collection3;
  collection3.setName( QLatin1String( "collection3" ) );
  collection3.setRemoteId( QLatin1String( "collection3" ) );
  collection3.setParentCollection( mStore->topLevelCollection() );

  job = mStore->fetchItems( collection3 );

  spy = new QSignalSpy( job, SIGNAL( itemsReceived( Akonadi::Item::List ) ) );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  items = job->items();
  QCOMPARE( (int)items.count(), 4 );
  QCOMPARE( itemsFromSpy( spy ), items );

  entrySet = entrySet3;
  QVERIFY( entrySet.remove( items[ 0 ].remoteId() ) );
  QVERIFY( entrySet.remove( items[ 1 ].remoteId() ) );
  QVERIFY( entrySet.remove( items[ 2 ].remoteId() ) );
  QVERIFY( entrySet.remove( items[ 3 ].remoteId() ) );

  QCOMPARE( items[ 0 ].parentCollection(), collection3 );
  QCOMPARE( items[ 1 ].parentCollection(), collection3 );
  QCOMPARE( items[ 2 ].parentCollection(), collection3 );
  QCOMPARE( items[ 3 ].parentCollection(), collection3 );

  // see data/README
  Q_FOREACH( const Item &item, items ) {
    Q_FOREACH( const QByteArray &flag, item.flags() ) {
      ++flagCounts[ flag ];
    }
  }
  QCOMPARE( flagCounts[ "\\SEEN" ], 2 );
  QCOMPARE( flagCounts[ "\\FLAGGED" ], 1 );
  QCOMPARE( flagCounts[ "$TODO" ], 1 );
  flagCounts.clear();

  var = job->property( "remoteIdToTagList" );
  QVERIFY( var.isValid() );

  tagListHash = var.value< QHash<QString, QVariant> >();
  QCOMPARE( (int)tagListHash.count(), 3 );
  QVERIFY( !tagListHash.contains( items[ 0 ].remoteId() ) );
  QVERIFY( !tagListHash.value( items[ 1 ].remoteId() ).toString().isEmpty() );
  QVERIFY( !tagListHash.value( items[ 2 ].remoteId() ).toString().isEmpty() );
  QVERIFY( !tagListHash.value( items[ 3 ].remoteId() ).toString().isEmpty() );

  // test listing maildir with index which contains IMAP UIDs (dimap cache directory)
  Collection collection4;
  collection4.setName( QLatin1String( "collection4" ) );
  collection4.setRemoteId( QLatin1String( "collection4" ) );
  collection4.setParentCollection( mStore->topLevelCollection() );

  job = mStore->fetchItems( collection4 );

  spy = new QSignalSpy( job, SIGNAL( itemsReceived( Akonadi::Item::List ) ) );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  items = job->items();
  QCOMPARE( (int)items.count(), 4 );
  QCOMPARE( itemsFromSpy( spy ), items );

  entrySet = entrySet4;
  QVERIFY( entrySet.remove( items[ 0 ].remoteId() ) );
  QVERIFY( entrySet.remove( items[ 1 ].remoteId() ) );
  QVERIFY( entrySet.remove( items[ 2 ].remoteId() ) );
  QVERIFY( entrySet.remove( items[ 3 ].remoteId() ) );

  QCOMPARE( items[ 0 ].parentCollection(), collection4 );
  QCOMPARE( items[ 1 ].parentCollection(), collection4 );
  QCOMPARE( items[ 2 ].parentCollection(), collection4 );
  QCOMPARE( items[ 3 ].parentCollection(), collection4 );

  // see data/README
  Q_FOREACH( const Item &item, items ) {
    Q_FOREACH( const QByteArray &flag, item.flags() ) {
      ++flagCounts[ flag ];
    }
  }
  QCOMPARE( flagCounts[ "\\SEEN" ], 2 );
  QCOMPARE( flagCounts[ "\\FLAGGED" ], 1 );
  QCOMPARE( flagCounts[ "$TODO" ], 1 );
  flagCounts.clear();

  var = job->property( "remoteIdToTagList" );
  QVERIFY( !var.isValid() );

  var = job->property( "remoteIdToIndexUid" );
  QVERIFY( var.isValid() );

  uidHash = var.value< QHash<QString, QVariant> >();
  QCOMPARE( (int)uidHash.count(), 4 );
  bool ok = false;
  QVERIFY( !uidHash.value( items[ 0 ].remoteId() ).toString().toInt( &ok) >= 0 && ok  );
  QVERIFY( !uidHash.value( items[ 1 ].remoteId() ).toString().toInt( &ok) >= 0 && ok  );
  QVERIFY( !uidHash.value( items[ 2 ].remoteId() ).toString().toInt( &ok) >= 0 && ok  );
  QVERIFY( !uidHash.value( items[ 3 ].remoteId() ).toString().toInt( &ok) >= 0 && ok  );

  // test listing mbox with index but newer modification date than index's one
  const QByteArray data5 = md5.readEntry( entrySet5.values().first() );
  QVERIFY( !data5.isEmpty() );

  QTest::qSleep( 1000 );

  QString newRemoteId = md5.addEntry( data5 );
  QVERIFY( !newRemoteId.isEmpty() );

  entrySet = QSet<QString>::fromList( md5.entryList() );
  entrySet.remove( newRemoteId );
  QCOMPARE( entrySet, entrySet5 );
  QFileInfo fileInfo5( md5.path(), QLatin1String( "new" ) );
  QFileInfo indexFileInfo5 = indexFile( QFileInfo( md5.path() ) );
  QVERIFY( fileInfo5.lastModified() > indexFileInfo5.lastModified() );

  Collection collection5;
  collection5.setName( QLatin1String( "collection5" ) );
  collection5.setRemoteId( QLatin1String( "collection5" ) );
  collection5.setParentCollection( mStore->topLevelCollection() );

  job = mStore->fetchItems( collection5 );

  spy = new QSignalSpy( job, SIGNAL( itemsReceived( Akonadi::Item::List ) ) );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  items = job->items();
  QCOMPARE( (int)items.count(), 5 );
  QCOMPARE( itemsFromSpy( spy ), items );

  entrySet = entrySet5;
  entrySet << newRemoteId;
  QVERIFY( entrySet.remove( items[ 0 ].remoteId() ) );
  QVERIFY( entrySet.remove( items[ 1 ].remoteId() ) );
  QVERIFY( entrySet.remove( items[ 2 ].remoteId() ) );
  QVERIFY( entrySet.remove( items[ 3 ].remoteId() ) );
  QVERIFY( entrySet.remove( items[ 4 ].remoteId() ) );

  QCOMPARE( items[ 0 ].parentCollection(), collection5 );
  QCOMPARE( items[ 1 ].parentCollection(), collection5 );
  QCOMPARE( items[ 2 ].parentCollection(), collection5 );
  QCOMPARE( items[ 3 ].parentCollection(), collection5 );
  QCOMPARE( items[ 4 ].parentCollection(), collection5 );

  QCOMPARE( items[ 0 ].flags(), QSet<QByteArray>() );
  QCOMPARE( items[ 1 ].flags(), QSet<QByteArray>() );
  QCOMPARE( items[ 2 ].flags(), QSet<QByteArray>() );
  QCOMPARE( items[ 3 ].flags(), QSet<QByteArray>() );
  QCOMPARE( items[ 4 ].flags(), QSet<QByteArray>() );

  var = job->property( "remoteIdToTagList" );
  QVERIFY( !var.isValid() );
}

void ItemFetchTest::testListingMBox()
{
  QDir topDir( mDir->name() );

  QVERIFY( TestDataUtil::installFolder( QLatin1String( "mbox" ), topDir.path(), QLatin1String( "collection1" ) ) );
  QVERIFY( TestDataUtil::installFolder( QLatin1String( "mbox" ), topDir.path(), QLatin1String( "collection2" ) ) );
  QVERIFY( TestDataUtil::installFolder( QLatin1String( "mbox-tagged" ), topDir.path(), QLatin1String( "collection3" ) ) );
  QVERIFY( TestDataUtil::installFolder( QLatin1String( "mbox-unpurged" ), topDir.path(), QLatin1String( "collection4" ) ) );
  QVERIFY( TestDataUtil::installFolder( QLatin1String( "mbox-tagged" ), topDir.path(), QLatin1String( "collection5" ) ) );

  QFileInfo fileInfo1( topDir.path(), QLatin1String( "collection1" ) );
  MBox mbox1;
  QVERIFY( mbox1.load( fileInfo1.absoluteFilePath() ) );
  MBoxEntry::List entryList1 = mbox1.entries();
  QCOMPARE( (int)entryList1.count(), 4 );

  QFileInfo indexFileInfo1 = indexFile( fileInfo1 );
  QVERIFY( QFile::remove( indexFileInfo1.absoluteFilePath() ) );

  QFileInfo fileInfo2( topDir.path(), QLatin1String( "collection2" ) );
  MBox mbox2;
  QVERIFY( mbox2.load( fileInfo2.absoluteFilePath() ) );
  MBoxEntry::List entryList2 = mbox2.entries();
  QCOMPARE( (int)entryList2.count(), 4 );

  QFileInfo fileInfo3( topDir.path(), QLatin1String( "collection3" ) );
  MBox mbox3;
  QVERIFY( mbox3.load( fileInfo3.absoluteFilePath() ) );
  MBoxEntry::List entryList3 = mbox3.entries();
  QCOMPARE( (int)entryList3.count(), 4 );

  QFileInfo fileInfo4( topDir.path(), QLatin1String( "collection4" ) );
  MBox mbox4;
  QVERIFY( mbox4.load( fileInfo4.absoluteFilePath() ) );
  MBoxEntry::List entryList4 = mbox4.entries();
  QCOMPARE( (int)entryList4.count(), 4 );

  QFileInfo fileInfo5( topDir.path(), QLatin1String( "collection5" ) );
  MBox mbox5;
  QVERIFY( mbox5.load( fileInfo5.absoluteFilePath() ) );
  MBoxEntry::List entryList5 = mbox5.entries();
  QCOMPARE( (int)entryList5.count(), 4 );

  mStore->setPath( topDir.path() );

  // common variables
  FileStore::ItemFetchJob *job = 0;

  QSignalSpy *spy = 0;
  Item::List items;

  QHash<QString, QVariant> tagListHash;
  const QVariant varTagListHash = QVariant::fromValue< QHash<QString, QVariant> >( tagListHash );
  QVariant var;

  // test listing mbox without index
  Collection collection1;
  collection1.setName( QLatin1String( "collection1" ) );
  collection1.setRemoteId( QLatin1String( "collection1" ) );
  collection1.setParentCollection( mStore->topLevelCollection() );

  job = mStore->fetchItems( collection1 );

  spy = new QSignalSpy( job, SIGNAL( itemsReceived( Akonadi::Item::List ) ) );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  items = job->items();
  QCOMPARE( (int)items.count(), 4 );
  QCOMPARE( itemsFromSpy( spy ), items );

  QCOMPARE( items[ 0 ].remoteId(), QString::number( entryList1[ 0 ].messageOffset() ) );
  QCOMPARE( items[ 1 ].remoteId(), QString::number( entryList1[ 1 ].messageOffset() ) );
  QCOMPARE( items[ 2 ].remoteId(), QString::number( entryList1[ 2 ].messageOffset() ) );
  QCOMPARE( items[ 3 ].remoteId(), QString::number( entryList1[ 3 ].messageOffset() ) );

  QCOMPARE( items[ 0 ].parentCollection(), collection1 );
  QCOMPARE( items[ 1 ].parentCollection(), collection1 );
  QCOMPARE( items[ 2 ].parentCollection(), collection1 );
  QCOMPARE( items[ 3 ].parentCollection(), collection1 );

  QCOMPARE( items[ 0 ].flags(), QSet<QByteArray>() );
  QCOMPARE( items[ 1 ].flags(), QSet<QByteArray>() );
  QCOMPARE( items[ 2 ].flags(), QSet<QByteArray>() );
  QCOMPARE( items[ 3 ].flags(), QSet<QByteArray>() );

  var = job->property( "remoteIdToTagList" );
  QVERIFY( !var.isValid() );

  // test listing empty mbox without index
  QFile file1( fileInfo1.absoluteFilePath() );
  QVERIFY( file1.open( QIODevice::WriteOnly | QIODevice::Truncate ) );
  file1.close();
  QCOMPARE( (int)file1.size(), 0 );

  job = mStore->fetchItems( collection1 );

  spy = new QSignalSpy( job, SIGNAL( itemsReceived( Akonadi::Item::List ) ) );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  items = job->items();
  QCOMPARE( (int)items.count(), 0 );
  QCOMPARE( spy->count(), 0 );

  var = job->property( "remoteIdToTagList" );
  QVERIFY( !var.isValid() );

  // test listing mbox with index
  Collection collection2;
  collection2.setName( QLatin1String( "collection2" ) );
  collection2.setRemoteId( QLatin1String( "collection2" ) );
  collection2.setParentCollection( mStore->topLevelCollection() );

  job = mStore->fetchItems( collection2 );

  spy = new QSignalSpy( job, SIGNAL( itemsReceived( Akonadi::Item::List ) ) );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  items = job->items();
  QCOMPARE( (int)items.count(), 4 );
  QCOMPARE( itemsFromSpy( spy ), items );

  QCOMPARE( items[ 0 ].remoteId(), QString::number( entryList2[ 0 ].messageOffset() ) );
  QCOMPARE( items[ 1 ].remoteId(), QString::number( entryList2[ 1 ].messageOffset() ) );
  QCOMPARE( items[ 2 ].remoteId(), QString::number( entryList2[ 2 ].messageOffset() ) );
  QCOMPARE( items[ 3 ].remoteId(), QString::number( entryList2[ 3 ].messageOffset() ) );

  QCOMPARE( items[ 0 ].parentCollection(), collection2 );
  QCOMPARE( items[ 1 ].parentCollection(), collection2 );
  QCOMPARE( items[ 2 ].parentCollection(), collection2 );
  QCOMPARE( items[ 3 ].parentCollection(), collection2 );

  // see data/README
  QCOMPARE( items[ 0 ].flags(), QSet<QByteArray>()  );
  QCOMPARE( items[ 1 ].flags(), QSet<QByteArray>() << "\\SEEN" << "$TODO" );
  QCOMPARE( items[ 2 ].flags(), QSet<QByteArray>() );
  QCOMPARE( items[ 3 ].flags(), QSet<QByteArray>() << "\\SEEN" << "\\FLAGGED" );

  var = job->property( "remoteIdToTagList" );
  QVERIFY( !var.isValid() );

  // test listing empty mbox with index
  QFile file2( fileInfo2.absoluteFilePath() );
  QVERIFY( file2.open( QIODevice::WriteOnly | QIODevice::Truncate ) );
  file2.close();
  QCOMPARE( (int)file2.size(), 0 );

  job = mStore->fetchItems( collection2 );

  spy = new QSignalSpy( job, SIGNAL( itemsReceived( Akonadi::Item::List ) ) );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  items = job->items();
  QCOMPARE( (int)items.count(), 0 );
  QCOMPARE( spy->count(), 0 );

  var = job->property( "remoteIdToTagList" );
  QVERIFY( !var.isValid() );

  // test listing mbox with index which has tags
  Collection collection3;
  collection3.setName( QLatin1String( "collection3" ) );
  collection3.setRemoteId( QLatin1String( "collection3" ) );
  collection3.setParentCollection( mStore->topLevelCollection() );

  job = mStore->fetchItems( collection3 );

  spy = new QSignalSpy( job, SIGNAL( itemsReceived( Akonadi::Item::List ) ) );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  items = job->items();
  QCOMPARE( (int)items.count(), 4 );
  QCOMPARE( itemsFromSpy( spy ), items );

  QCOMPARE( items[ 0 ].remoteId(), QString::number( entryList3[ 0 ].messageOffset() ) );
  QCOMPARE( items[ 1 ].remoteId(), QString::number( entryList3[ 1 ].messageOffset() ) );
  QCOMPARE( items[ 2 ].remoteId(), QString::number( entryList3[ 2 ].messageOffset() ) );
  QCOMPARE( items[ 3 ].remoteId(), QString::number( entryList3[ 3 ].messageOffset() ) );

  QCOMPARE( items[ 0 ].parentCollection(), collection3 );
  QCOMPARE( items[ 1 ].parentCollection(), collection3 );
  QCOMPARE( items[ 2 ].parentCollection(), collection3 );
  QCOMPARE( items[ 3 ].parentCollection(), collection3 );

  // see data/README
  QCOMPARE( items[ 0 ].flags(), QSet<QByteArray>()  );
  QCOMPARE( items[ 1 ].flags(), QSet<QByteArray>() << "\\SEEN" << "$TODO" );
  QCOMPARE( items[ 2 ].flags(), QSet<QByteArray>() );
  QCOMPARE( items[ 3 ].flags(), QSet<QByteArray>() << "\\SEEN" << "\\FLAGGED" );

  var = job->property( "remoteIdToTagList" );
  QVERIFY( var.isValid() );

  tagListHash = var.value< QHash<QString, QVariant> >();
  QCOMPARE( (int)tagListHash.count(), 3 );
  QVERIFY( !tagListHash.value( items[ 0 ].remoteId() ).toString().isEmpty() );
  QVERIFY( !tagListHash.contains( items[ 1 ].remoteId() ) );
  QVERIFY( !tagListHash.value( items[ 2 ].remoteId() ).toString().isEmpty() );
  QVERIFY( !tagListHash.value( items[ 3 ].remoteId() ).toString().isEmpty() );

  // test listing mbox with index and unpurged messages (in mbox but not in index)
  Collection collection4;
  collection4.setName( QLatin1String( "collection4" ) );
  collection4.setRemoteId( QLatin1String( "collection4" ) );
  collection4.setParentCollection( mStore->topLevelCollection() );

  job = mStore->fetchItems( collection4 );

  spy = new QSignalSpy( job, SIGNAL( itemsReceived( Akonadi::Item::List ) ) );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  items = job->items();
  QCOMPARE( (int)items.count(), 4 );
  QCOMPARE( itemsFromSpy( spy ), items );

  QCOMPARE( items[ 0 ].remoteId(), QString::number( entryList4[ 0 ].messageOffset() ) );
  QCOMPARE( items[ 1 ].remoteId(), QString::number( entryList4[ 1 ].messageOffset() ) );
  QCOMPARE( items[ 2 ].remoteId(), QString::number( entryList4[ 2 ].messageOffset() ) );
  QCOMPARE( items[ 3 ].remoteId(), QString::number( entryList4[ 3 ].messageOffset() ) );

  QCOMPARE( items[ 0 ].parentCollection(), collection4 );
  QCOMPARE( items[ 1 ].parentCollection(), collection4 );
  QCOMPARE( items[ 2 ].parentCollection(), collection4 );
  QCOMPARE( items[ 3 ].parentCollection(), collection4 );

  // see data/README
  QCOMPARE( items[ 0 ].flags(), QSet<QByteArray>() << "\\SEEN" );
  QCOMPARE( items[ 1 ].flags(), QSet<QByteArray>() << "\\DELETED" );
  QCOMPARE( items[ 2 ].flags(), QSet<QByteArray>() << "\\DELETED" );
  QCOMPARE( items[ 3 ].flags(), QSet<QByteArray>() << "\\SEEN" );

  var = job->property( "remoteIdToTagList" );
  QVERIFY( !var.isValid() );

  // test listing mbox with index but newer modification date than index's one
  QFile file5( fileInfo5.absoluteFilePath() );
  QVERIFY( file5.open( QIODevice::ReadOnly ) );
  const QByteArray data5 = file5.readAll();
  file5.close();

  QTest::qSleep( 1000 );

  QVERIFY( file5.open( QIODevice::WriteOnly ) );
  file5.write( data5 );
  file5.close();

  QCOMPARE( file5.size(), fileInfo5.size() );
  fileInfo5.refresh();
  QFileInfo indexFileInfo5 = indexFile( fileInfo5 );
  QVERIFY( fileInfo5.lastModified() > indexFileInfo5.lastModified() );

  Collection collection5;
  collection5.setName( QLatin1String( "collection5" ) );
  collection5.setRemoteId( QLatin1String( "collection5" ) );
  collection5.setParentCollection( mStore->topLevelCollection() );

  job = mStore->fetchItems( collection5 );

  spy = new QSignalSpy( job, SIGNAL( itemsReceived( Akonadi::Item::List ) ) );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  items = job->items();
  QCOMPARE( (int)items.count(), 4 );
  QCOMPARE( itemsFromSpy( spy ), items );

  QCOMPARE( items[ 0 ].remoteId(), QString::number( entryList5[ 0 ].messageOffset() ) );
  QCOMPARE( items[ 1 ].remoteId(), QString::number( entryList5[ 1 ].messageOffset() ) );
  QCOMPARE( items[ 2 ].remoteId(), QString::number( entryList5[ 2 ].messageOffset() ) );
  QCOMPARE( items[ 3 ].remoteId(), QString::number( entryList5[ 3 ].messageOffset() ) );

  QCOMPARE( items[ 0 ].parentCollection(), collection5 );
  QCOMPARE( items[ 1 ].parentCollection(), collection5 );
  QCOMPARE( items[ 2 ].parentCollection(), collection5 );
  QCOMPARE( items[ 3 ].parentCollection(), collection5 );

  QCOMPARE( items[ 0 ].flags(), QSet<QByteArray>() );
  QCOMPARE( items[ 1 ].flags(), QSet<QByteArray>() );
  QCOMPARE( items[ 2 ].flags(), QSet<QByteArray>() );
  QCOMPARE( items[ 3 ].flags(), QSet<QByteArray>() );

  var = job->property( "remoteIdToTagList" );
  QVERIFY( !var.isValid() );

  // test that a new message in an mbox with index it not marked as deleted
  KMime::Message::Ptr msgPtr( new KMime::Message );
  msgPtr->subject()->from7BitString( "Test 5" );
  msgPtr->to()->from7BitString( "kevin.krammer@gmx.at" );
  msgPtr->assemble();

  Item item3_5;
  item3_5.setMimeType( KMime::Message::mimeType() );
  item3_5.setPayload<KMime::Message::Ptr>( msgPtr );

  FileStore::ItemCreateJob *itemCreate = mStore->createItem( item3_5, collection3 );
  QVERIFY( itemCreate->exec() );

  item3_5 = itemCreate->item();
  QVERIFY( !item3_5.remoteId().isEmpty() );

  job = mStore->fetchItems( collection3 );

  spy = new QSignalSpy( job, SIGNAL( itemsReceived( Akonadi::Item::List ) ) );

  QVERIFY( job->exec() );
  QCOMPARE( job->error(), 0 );

  items = job->items();
  QCOMPARE( (int)items.count(), 5 );
  QCOMPARE( itemsFromSpy( spy ), items );

  QCOMPARE( items[ 0 ].remoteId(), QString::number( entryList3[ 0 ].messageOffset() ) );
  QCOMPARE( items[ 1 ].remoteId(), QString::number( entryList3[ 1 ].messageOffset() ) );
  QCOMPARE( items[ 2 ].remoteId(), QString::number( entryList3[ 2 ].messageOffset() ) );
  QCOMPARE( items[ 3 ].remoteId(), QString::number( entryList3[ 3 ].messageOffset() ) );
  QCOMPARE( items[ 4 ].remoteId(), item3_5.remoteId() );

  QCOMPARE( items[ 0 ].parentCollection(), collection3 );
  QCOMPARE( items[ 1 ].parentCollection(), collection3 );
  QCOMPARE( items[ 2 ].parentCollection(), collection3 );
  QCOMPARE( items[ 3 ].parentCollection(), collection3 );
  QCOMPARE( items[ 4 ].parentCollection(), collection3 );

  // see data/README
  QCOMPARE( items[ 0 ].flags(), QSet<QByteArray>()  );
  QCOMPARE( items[ 1 ].flags(), QSet<QByteArray>() << "\\SEEN" << "$TODO" );
  QCOMPARE( items[ 2 ].flags(), QSet<QByteArray>() );
  QCOMPARE( items[ 3 ].flags(), QSet<QByteArray>() << "\\SEEN" << "\\FLAGGED" );
  QCOMPARE( items[ 4 ].flags(), QSet<QByteArray>() );

  var = job->property( "remoteIdToTagList" );
  QVERIFY( var.isValid() );
}

void ItemFetchTest::testSingleItemFetchMaildir()
{
  QDir topDir( mDir->name() );

  QVERIFY( TestDataUtil::installFolder( QLatin1String( "maildir" ), topDir.path(), QLatin1String( "collection1" ) ) );

  KPIM::Maildir topLevelMd( topDir.path(), true );

  KPIM::Maildir md1 = topLevelMd.subFolder( QLatin1String( "collection1" ) );
  QStringList entryList1 = md1.entryList();
  QCOMPARE( (int)entryList1.count(), 4 );

  KRandomSequence randomSequence;
  QStringList randomList1 = entryList1;
  randomSequence.randomize( randomList1 );

  mStore->setPath( topDir.path() );

  // common variables
  FileStore::ItemFetchJob *job = 0;

  QSignalSpy *spy = 0;
  Item::List items;

  // test fetching from maildir, headers only
  Collection collection1;
  collection1.setName( QLatin1String( "collection1" ) );
  collection1.setRemoteId( QLatin1String( "collection1" ) );
  collection1.setParentCollection( mStore->topLevelCollection() );

  Q_FOREACH( const QString &entry, randomList1 ) {
    Item item1;
    item1.setId( KRandom::random() );
    item1.setRemoteId( entry );
    item1.setParentCollection( collection1 );

    job = mStore->fetchItem( item1 );
    job->fetchScope().fetchPayloadPart( MessagePart::Header );

    spy = new QSignalSpy( job, SIGNAL( itemsReceived( Akonadi::Item::List ) ) );

    QVERIFY( job->exec() );
    QCOMPARE( job->error(), 0 );

    items = job->items();
    QCOMPARE( (int)items.count(), 1 );
    QCOMPARE( itemsFromSpy( spy ), items );

    Item item = items.first();
    QCOMPARE( item, item1 );
    QVERIFY( item.hasPayload<KMime::Message::Ptr>() );

    KMime::Message::Ptr msgPtr = item.payload<KMime::Message::Ptr>();
    QVERIFY( msgPtr != 0 );

    const QSet<QByteArray> parts = messageParts( msgPtr );
    QVERIFY( !parts.isEmpty() );
    QVERIFY( parts.contains( MessagePart::Header ) );
    QVERIFY( !parts.contains( MessagePart::Body ) );
  }

  // test fetching from maildir, including body
  randomSequence.randomize( randomList1 );
  Q_FOREACH( const QString &entry, randomList1 ) {
    Item item1;
    item1.setId( KRandom::random() );
    item1.setRemoteId( entry );
    item1.setParentCollection( collection1 );

    job = mStore->fetchItem( item1 );
    job->fetchScope().fetchPayloadPart( MessagePart::Header );
    job->fetchScope().fetchPayloadPart( MessagePart::Body );

    spy = new QSignalSpy( job, SIGNAL( itemsReceived( Akonadi::Item::List ) ) );

    QVERIFY( job->exec() );
    QCOMPARE( job->error(), 0 );

    items = job->items();
    QCOMPARE( (int)items.count(), 1 );
    QCOMPARE( itemsFromSpy( spy ), items );

    Item item = items.first();
    QCOMPARE( item, item1 );
    QVERIFY( item.hasPayload<KMime::Message::Ptr>() );

    KMime::Message::Ptr msgPtr = item.payload<KMime::Message::Ptr>();
    QVERIFY( msgPtr != 0 );

    const QSet<QByteArray> parts = messageParts( msgPtr );
    QVERIFY( !parts.isEmpty() );
    QVERIFY( parts.contains( MessagePart::Header ) );
    QVERIFY( parts.contains( MessagePart::Body ) );
  }

  // test fetching from maildir, just specifying full payload
  randomSequence.randomize( randomList1 );
  Q_FOREACH( const QString &entry, randomList1 ) {
    Item item1;
    item1.setId( KRandom::random() );
    item1.setRemoteId( entry );
    item1.setParentCollection( collection1 );

    job = mStore->fetchItem( item1 );
    job->fetchScope().fetchFullPayload( true );

    spy = new QSignalSpy( job, SIGNAL( itemsReceived( Akonadi::Item::List ) ) );

    QVERIFY( job->exec() );
    QCOMPARE( job->error(), 0 );

    items = job->items();
    QCOMPARE( (int)items.count(), 1 );
    QCOMPARE( itemsFromSpy( spy ), items );

    Item item = items.first();
    QCOMPARE( item, item1 );
    QVERIFY( item.hasPayload<KMime::Message::Ptr>() );

    KMime::Message::Ptr msgPtr = item.payload<KMime::Message::Ptr>();
    QVERIFY( msgPtr != 0 );

    const QSet<QByteArray> parts = messageParts( msgPtr );
    QVERIFY( !parts.isEmpty() );
    QVERIFY( parts.contains( MessagePart::Header ) );
    QVERIFY( parts.contains( MessagePart::Body ) );
  }
}

void ItemFetchTest::testSingleItemFetchMBox()
{
  QDir topDir( mDir->name() );

  QVERIFY( TestDataUtil::installFolder( QLatin1String( "mbox" ), topDir.path(), QLatin1String( "collection1" ) ) );

  QFileInfo fileInfo1( topDir.path(), QLatin1String( "collection1" ) );
  MBox mbox1;
  QVERIFY( mbox1.load( fileInfo1.absoluteFilePath() ) );
  MBoxEntry::List entryList1 = mbox1.entries();
  QCOMPARE( (int)entryList1.count(), 4 );

  KRandomSequence randomSequence;
  MBoxEntry::List randomList1 = entryList1;
  randomSequence.randomize( randomList1 );

  mStore->setPath( topDir.path() );

  // common variables
  FileStore::ItemFetchJob *job = 0;

  QSignalSpy *spy = 0;
  Item::List items;

  // test fetching from maildir, headers only
  Collection collection1;
  collection1.setName( QLatin1String( "collection1" ) );
  collection1.setRemoteId( QLatin1String( "collection1" ) );
  collection1.setParentCollection( mStore->topLevelCollection() );

  Q_FOREACH( const MBoxEntry &entry, randomList1 ) {
    Item item1;
    item1.setId( KRandom::random() );
    item1.setRemoteId( QString::number( entry.messageOffset() ) );
    item1.setParentCollection( collection1 );

    job = mStore->fetchItem( item1 );
    job->fetchScope().fetchPayloadPart( MessagePart::Header );

    spy = new QSignalSpy( job, SIGNAL( itemsReceived( Akonadi::Item::List ) ) );

    QVERIFY( job->exec() );
    QCOMPARE( job->error(), 0 );

    items = job->items();
    QCOMPARE( (int)items.count(), 1 );
    QCOMPARE( itemsFromSpy( spy ), items );

    Item item = items.first();
    QCOMPARE( item, item1 );
    QVERIFY( item.hasPayload<KMime::Message::Ptr>() );

    KMime::Message::Ptr msgPtr = item.payload<KMime::Message::Ptr>();
    QVERIFY( msgPtr != 0 );

    const QSet<QByteArray> parts = messageParts( msgPtr );
    QVERIFY( !parts.isEmpty() );
    QVERIFY( parts.contains( MessagePart::Header ) );
    QVERIFY( !parts.contains( MessagePart::Body ) );
  }

  // test fetching from maildir, including body
  randomSequence.randomize( randomList1 );
  Q_FOREACH( const MBoxEntry &entry, randomList1 ) {
    Item item1;
    item1.setId( KRandom::random() );
    item1.setRemoteId( QString::number( entry.messageOffset() ) );
    item1.setParentCollection( collection1 );

    job = mStore->fetchItem( item1 );
    job->fetchScope().fetchPayloadPart( MessagePart::Header );
    job->fetchScope().fetchPayloadPart( MessagePart::Body );

    spy = new QSignalSpy( job, SIGNAL( itemsReceived( Akonadi::Item::List ) ) );

    QVERIFY( job->exec() );
    QCOMPARE( job->error(), 0 );

    items = job->items();
    QCOMPARE( (int)items.count(), 1 );
    QCOMPARE( itemsFromSpy( spy ), items );

    Item item = items.first();
    QCOMPARE( item, item1 );
    QVERIFY( item.hasPayload<KMime::Message::Ptr>() );

    KMime::Message::Ptr msgPtr = item.payload<KMime::Message::Ptr>();
    QVERIFY( msgPtr != 0 );

    const QSet<QByteArray> parts = messageParts( msgPtr );
    QVERIFY( !parts.isEmpty() );
    QVERIFY( parts.contains( MessagePart::Header ) );
    QVERIFY( parts.contains( MessagePart::Body ) );
  }

  // test fetching from maildir, just specifying full payload
  randomSequence.randomize( randomList1 );
  Q_FOREACH( const MBoxEntry &entry, randomList1 ) {
    Item item1;
    item1.setId( KRandom::random() );
    item1.setRemoteId( QString::number( entry.messageOffset() ) );
    item1.setParentCollection( collection1 );

    job = mStore->fetchItem( item1 );
    job->fetchScope().fetchFullPayload( true );

    spy = new QSignalSpy( job, SIGNAL( itemsReceived( Akonadi::Item::List ) ) );

    QVERIFY( job->exec() );
    QCOMPARE( job->error(), 0 );

    items = job->items();
    QCOMPARE( (int)items.count(), 1 );
    QCOMPARE( itemsFromSpy( spy ), items );

    Item item = items.first();
    QCOMPARE( item, item1 );
    QVERIFY( item.hasPayload<KMime::Message::Ptr>() );

    KMime::Message::Ptr msgPtr = item.payload<KMime::Message::Ptr>();
    QVERIFY( msgPtr != 0 );

    const QSet<QByteArray> parts = messageParts( msgPtr );
    QVERIFY( !parts.isEmpty() );
    QVERIFY( parts.contains( MessagePart::Header ) );
    QVERIFY( parts.contains( MessagePart::Body ) );
  }
}

QTEST_KDEMAIN( ItemFetchTest, NoGUI )

#include "itemfetchtest.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
