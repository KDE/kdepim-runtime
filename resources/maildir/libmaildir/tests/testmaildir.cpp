/*
  This file is part of the kpimutils library.

  Copyright (C) 2007 Till Adam <adam@kde.org>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License version 2 as published by the Free Software Foundation.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/


#include "testmaildir.h"
#include "testmaildir.moc"

#include <memory>

#include <QDir>
#include <QFile>

#include <qtest_kde.h>
#include <kstandarddirs.h>
#include <ktempdir.h>
#include <akonadi/kmime/messageflags.h>

QTEST_KDEMAIN( MaildirTest, NoGUI )

#include "../maildir.h"
using namespace KPIM;

static const char * testDir = "libmaildir-unit-test";
static const char * testString = "From: theDukeOfMonmouth@uk.gov\n\ntest\n";
static const char * testStringHeaders = "From: theDukeOfMonmouth@uk.gov\n";

void MaildirTest::init()
{
  m_temp = new KTempDir( KStandardDirs::locateLocal( "tmp", QLatin1String( testDir ) ) );

  QDir temp( m_temp->name() );
  QVERIFY( temp.exists() );

  temp.mkdir( QLatin1String( "new" ) );
  QVERIFY( temp.exists( QLatin1String( "new" ) ) );
  temp.mkdir( QLatin1String( "cur" ) );
  QVERIFY( temp.exists( QLatin1String( "cur" ) ) );
  temp.mkdir( QLatin1String( "tmp" ) );
  QVERIFY( temp.exists( QLatin1String( "tmp" ) ) );
}

void MaildirTest::cleanup()
{
  m_temp->unlink();
  QDir d( m_temp->name() );
  const QString subFolderPath( QString::fromLatin1( ".%1.directory" ).arg( d.dirName() ) );
  KTempDir::removeDir(subFolderPath);

  delete m_temp;
  m_temp = 0;
}

void MaildirTest::fillDirectory(const QString& name, int limit )
{
   QFile file;
   QDir::setCurrent( m_temp->name() + QLatin1Char( '/' ) + name );
   for ( int i=0; i<limit ; i++) {
     file.setFileName( QLatin1String( "testmail-" ) + QString::number( i ) );
     file.open( QIODevice::WriteOnly );
     file.write( testString );
     file.flush();
     file.close();
   }
}

void MaildirTest::createSubFolders()
{
  QDir d( m_temp->name() );
  const QString subFolderPath( QString::fromLatin1( ".%1.directory" ).arg( d.dirName() ) );
  d.cdUp();
  d.mkdir( subFolderPath );
  d.cd( subFolderPath );
  d.mkdir( QLatin1String( "foo" ) );
  d.mkdir( QLatin1String( "barbar" ) );
  d.mkdir( QLatin1String( "bazbaz" ) );
}

void MaildirTest::fillNewDirectory()
{
  fillDirectory( QLatin1String( "new" ), 140 );
}

void MaildirTest::fillCurrentDirectory()
{
  fillDirectory( QLatin1String( "cur" ), 20 );
}


void MaildirTest::testMaildirInstantiation()
{
  Maildir d( QLatin1String( "/foo/bar/Mail" ) );
  Maildir d2( d );
  Maildir d3;
  d3 = d;
  QVERIFY( d == d2 );
  QVERIFY( d3 == d2 );
  QVERIFY( d == d3 );
  QCOMPARE( d.path(), QString( QLatin1String( "/foo/bar/Mail" ) ) );
  QCOMPARE( d.name(), QString( QLatin1String( "Mail" ) ) );

  QVERIFY( !d.isValid() );

  Maildir good( m_temp->name() );
  QVERIFY( good.isValid() );

  QDir temp( m_temp->name() );
  temp.rmdir( QLatin1String( "new" ) );
  QString error;
  QVERIFY( !good.isValid( error ) );
  QVERIFY( !error.isEmpty() );

  Maildir root1( QLatin1String( "/foo/bar/Mail" ), true );
  QVERIFY( root1.isRoot() );

  Maildir root1Copy = root1;
  QCOMPARE( root1Copy.path(), root1.path() );
  QCOMPARE( root1Copy.isRoot(), root1.isRoot() );

  // FIXME test insufficient permissions?
}

void MaildirTest::testMaildirListing()
{
  fillNewDirectory();

  Maildir d( m_temp->name() );
  QStringList entries = d.entryList();

  QCOMPARE( entries.count(), 140 );

  fillCurrentDirectory();
  entries = d.entryList();
  QCOMPARE( entries.count(), 160 );
}

void MaildirTest::testMaildirAccess()
{
  fillCurrentDirectory();
  Maildir d( m_temp->name() );
  QStringList entries = d.entryList();
  QCOMPARE( entries.count(), 20 );

  QByteArray data = d.readEntry( entries[0] );
  QVERIFY( !data.isEmpty() );
  QCOMPARE( data, QByteArray( testString ) );
}

void MaildirTest::testMaildirReadHeaders()
{
  fillCurrentDirectory();
  Maildir d( m_temp->name() );
  QStringList entries = d.entryList();
  QCOMPARE( entries.count(), 20 );

  QByteArray data = d.readEntryHeaders( entries[0] );
  QVERIFY( !data.isEmpty() );
  QCOMPARE( data, QByteArray( testStringHeaders ) );
}

void MaildirTest::testMaildirWrite()
{
  fillCurrentDirectory();
  Maildir d( m_temp->name() );
  QStringList entries = d.entryList();
  QCOMPARE( entries.count(), 20 );

  QByteArray data = d.readEntry( entries[0] );
  QByteArray data2 = "changed\n";
  d.writeEntry( entries[0], data2 );
  QCOMPARE( data2, d.readEntry( entries[0] ) );
}

void MaildirTest::testMaildirAppend()
{
  Maildir d( m_temp->name() );
  QByteArray data = "newentry\n";
  QString key = d.addEntry( data );
  QVERIFY( !key.isEmpty() );
  QCOMPARE( data, d.readEntry( key ) );
}

void MaildirTest::testMaildirCreation()
{
  QString p( QLatin1String( "CREATETEST" ) );
  std::auto_ptr<KTempDir> temp ( new KTempDir( KStandardDirs::locateLocal( "tmp", p ) ) );
  Maildir d( temp->name() + p );
  QVERIFY( !d.isValid() );
  d.create();
  QVERIFY( d.isValid() );
}

void MaildirTest::testMaildirRemoveEntry()
{
  Maildir d( m_temp->name() );
  QByteArray data = "newentry\n";
  QString key = d.addEntry( data );
  QVERIFY( !key.isEmpty() );
  QCOMPARE( data, d.readEntry( key ) );
  QVERIFY( d.removeEntry( key ) );
  QVERIFY( d.readEntry( key ).isEmpty() );
}

void MaildirTest::testMaildirListSubfolders()
{
  fillNewDirectory();

  Maildir d( m_temp->name() );
  QStringList entries = d.subFolderList();

  QVERIFY( entries.isEmpty() );

  createSubFolders();

  entries = d.subFolderList();
  QVERIFY( !entries.isEmpty() );
  QCOMPARE( entries.count(), 3 );
}


void MaildirTest::testMaildirCreateSubfolder()
{
  Maildir d( m_temp->name() );
  QStringList entries = d.subFolderList();
  QVERIFY( entries.isEmpty() );

  d.addSubFolder( QLatin1String( "subFolderTest" ) );
  entries = d.subFolderList();
  QVERIFY( !entries.isEmpty() );
  QCOMPARE( entries.count(), 1 );
  Maildir child = d.subFolder( entries.first() );
  QVERIFY( child.isValid() );
}

void MaildirTest::testMaildirRemoveSubfolder()
{
  Maildir d( m_temp->name() );
  QVERIFY( d.isValid() );

  QString folderPath = d.addSubFolder( QLatin1String( "subFolderTest" ) );
  QVERIFY( !folderPath.isEmpty() );
  QVERIFY( folderPath.endsWith( QLatin1String( ".directory/subFolderTest" ) ) );
  bool removingWorked = d.removeSubFolder( QLatin1String( "subFolderTest" ) );
  QVERIFY( removingWorked );
}

void MaildirTest::testMaildirRename()
{
  Maildir d( m_temp->name() );
  QVERIFY( d.isValid() );

  QString folderPath = d.addSubFolder( QLatin1String( "rename me!" ) );
  QVERIFY( !folderPath.isEmpty() );

  Maildir d2( folderPath );
  QVERIFY( d2.isValid() );
  QVERIFY( d2.rename( QLatin1String( "renamed" ) ) );
  QCOMPARE( d2.name(), QString( QLatin1String( "renamed" ) ) );

  // same again, should not fail
  QVERIFY( d2.rename( QLatin1String( "renamed" ) ) );
  QCOMPARE( d2.name(), QString( QLatin1String( "renamed" ) ) );

  // already existing name
  QVERIFY( !d.addSubFolder( QLatin1String( "this name is already taken" ) ).isEmpty() );
  QVERIFY( !d2.rename( QLatin1String( "this name is already taken" ) ) );
}

void MaildirTest::testMaildirMoveTo()
{
  Maildir d( m_temp->name() );
  QVERIFY( d.isValid() );

  QString folderPath1 = d.addSubFolder( QLatin1String( "child1" ) );
  QVERIFY( !folderPath1.isEmpty() );

  Maildir d2( folderPath1 );
  QVERIFY( d2.isValid() );

  QDir d2Dir( d2.path() );
  QVERIFY( d2Dir.exists() );

  QString folderPath11 = d2.addSubFolder( QLatin1String( "grandchild1" ) );

  Maildir d21( folderPath11 );
  QVERIFY( d21.isValid() );

  QDir d2SubDir( Maildir::subDirPathForFolderPath( d2.path() ) );
  QVERIFY( d2SubDir.exists() );

  QString folderPath2 = d.addSubFolder( QLatin1String( "child2" ) );
  QVERIFY( !folderPath2.isEmpty() );

  Maildir d3( folderPath2 );
  QVERIFY( d3.isValid() );

  // move child1 to child2
  QVERIFY( d2.moveTo( d3 ) );

  Maildir d31 = d3.subFolder( QLatin1String( "child1" ) );
  QVERIFY( d31.isValid() );

  QVERIFY( !d2Dir.exists() );
  QVERIFY( !d2SubDir.exists() );

  QDir d31Dir( d31.path() );
  QVERIFY( d31Dir.exists() );

  QDir d31SubDir( Maildir::subDirPathForFolderPath( d31.path() ) );
  QVERIFY( d31SubDir.exists() );

  Maildir d311 = d31.subFolder( QLatin1String( "grandchild1" ) );
  QVERIFY( d311.isValid() );

  // try moving again
  d2 = Maildir( folderPath1 );
  QVERIFY( !d2.isValid() );
  QVERIFY( !d2.moveTo( d3 ) );
}

void MaildirTest::testMaildirFlagsReading()
{
  QFile file;
  const QStringList markers = QStringList() << "P" << "R" << "S" << "F" << "FPRS";
  QDir::setCurrent( m_temp->name() + QLatin1Char( '/' ) + "cur" );
  for ( int i=0; i<6 ; i++) {
    QString fileName = QLatin1String( "testmail-" ) + QString::number( i );
    if ( i < 5 ) {
      fileName +=
  #ifdef Q_OS_WIN
                      "!2,"
  #else
                      ":2,"
  #endif
                      + markers[i];
    }
    file.setFileName( fileName );
    file.open( QIODevice::WriteOnly );
    file.write( testString );
    file.flush();
    file.close();
  }

  Maildir d( m_temp->name() );
  QStringList entries = d.entryList();
  QCOMPARE( entries.count(), 6 );

  Akonadi::Item::Flags flags = d.readEntryFlags( entries[0] );
  QCOMPARE( flags.count(), 1 );
  QVERIFY( flags.contains( Akonadi::MessageFlags::Forwarded ) );

  flags = d.readEntryFlags( entries[1] );
  QCOMPARE( flags.count(), 1 );
  QVERIFY( flags.contains( Akonadi::MessageFlags::Replied ) );

  flags = d.readEntryFlags( entries[2] );
  QCOMPARE( flags.count(), 1 );
  QVERIFY( flags.contains( Akonadi::MessageFlags::Seen ) );

  flags = d.readEntryFlags( entries[3] );
  QCOMPARE( flags.count(), 1 );
  QVERIFY( flags.contains( Akonadi::MessageFlags::Flagged ) );

  flags = d.readEntryFlags( entries[4] );
  QCOMPARE( flags.count(), 4 );
  QVERIFY( flags.contains( Akonadi::MessageFlags::Forwarded ) );
  QVERIFY( flags.contains( Akonadi::MessageFlags::Replied ) );
  QVERIFY( flags.contains( Akonadi::MessageFlags::Seen ) );
  QVERIFY( flags.contains( Akonadi::MessageFlags::Flagged ) );

  flags = d.readEntryFlags( entries[5] );
  QVERIFY( flags.isEmpty() );
}

void MaildirTest::testMaildirFlagsWriting_data()
{
  QTest::addColumn<QString>( "origDir" );
  QTest::addColumn<QString>( "origFileName" );
  QTest::newRow( "cur/" ) << "cur" << "testmail";
  QTest::newRow( "cur/S" ) << "cur" << "testmail:2,S"; // wrongly marked as "seen" on disk (#289428)
  QTest::newRow( "new/" ) << "new" << "testmail";
  QTest::newRow( "new/S" ) << "new" << "testmail:2,S";
}

void MaildirTest::testMaildirFlagsWriting()
{
  QFETCH( QString, origDir );
  QFETCH( QString, origFileName );

  // create an initialy new mail
  QFile file;
  QDir::setCurrent( m_temp->name() );
  file.setFileName( origDir + '/' + origFileName );
  file.open( QIODevice::WriteOnly );
  file.write( testString );
  file.flush();
  file.close();

  // add a single flag
  Maildir d( m_temp->name() );
  const QStringList entries = d.entryList();
  QCOMPARE( entries.size(), 1 );
  QVERIFY( QFile::exists( origDir + '/' + entries[0] ) );
  const QString newKey = d.changeEntryFlags( entries[0], Akonadi::Item::Flags() << Akonadi::MessageFlags::Seen );
  // make sure the new key exists
  QCOMPARE( newKey, d.entryList()[0] );
  QVERIFY( QFile::exists( "cur/" + newKey ) );
  // and it's the right file
  QCOMPARE( d.readEntry( newKey ), QByteArray( testString ) );
  // now check the file name
  QVERIFY( newKey.endsWith( QLatin1String( "2,S" ) ) );
  // and more flags
  const QString newKey2 = d.changeEntryFlags( newKey, Akonadi::Item::Flags() << Akonadi::MessageFlags::Seen << Akonadi::MessageFlags::Replied );
  // check the file name, and the sorting of markers
  QVERIFY( newKey2.endsWith( QLatin1String( "2,RS" ) ) );
  QVERIFY( QFile::exists( "cur/" + newKey2 ) );
}
