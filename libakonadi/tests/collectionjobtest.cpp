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

#include "collectionjobtest.h"
#include <qtest_kde.h>

#include "collection.h"
#include "collectioncreatejob.h"
#include "collectiondeletejob.h"
#include "collectionlistjob.h"
#include "collectionmodifyjob.h"
#include "collectionrenamejob.h"
#include "collectionstatusjob.h"
#include "control.h"
#include "messagecollectionattribute.h"

#include <QDBusConnection>
#include <QDBusConnectionInterface>

using namespace Akonadi;

QTEST_KDEMAIN( CollectionJobTest, NoGUI )

void CollectionJobTest::initTestCase()
{
  Control::start();
}

static Collection findCol( const Collection::List &list, const QString &name ) {
  foreach ( const Collection col, list )
    if ( col.name() == name )
      return col;
  return Collection();
}

// list compare which ignores the order
template <class T> static void compareLists( const QList<T> &l1, const QList<T> &l2 )
{
  QCOMPARE( l1.count(), l2.count() );
  foreach ( const T entry, l1 ) {
    QVERIFY( l2.contains( entry ) );
  }
}

template <typename T> static T* extractAttribute( QList<CollectionAttribute*> attrs )
{
  T dummy;
  foreach ( CollectionAttribute* attr, attrs ) {
    if ( attr->type() == dummy.type() )
      return dynamic_cast<T*>( attr );
  }
  return 0;
}

static int res1ColId = -1;
static int res2ColId = -1;
static int res3ColId = -1;
static int searchColId = -1;

void CollectionJobTest::testTopLevelList( )
{
  // non-recursive top-level list
  CollectionListJob *job = new CollectionListJob( Collection::root(), CollectionListJob::Flat );
  QVERIFY( job->exec() );
  Collection::List list = job->collections();

  // check if everything is there and has the correct types and attributes
  QEXPECT_FAIL( "", "Search folder not yet ported", Continue );
  QCOMPARE( list.count(), 4 );
  Collection col;

  col = findCol( list, "res1" );
  QVERIFY( col.isValid() );
  res1ColId = col.id(); // for the next test
  QVERIFY( res1ColId > 0 );
  QCOMPARE( col.type(), Collection::Resource );
  QCOMPARE( col.parent(), Collection::root().id() );

  QVERIFY( findCol( list, "res2" ).isValid() );
  res2ColId = findCol( list, "res2" ).id();
  QVERIFY( res2ColId > 0 );
  QVERIFY( findCol( list, "res3" ).isValid() );
  res3ColId = findCol( list, "res3" ).id();
  QVERIFY( res3ColId > 0 );

#if 0
  col = findCol( list, "Search" );
  searchColId = col.id();
  QVERIFY( col.isValid() );
  QCOMPARE( col.type(), Collection::VirtualParent );
#endif
}

void CollectionJobTest::testFolderList( )
{
  // recursive list of physical folders
  CollectionListJob *job = new CollectionListJob( Collection( res1ColId ), CollectionListJob::Recursive );
  QVERIFY( job->exec() );
  Collection::List list = job->collections();

  // check if everything is there
  QCOMPARE( list.count(), 4 );
  Collection col;
  QList<QByteArray> contentTypes;

  col = findCol( list, "foo" );
  QVERIFY( col.isValid() );
  QCOMPARE( col.parent(), res1ColId );
  QCOMPARE( col.type(), Collection::Folder );
  contentTypes << "message/rfc822" << "text/calendar" << "text/vcard";
  compareLists( col.contentTypes(), contentTypes );

  QVERIFY( findCol( list, "bar" ).isValid() );
  QCOMPARE( findCol( list, "bar" ).parent(), col.id() );
  QVERIFY( findCol( list, "bla" ).isValid() );
}

void CollectionJobTest::testNonRecursiveFolderList( )
{
  CollectionListJob *job = new CollectionListJob( Collection( res1ColId ), CollectionListJob::Local );
  QVERIFY( job->exec() );
  Collection::List list = job->collections();

  QCOMPARE( list.count(), 1 );
  QVERIFY( findCol( list, "res1" ).isValid() );
}

