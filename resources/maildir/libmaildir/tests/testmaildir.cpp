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

QTEST_KDEMAIN_CORE( MaildirTest )

#include "../maildir.h"
using namespace KPIM;

static const char * testDir = "libmaildir-unit-test";
static const char * testString = "From: theDukeOfMonmouth@uk.gov\n\ntest\n";
static const char * testStringHeaders = "From: theDukeOfMonmouth@uk.gov\n";

void MaildirTest::initTestCase()
{
  m_temp = new KTempDir( KStandardDirs::locateLocal("tmp", testDir ) );

  QDir temp( m_temp->name() );
  QVERIFY( temp.exists() );

  temp.mkdir("new");
  QVERIFY( temp.exists( "new" ) );
  temp.mkdir("cur");
  QVERIFY( temp.exists( "cur" ) );
  temp.mkdir("tmp");
  QVERIFY( temp.exists( "tmp") );
}

void MaildirTest::fillDirectory(const QString& name, int limit )
{
   QFile file;
   QDir::setCurrent( m_temp->name() + "/" + name );
   for ( int i=0; i<limit ; i++) {
     file.setFileName("testmail-" + QString::number(i) );
     file.open( QIODevice::WriteOnly );
     file.write( testString );
     file.flush();
     file.close();
   }
}

void MaildirTest::createSubFolders()
{
  QDir d( m_temp->name() );
  const QString subFolderPath(QString::fromLatin1(".%1.directory" ).arg( d.dirName() ));
  d.cdUp();
  d.mkdir( subFolderPath );
  d.cd( subFolderPath );
  d.mkdir( "foo" );
  d.mkdir( "barbar" );
  d.mkdir( "bazbaz" );
}

void MaildirTest::fillNewDirectory()
{
  fillDirectory("new", 140);
}

void MaildirTest::fillCurrentDirectory()
{
  fillDirectory("cur", 20);
}


void MaildirTest::testMaildirInstantiation()
{
  Maildir d( "/foo/bar/Mail");
  Maildir d2( d );
  Maildir d3;
  d3 = d;
  QVERIFY(d == d2);
  QVERIFY(d3 == d2);
  QVERIFY(d == d3);

  QVERIFY(!d.isValid());

  Maildir good( m_temp->name() );
  QVERIFY(good.isValid());

  QDir temp( m_temp->name() );
  temp.rmdir( "new");
  QString error;
  QVERIFY(!good.isValid( error ));
  QVERIFY(!error.isEmpty());

  // FIXME test insufficient permissions?
  cleanupTestCase();
}

void MaildirTest::testMaildirListing()
{
  initTestCase();
  fillNewDirectory();

  Maildir d( m_temp->name() );
  QStringList entries = d.entryList();

  QCOMPARE( entries.count(), 140);

  fillCurrentDirectory();
  entries = d.entryList();
  QCOMPARE( entries.count(), 160 );
  cleanupTestCase();
}

void MaildirTest::testMaildirAccess()
{
  initTestCase();
  fillCurrentDirectory();
  Maildir d( m_temp->name() );
  QStringList entries = d.entryList();
  QCOMPARE( entries.count(), 20 );

  QByteArray data = d.readEntry( entries[0] );
  QVERIFY(!data.isEmpty());
  QCOMPARE( data, QByteArray( testString) );
}

void MaildirTest::testMaildirReadHeaders()
{
  Maildir d( m_temp->name() );
  QStringList entries = d.entryList();
  QCOMPARE( entries.count(), 20 );

  QByteArray data = d.readEntryHeaders( entries[0] );
  QVERIFY(!data.isEmpty());
  QCOMPARE( data, QByteArray( testStringHeaders ) );
}



void MaildirTest::testMaildirWrite()
{
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
  QString p("CREATETEST");
  std::auto_ptr<KTempDir> temp ( new KTempDir( KStandardDirs::locateLocal("tmp", p ) ) );
  Maildir d( temp->name() + p );
  QVERIFY(!d.isValid());
  d.create();
  QVERIFY(d.isValid());
}

void MaildirTest::testMaildirRemoveEntry()
{
  Maildir d( m_temp->name() );
  QByteArray data = "newentry\n";
  QString key = d.addEntry( data );
  QVERIFY( !key.isEmpty() );
  QCOMPARE( data, d.readEntry( key ) );
  QVERIFY( d.removeEntry( key ) );
  QVERIFY( d.readEntry(key).isEmpty() );
}

void MaildirTest::testMaildirListSubfolders()
{
  initTestCase();
  fillNewDirectory();

  Maildir d( m_temp->name() );
  QStringList entries = d.subFolderList();

  QVERIFY( entries.isEmpty() );

  createSubFolders();

  entries = d.subFolderList();
  QVERIFY( !entries.isEmpty() );
  QCOMPARE( entries.count(), 3 );

  cleanupTestCase();
}


void MaildirTest::testMaildirCreateSubfolder()
{
  initTestCase();
  Maildir d( m_temp->name() );
  QStringList entries = d.subFolderList();
  QVERIFY( entries.isEmpty() );

  d.addSubFolder( "subFolderTest" );
  entries = d.subFolderList();
  QVERIFY( !entries.isEmpty() );
  QCOMPARE( entries.count(), 1 );
  Maildir child = d.subFolder( entries.first() );
  QVERIFY( child.isValid() );

  cleanupTestCase();
}

void MaildirTest::testMaildirRemoveSubfolder()
{
  initTestCase();
  Maildir d( m_temp->name() );
  QVERIFY( d.isValid() );

  QString folderPath = d.addSubFolder( "subFolderTest" );
  QVERIFY( !folderPath.isEmpty() );
  QVERIFY( folderPath.endsWith( ".directory/subFolderTest" ) );
  bool removingWorked = d.removeSubFolder( "subFolderTest" );
  QVERIFY( removingWorked );

  cleanupTestCase();
}

void MaildirTest::cleanupTestCase()
{
  m_temp->unlink();
}

