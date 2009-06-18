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

#include "../mbox.h"

static const char * testDir = "libmbox-unit-test";
static const char * testFile = "test-mbox-file";

QString MboxTest::fileName()
{
  return mTempDir->name() + testFile;
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
  QEXPECT_FAIL("", "KDELockFile method is not yet implmented", Continue);
  QVERIFY( mbox1.setLockType( MBox::KDELockFile ) );
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

  QEXPECT_FAIL("", "KDELockFile method is not yet implmented", Continue);
  QVERIFY( mbox.setLockType( MBox::KDELockFile ) );
  QVERIFY( !mbox.lock() );
}

void MboxTest::testProcMailLock()
{
  // It really only makes sense to test this if the lockfile executable can be
  // found.
  /*
  MBox mbox(fileName(), true);

  QEXPECT_FAIL("", "This only works when procmail is installed.", Continue);
  QVERIFY( mbox.setLockType( MBox::ProcmailLockfile ) );
  if (!KStandardDirs::findExe("lockfile").isEmpty()) {
    QVERIFY(!QFile(fileName() + ".lock").exists());
    QCOMPARE(mbox.open(), 0);
    QVERIFY(QFile(fileName() + ".lock").exists());
    mbox.close();
    QVERIFY(!QFile(fileName() + ".lock").exists());
  } else {
    QVERIFY(!QFile(fileName() + ".lock").exists());
    QVERIFY(mbox.open() != 0);
    QVERIFY(QFile(fileName() + ".lock").exists());
    mbox.close();
    QVERIFY(!QFile(fileName() + ".lock").exists());
  }
  */
}

void MboxTest::cleanupTestCase()
{
  mTempDir->unlink();
}