void CollectionJobTest::testEmptyFolderList( )
{
  CollectionListJob *job = new CollectionListJob( Collection( res3ColId ), CollectionListJob::Flat );
  QVERIFY( job->exec() );
  Collection::List list = job->collections();

  QCOMPARE( list.count(), 0 );
}

#if 0
void CollectionJobTest::testSearchFolderList( )
{
  CollectionListJob *job = new CollectionListJob( Collection( searchColId ), CollectionListJob::Flat );
  QVERIFY( job->exec() );
  Collection::List list = job->collections();

  QCOMPARE( list.count(), 3 );
  Collection col = findCol( list, "Search/Test ?er" );
  QVERIFY( col.isValid() );
  QCOMPARE( col.type(), Collection::Virtual );
  QVERIFY( findCol( list, "all" ).isValid() );
  QVERIFY( findCol( list, "kde-core-devel" ).isValid() );
}
#endif

void CollectionJobTest::testResourceFolderList()
{
  // non-existing resource
  CollectionListJob *job = new CollectionListJob( Collection::root(), CollectionListJob::Flat );
  job->setResource( "i_dont_exist" );
  QVERIFY( !job->exec() );

  // recursive listing of all collections of an existing resource
  job = new CollectionListJob( Collection::root(), CollectionListJob::Recursive );
  job->setResource( "akonadi_dummy_resource_1" );
  QVERIFY( job->exec() );

  Collection::List list = job->collections();
  QCOMPARE( list.count(), 5 );
  QVERIFY( findCol( list, "res1" ).isValid() );
  QVERIFY( findCol( list, "foo" ).isValid() );
  QVERIFY( findCol( list, "bar" ).isValid() );
  QVERIFY( findCol( list, "bla" ).isValid() );
  int fooId = findCol( list, "foo" ).id();

  // limited listing of a resource
  job = new CollectionListJob( Collection( fooId ), CollectionListJob::Recursive );
  job->setResource( "akonadi_dummy_resource_1" );
  QVERIFY( job->exec() );

  list = job->collections();
  QCOMPARE( list.count(), 3 );
  QVERIFY( findCol( list, "bar" ).isValid() );
  QVERIFY( findCol( list, "bla" ).isValid() );
}

void CollectionJobTest::testIllegalCreateFolder( )
{
  // empty
  CollectionCreateJob *job = new CollectionCreateJob( Collection::root(), QString(), this );
  QVERIFY( !job->exec() );

#if 0
  // search folder
  job = new CollectionCreateJob( "/Search/New Folder", this );
  QVERIFY( !job->exec() );
#endif

  // already existing folder
  job = new CollectionCreateJob( Collection( res2ColId ), "foo2", this );
  QVERIFY( !job->exec() );

#if 0
  // Parent folder with \Noinferiors flag
  job = new CollectionCreateJob( "res2/foo2/bar", this );
  QVERIFY( !job->exec() );
#endif
}

