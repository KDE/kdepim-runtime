/*
  Copyright (C) 2009 Bertjan Broeksema <b.broeksema@kdemail.net>

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


#include "mboxtest.h"
#include "mboxtest.moc"

#include <QtCore/QDir>
#include <QtCore/QFile>

#include <qtest_kde.h>
#include <kstandarddirs.h>
#include <ktempdir.h>

QTEST_KDEMAIN_CORE(MboxTest)

#include "test-entries.h"

static const char * testDir = "libmbox-unit-test";
static const char * testFile = "test-mbox-file";
static const char * testLockFile = "test-mbox-lock-file";

QString MboxTest::fileName()
{
  return mTempDir->name() + testFile;
}

QString MboxTest::lockFileName()
{
  return mTempDir->name() + testLockFile;
}

void MboxTest::removeTestFile()
{
  QFile file( fileName() );
  file.remove();
  QVERIFY( !file.exists() );
}

void MboxTest::initTestCase()
{
  mTempDir = new KTempDir( KStandardDirs::locateLocal("tmp", testDir ) );

  QDir temp(mTempDir->name());
  QVERIFY(temp.exists());

  QFile mboxfile( fileName() );
  mboxfile.open( QIODevice::WriteOnly );
  mboxfile.close();
  QVERIFY(mboxfile.exists());

  mMail1 = MessagePtr( new KMime::Message );
  mMail1->setContent( KMime::CRLFtoLF( sEntry1 ) );
  mMail1->parse();

  mMail2 = MessagePtr( new KMime::Message );
  mMail2->setContent( KMime::CRLFtoLF( sEntry2 ) );
  mMail2->parse();

}

void MboxTest::testSetLockMethod()
{
  MBox mbox1;

  if ( !KStandardDirs::findExe( "lockfile" ).isEmpty() ) {
    QVERIFY( mbox1.setLockType(MBox::ProcmailLockfile) );
  } else {
    QVERIFY( !mbox1.setLockType( MBox::ProcmailLockfile ) );
  }

  if ( !KStandardDirs::findExe("mutt_dotlock").isEmpty() ) {
    QVERIFY( mbox1.setLockType( MBox::MuttDotlock ) );
    QVERIFY( mbox1.setLockType( MBox::MuttDotlockPrivileged ) );
  } else {
    QVERIFY( !mbox1.setLockType( MBox::MuttDotlock ) );
    QVERIFY( !mbox1.setLockType( MBox::MuttDotlockPrivileged ) );
  }

  QVERIFY( mbox1.setLockType( MBox::None ) );
}

void MboxTest::testLockBeforeLoad()
{
  // Should fail because it's not known which file to lock.
  MBox mbox;

  if ( !KStandardDirs::findExe( "lockfile" ).isEmpty() ) {
    QVERIFY( mbox.setLockType(MBox::ProcmailLockfile) );
    QVERIFY( !mbox.lock() );
  }

  if ( !KStandardDirs::findExe("mutt_dotlock").isEmpty() ) {
    QVERIFY( mbox.setLockType( MBox::MuttDotlock ) );
    QVERIFY( !mbox.lock() );
    QVERIFY( mbox.setLockType( MBox::MuttDotlockPrivileged ) );
    QVERIFY( !mbox.lock() );
  }

  QVERIFY( mbox.setLockType( MBox::None ) );
  QVERIFY( !mbox.lock() );
}

void MboxTest::testProcMailLock()
{
  // It really only makes sense to test this if the lockfile executable can be
  // found.

  MBox mbox;
  if ( !mbox.setLockType( MBox::ProcmailLockfile ) ) {
    QEXPECT_FAIL( "", "This test only works when procmail is installed.", Abort );
    QVERIFY( false );
  }

  QVERIFY( mbox.load( fileName() ) );

  // By default the filename is used as part of the lockfile filename.
  QVERIFY( !QFile( fileName() + ".lock" ).exists() );
  QVERIFY( mbox.lock() );
  QVERIFY( QFile( fileName() + ".lock" ).exists() );
  QVERIFY( mbox.unlock() );
  QVERIFY( !QFile( fileName() + ".lock" ).exists() );

  mbox.setLockFile( lockFileName() );
  QVERIFY( !QFile( lockFileName() ).exists() );
  QVERIFY( mbox.lock() );
  QVERIFY( QFile( lockFileName() ).exists() );
  QVERIFY( mbox.unlock() );
  QVERIFY( !QFile( lockFileName() ).exists() );
}

void MboxTest::testAppend()
{
  QFileInfo info( fileName() );
  QCOMPARE( info.size(), static_cast<qint64>( 0 ) );

  MBox mbox;
  mbox.setLockType( MBox::None );

  // When no file is loaded no entries should get added to the mbox.
  QCOMPARE( mbox.entryList().size(), 0 );
  QCOMPARE( mbox.appendEntry( mMail1 ), static_cast<qint64>( -1 ) );
  QCOMPARE( mbox.entryList().size(), 0 );

  QVERIFY( mbox.load( fileName() ) );

  // First message added to an emtpy file should be at offset 0
  QCOMPARE( mbox.entryList().size(), 0 );
  QCOMPARE( mbox.appendEntry( mMail1 ), static_cast<qint64>( 0 ) );
  QCOMPARE( mbox.entryList().size(), 1 );
  QCOMPARE( mbox.entryList().first().second, static_cast<quint64>( sEntry1.size() ) );

  QVERIFY( mbox.appendEntry( mMail2 ) > sEntry1.size() );
  QCOMPARE( mbox.entryList().size(), 2 );
  QCOMPARE( mbox.entryList().last().second, static_cast<quint64>( sEntry2.size() ) );
}

void MboxTest::testSaveAndLoad()
{
  removeTestFile();

  MBox mbox;
  QVERIFY( mbox.setLockType( MBox::None ) );
  QVERIFY( mbox.load( fileName() ) );
  mbox.appendEntry( mMail1 );
  mbox.appendEntry( mMail2 );

  QList<MsgInfo> infos1 = mbox.entryList();
  QCOMPARE( infos1.size(), 2 );

  QVERIFY( mbox.save() );
  QVERIFY( QFileInfo( fileName() ).exists() );

  QList<MsgInfo> infos2 = mbox.entryList();
  QCOMPARE( infos2.size(), 2 );

  for ( int i = 0; i < 2; ++i ) {
    QCOMPARE( infos1.at(i).first, infos2.at(i).first );
    QCOMPARE( infos1.at(i).second, infos2.at(i).second );
  }

  MBox mbox2;
  QVERIFY( mbox2.setLockType( MBox::None ) );
  QVERIFY( mbox2.load( fileName() ) );

  QList<MsgInfo> infos3 = mbox2.entryList();
  QCOMPARE( infos3.size(), 2 );

  for ( int i = 0; i < 2; ++i ) {
    QCOMPARE( infos3.at(i).first, infos2.at(i).first );

    quint64 minSize = infos2.at(i).second;
    quint64 maxSize = infos2.at(i).second + 1;
    QVERIFY( infos3.at(i).second >= minSize  );
    QVERIFY( infos3.at(i).second <= maxSize  );
  }
}

void MboxTest::testBlankLines()
{
  for ( int i = 0; i < 5; ++i ) {
    removeTestFile();

    MessagePtr mail = MessagePtr( new KMime::Message );
    mail->setContent( KMime::CRLFtoLF( sEntry1 + QByteArray( i, '\n' ) ) );
    mail->parse();

    MBox mbox1;
    QVERIFY( mbox1.setLockType( MBox::None ) );
    QVERIFY( mbox1.load( fileName() ) );
    mbox1.appendEntry( mail );
    mbox1.appendEntry( mail );
    mbox1.appendEntry( mail );
    mbox1.save();

    MBox mbox2;
    QVERIFY( mbox1.setLockType( MBox::None ) );
    QVERIFY( mbox1.load( fileName() ) );
    QCOMPARE( mbox1.entryList().size(), 3 );

    quint64 minSize = sEntry1.size() + i - 1; // Possibly on '\n' falls off.
    quint64 maxSize = sEntry1.size() + i;
    for ( int i = 0; i < 3; ++i ) {
      QVERIFY( mbox1.entryList().at( i ).second >= minSize  );
      QVERIFY( mbox1.entryList().at( i ).second <= maxSize  );
    }
  }
}

void MboxTest::testEntries()
{
  removeTestFile();

  MBox mbox1;
  QVERIFY( mbox1.setLockType( MBox::None ) );
  QVERIFY( mbox1.load( fileName() ) );
  mbox1.appendEntry( mMail1 );
  mbox1.appendEntry( mMail2 );
  mbox1.appendEntry( mMail1 );

  QList<MsgInfo> infos = mbox1.entryList();
  QCOMPARE( infos.size() , 3 );

  QSet<quint64> deletedEntries;
  deletedEntries << infos.at( 0 ).first;

  QList<MsgInfo> infos2 = mbox1.entryList( deletedEntries );
  QCOMPARE( infos2.size() , 2 );
  QVERIFY( infos2.first().first != infos.first().first );
  QVERIFY( infos2.last().first != infos.first().first );

  deletedEntries << infos.at( 1 ).first;
  infos2 = mbox1.entryList( deletedEntries );

  QCOMPARE( infos2.size() , 1 );
  QVERIFY( infos2.first().first != infos.at( 0 ).first );
  QVERIFY( infos2.first().first != infos.at( 1 ).first );

  deletedEntries << infos.at( 2 ).first;
  infos2 = mbox1.entryList( deletedEntries );
  QCOMPARE( infos2.size() , 0 );

  QVERIFY( !deletedEntries.contains( 10 ) ); // some random offset
  infos2 = mbox1.entryList( QSet<quint64>() << 10 );
  QCOMPARE( infos2.size() , 3 );
  QCOMPARE( infos2.at( 0 ).first, infos.at( 0 ).first );
  QCOMPARE( infos2.at( 1 ).first, infos.at( 1 ).first );
  QCOMPARE( infos2.at( 2 ).first, infos.at( 2 ).first );
}

void MboxTest::testPurge()
{
  MBox mbox1;
  QVERIFY( mbox1.setLockType( MBox::None ) );
  QVERIFY( mbox1.load( fileName() ) );
  mbox1.appendEntry( mMail1 );
  mbox1.appendEntry( mMail1 );
  mbox1.appendEntry( mMail1 );
  QVERIFY( mbox1.save() );

  QList<MsgInfo> list = mbox1.entryList();

  // First test: Delete only the first (all messages afterwards have to be moved).
  mbox1.purge( QSet<quint64>() << list.first().first );

  MBox mbox2;
  QVERIFY( mbox2.load( fileName() ) );
  QList<MsgInfo> list2 = mbox2.entryList();
  QCOMPARE( list2.size(), 2 ); // Is a message actually gone?

  quint64 newOffsetSecondMessage = list.last().first - list.at( 1 ).first;

  QCOMPARE( list2.first().first, static_cast<quint64>( 0 ) );
  QCOMPARE( list2.last().first, newOffsetSecondMessage );

  // Second test: Delete the first two (the last message have to be moved).
  removeTestFile();

  QVERIFY( mbox1.load( fileName() ) );
  mbox1.appendEntry( mMail1 );
  mbox1.appendEntry( mMail1 );
  mbox1.appendEntry( mMail1 );
  QVERIFY( mbox1.save() );

  list = mbox1.entryList();

  mbox1.purge( QSet<quint64>() << list.at( 0 ).first << list.at( 1 ).first );
  QVERIFY( mbox2.load( fileName() ) );
  list2 = mbox2.entryList();
  QCOMPARE( list2.size(), 1 ); // Are the messages actually gone?
  QCOMPARE( list2.first().first, static_cast<quint64>( 0 ) );

  // Third test: Delete all messages.
  removeTestFile();

  QVERIFY( mbox1.load( fileName() ) );
  mbox1.appendEntry( mMail1 );
  mbox1.appendEntry( mMail1 );
  mbox1.appendEntry( mMail1 );
  QVERIFY( mbox1.save() );

  list = mbox1.entryList();

  mbox1.purge( QSet<quint64>() << list.at( 0 ).first << list.at( 1 ).first << list.at( 2 ).first );
  QVERIFY( mbox2.load( fileName() ) );
  list2 = mbox2.entryList();
  QCOMPARE( list2.size(), 0 ); // Are the messages actually gone?
}

void MboxTest::cleanupTestCase()
{
  mTempDir->unlink();
}
