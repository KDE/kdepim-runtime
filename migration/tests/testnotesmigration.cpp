/*
    Copyright 2010 Stephen Kelly <steveire@gmail.com>

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

#include "testnotesmigration.h"

#include <QtCore/QObject>

#include <Akonadi/Collection>
#include <Akonadi/EntityTreeModel>
#include <Akonadi/ChangeRecorder>
#include <Akonadi/ItemFetchScope>
#include <Akonadi/EntityDisplayAttribute>

#include <akonadi/qtest_akonadi.h>
#include <QtTest/qtestcase.h>

#include "kjots/kjotsmigrator.h"
#include "kres/knotesmigrator.h"

#include <KMime/KMimeMessage>

using namespace Akonadi;

void NotesMigrationTest::initTestCase()
{
  ItemFetchScope scope;
  scope.fetchFullPayload( true ); // Need to have full item when adding it to the internal data structure
  scope.fetchAttribute< EntityDisplayAttribute >();

  Akonadi::ChangeRecorder *changeRecorder = new Akonadi::ChangeRecorder( this );
  changeRecorder->fetchCollection( true );
  changeRecorder->setItemFetchScope( scope );
  changeRecorder->setCollectionMonitored( Collection::root() );
  changeRecorder->setMimeTypeMonitored( "text/x-vnd.akonadi.note" );

  m_etm = new Akonadi::EntityTreeModel( changeRecorder, this );
  connect( m_etm, SIGNAL(rowsInserted(QModelIndex,int,int)), SLOT(checkRowsInserted(QModelIndex,int,int)));

}

void NotesMigrationTest::testKJotsBooksMigration()
{
  KJotsMigrator *migrator = new KJotsMigrator;

  m_expectedStructure.insert( "Local Notes", ( QStringList() << "rich content book" << "Something" << "Book2" ) );
  m_expectedStructure.insert( "Something", ( QStringList() << "Page 1" << "Page 2" ) );
  m_expectedStructure.insert( "Book2", ( QStringList() << "Page 1" << "Page 2" << "Nested book" ) );
  m_expectedStructure.insert( "Nested book", ( QStringList() << "nested page 1" << "nested page 2" ) );
  m_expectedStructure.insert( "rich content book", ( QStringList() << "rich content page 1" << "rich content page 2" ) );

  QTest::qWait( 5000 );

  QHashIterator<QString, QStringList> it( m_expectedStructure );
  while ( it.hasNext() )
  {
    it.next();
    QString key = it.key();
    QStringList value = it.value();
    value.sort();
    QVERIFY( m_seenStructure.contains( key ) );
    QStringList seenValue = m_seenStructure.value( key );
    seenValue.sort();
    QVERIFY( value == seenValue );
  }
}

void NotesMigrationTest::checkRowsInserted( const QModelIndex &parent, int start, int end )
{
  int rowCount = m_etm->rowCount( parent );
  static const int column = 0;
  if ( !parent.isValid() && !m_expectedStructure.isEmpty() )
  {
    Collection resourceRootCollection = m_etm->index(start, column, parent).data( EntityTreeModel::CollectionRole ).value<Akonadi::Collection>();
    QVERIFY( resourceRootCollection.isValid() );
    QVERIFY( resourceRootCollection.name() == "Local Notes" );
    QVERIFY( resourceRootCollection.contentMimeTypes() == ( QStringList() << Akonadi::Collection::mimeType() << "text/x-vnd.akonadi.note" ) );
    QVERIFY( resourceRootCollection.parentCollection() == Akonadi::Collection::root() );
    m_seenStructure.insert( resourceRootCollection.name(), QStringList() );
    return;
  }
  for (int row = start; row <= end; ++row )
  {
    QModelIndex index = m_etm->index( row, column, parent );

    Collection newCollection = index.data( EntityTreeModel::CollectionRole ).value<Akonadi::Collection>();
    QString parentName = parent.data().toString();
    if ( newCollection.isValid() )
    {
      // This is a new collection in the resource.
      QVERIFY( newCollection.contentMimeTypes() == ( QStringList() << Akonadi::Collection::mimeType() << "text/x-vnd.akonadi.note" ) );
      if ( !m_expectedStructure.isEmpty() )
      {
        QVERIFY( m_expectedStructure[ parentName ].contains( index.data().toString() ) );
        m_seenStructure[ parentName ].append( index.data().toString() );
      }
      else
        m_seenNotes.append( index.data().toString() );
    } else {
      Item newItem = index.data( EntityTreeModel::ItemRole ).value<Akonadi::Item>();
      QVERIFY( newItem.isValid() );
      QVERIFY( newItem.hasPayload<KMime::Message::Ptr>() );
      KMime::Message::Ptr note = newItem.payload<KMime::Message::Ptr>();

      if ( !m_expectedStructure.isEmpty() )
      {
        QVERIFY( m_expectedStructure[ parentName ].contains( note->subject()->asUnicodeString() ) );
        m_seenStructure[ parentName ].append( note->subject()->asUnicodeString() );
      } else {
        m_seenNotes.append( note->subject()->asUnicodeString() );
      }
    }
  }
}

void NotesMigrationTest::testLocalKNotesMigration()
{
  m_expectedStructure.clear();
  m_seenStructure.clear();

  m_expectedNotes << "Notes" << "2010-02-08 12:12" << "2010-02-08 12:29";

  KNotesMigrator *migrator = new KNotesMigrator;

  QTest::qWait( 2000 );

  m_expectedNotes.sort();
  m_seenNotes.sort();

  kDebug() << m_seenNotes << m_expectedNotes;
  QVERIFY( m_seenNotes == m_expectedNotes );
}

QTEST_AKONADIMAIN( NotesMigrationTest, NoGUI )