void CollectionJobTest::testCreateDeleteFolder( )
{
  // simple new folder
  CollectionCreateJob *job = new CollectionCreateJob( Collection( res3ColId ), "new folder", this );
  QVERIFY( job->exec() );
  Collection newCol = job->collection();
  QVERIFY( newCol.isValid() );
  QCOMPARE( newCol.parent(), res3ColId );
  QCOMPARE( newCol.name(), QString( "new folder" ) );

  CollectionListJob *ljob = new CollectionListJob( Collection( res3ColId ), CollectionListJob::Flat, this );
  QVERIFY( ljob->exec() );
  QCOMPARE( findCol( ljob->collections(), "new folder" ), newCol );

  CollectionDeleteJob *del = new CollectionDeleteJob( "res3/new folder", this );
  QVERIFY( del->exec() );

  ljob = new CollectionListJob( Collection( res3ColId ), CollectionListJob::Flat, this );
  QVERIFY( ljob->exec() );
  QVERIFY( !findCol( ljob->collections(), "new folder" ).isValid() );

  // folder that already exists within another resource
  job = new CollectionCreateJob( Collection( res3ColId ), "foo", this );
  QVERIFY( job->exec() );
  newCol = job->collection();
  QVERIFY( newCol.isValid() );

  ljob = new CollectionListJob( Collection( res3ColId ), CollectionListJob::Flat, this );
  QVERIFY( ljob->exec() );
  QCOMPARE( findCol( ljob->collections(), "foo" ), newCol );

  del = new CollectionDeleteJob( "res3/foo", this );
  QVERIFY( del->exec() );

  ljob = new CollectionListJob( Collection( res3ColId ), CollectionListJob::Flat, this );
  QVERIFY( ljob->exec() );
  QVERIFY( !findCol( ljob->collections(), "res3/foo" ).isValid() );

  // folder with mime types
  job = new CollectionCreateJob( Collection( res3ColId ), "mail folder", this );
  QList<QByteArray> mimeTypes;
  mimeTypes << "inode/directory" << "message/rfc822";
  job->setContentTypes( mimeTypes );
  QVERIFY( job->exec() );
  newCol = job->collection();
  QVERIFY( newCol.isValid() );

  CollectionStatusJob *status = new CollectionStatusJob( newCol, this );
  QVERIFY( status->exec() );
  CollectionContentTypeAttribute *attr = extractAttribute<CollectionContentTypeAttribute>( status->attributes() );
  QVERIFY( attr != 0 );
  compareLists( attr->contentTypes(), mimeTypes );

  del = new CollectionDeleteJob( "res3/mail folder", this );
  QVERIFY( del->exec() );
}

void CollectionJobTest::testCreateDeleteFolderRecursive()
{
  // folder with missing parents
  CollectionCreateJob *job = new CollectionCreateJob( Collection( INT_MAX ), "sub1", this );
  QVERIFY( !job->exec() );
}

void CollectionJobTest::testStatus()
{
  // empty folder
  CollectionStatusJob *status = new CollectionStatusJob( Collection( res1ColId ), this );
  QVERIFY( status->exec() );

  QList<CollectionAttribute*> attrs = status->attributes();
  QCOMPARE( attrs.count(), 2 );
  MessageCollectionAttribute *mcattr = extractAttribute<MessageCollectionAttribute>( attrs );
  QVERIFY( mcattr != 0 );
  QCOMPARE( mcattr->count(), 0 );
  QCOMPARE( mcattr->unreadCount(), 0 );

  CollectionContentTypeAttribute *ctattr = extractAttribute<CollectionContentTypeAttribute>( attrs );
  QVERIFY( ctattr != 0 );
  QVERIFY( ctattr->contentTypes().isEmpty() );

#if 0
  // folder with attributes and content
  status = new CollectionStatusJob( "res1/foo", this );
  QVERIFY( status->exec() );

  attrs = status->attributes();
  QCOMPARE( attrs.count(), 2 );
  mcattr = extractAttribute<MessageCollectionAttribute>( attrs );
  QVERIFY( mcattr != 0 );
  QCOMPARE( mcattr->count(), 3 );
  QCOMPARE( mcattr->unreadCount(), 0 );

  ctattr = extractAttribute<CollectionContentTypeAttribute>( attrs );
  QVERIFY( ctattr != 0 );
  QList<QByteArray> mimeTypes = ctattr->contentTypes();
  QCOMPARE( mimeTypes.count(), 3 );
  QVERIFY( mimeTypes.contains( "text/calendar" ) );
  QVERIFY( mimeTypes.contains( "text/vcard" ) );
  QVERIFY( mimeTypes.contains( "message/rfc822" ) );
#endif
}

void CollectionJobTest::testModify()
{
  QList<QByteArray> reference;
  reference << "text/calendar" << "text/vcard" << "message/rfc822";

  Collection col;
  CollectionListJob *ljob = new CollectionListJob( Collection( res1ColId ), CollectionListJob::Flat );
  QVERIFY( ljob->exec() );
  col = findCol( ljob->collections(), "foo" );
  QVERIFY( col.isValid() );

  // test noop modify
  CollectionModifyJob *mod = new CollectionModifyJob( col, this );
  QVERIFY( mod->exec() );

  CollectionStatusJob *status = new CollectionStatusJob( col, this );
  QVERIFY( status->exec() );
  CollectionContentTypeAttribute *attr = extractAttribute<CollectionContentTypeAttribute>( status->attributes() );
  QVERIFY( attr != 0 );
  compareLists( attr->contentTypes(), reference );

  // test clearing content types
  mod = new CollectionModifyJob( col, this );
  mod->setContentTypes( QList<QByteArray>() );
  QVERIFY( mod->exec() );

  status = new CollectionStatusJob( col, this );
  QVERIFY( status->exec() );
  attr = extractAttribute<CollectionContentTypeAttribute>( status->attributes() );
  QVERIFY( attr != 0 );
  QVERIFY( attr->contentTypes().isEmpty() );

  // test setting contnet types
  mod = new CollectionModifyJob( col, this );
  mod->setContentTypes( reference );
  QVERIFY( mod->exec() );

  status = new CollectionStatusJob( col, this );
  QVERIFY( status->exec() );
  attr = extractAttribute<CollectionContentTypeAttribute>( status->attributes() );
  QVERIFY( attr != 0 );
  compareLists( attr->contentTypes(), reference );
}

void CollectionJobTest::testRename()
{
  CollectionRenameJob *rename = new CollectionRenameJob( "res1", "res1 (renamed)", this );
  QVERIFY( rename->exec() );

  CollectionListJob *ljob = new CollectionListJob( Collection( res1ColId ), CollectionListJob::Recursive );
  QVERIFY( ljob->exec() );
  Collection::List list = ljob->collections();

  QCOMPARE( list.count(), 4 );
  QVERIFY( findCol( list, "foo" ).isValid() );
  QVERIFY( findCol( list, "bar" ).isValid() );
  QVERIFY( findCol( list, "bla" ).isValid() );

  ljob = new CollectionListJob( Collection( res1ColId ), CollectionListJob::Local );
  QVERIFY( ljob->exec() );
  list = ljob->collections();

  QCOMPARE( list.count(), 1 );
  Collection col = list.first();
  QCOMPARE( col.name(), QLatin1String("res1 (renamed)") );
  QCOMPARE( col.parent(), Collection::root().id() );

  // cleanup
  rename = new CollectionRenameJob( "res1 (renamed)", "res1", this );
  QVERIFY( rename->exec() );
}

void CollectionJobTest::testIllegalRename()
{
  // non-existing collection
  CollectionRenameJob *rename = new CollectionRenameJob( "i dont exist", "i dont exist either", this );
  QVERIFY( !rename->exec() );

  // already existing target
  rename = new CollectionRenameJob( "res1", "res2", this );
  QVERIFY( !rename->exec() );

  // root being source or target
  rename = new CollectionRenameJob( Collection::root().path(), "some name", this );
  QVERIFY( !rename->exec() );

  rename = new CollectionRenameJob( "res1", Collection::root().path(), this );
  QVERIFY( !rename->exec() );
}

void CollectionJobTest::testUtf8CollectionName()
{
  QString folderName = QString::fromUtf8( "ä" );

  // create collection
  CollectionCreateJob *create = new CollectionCreateJob( Collection( res3ColId ), folderName, this );
  QVERIFY( create->exec() );
  Collection col = create->collection();
  QVERIFY( col.isValid() );

  // list parent
  CollectionListJob *list = new CollectionListJob( Collection( res3ColId ), CollectionListJob::Recursive, this );
  QVERIFY( list->exec() );
  QCOMPARE( list->collections().count(), 1 );
  QCOMPARE( col, list->collections().first() );
  QCOMPARE( col.name(), folderName );

  // modify collection
  CollectionModifyJob *modify = new CollectionModifyJob( col, this );
  QList<QByteArray> contentTypes;
  contentTypes << "message/rfc822";
  modify->setContentTypes( contentTypes );
  QVERIFY( modify->exec() );

  // collection status
  CollectionStatusJob *status = new CollectionStatusJob( col, this );
  QVERIFY( status->exec() );
  CollectionContentTypeAttribute *ccta = extractAttribute<CollectionContentTypeAttribute>( status->attributes() );
  QVERIFY( ccta );
  compareLists( ccta->contentTypes(), contentTypes );

  // delete collection
  CollectionDeleteJob *del = new CollectionDeleteJob( "res3/" + folderName, this );
  QVERIFY( del->exec() );
}

#include "collectionjobtest.moc"
