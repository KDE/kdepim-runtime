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

  QFile mboxfile(fileName());
  mboxfile.open(QFile::ReadWrite);

  // Put some testdata in the file.
  QTextStream out(&mboxfile);
  out << "From: me@me.me" << endl;

  mboxfile.close();
  QVERIFY(mboxfile.exists());
}

void MboxTest::testIsValid()
{
  MBox mbox1(fileName(), true); // ReadOnly
  QVERIFY(mbox1.isValid());     // FCNTL is the default lock method.

  if (!KStandardDirs::findExe("lockfile").isEmpty()) {
    mbox1.setLockType(MBox::ProcmailLockfile);
    QVERIFY(mbox1.isValid());
  } else {
    mbox1.setLockType(MBox::ProcmailLockfile);
    QVERIFY(!mbox1.isValid());
  }

  if (!KStandardDirs::findExe("mutt_dotlock").isEmpty()) {
    mbox1.setLockType(MBox::MuttDotlock);
    QVERIFY(mbox1.isValid());
    mbox1.setLockType(MBox::MuttDotlockPrivileged);
    QVERIFY(mbox1.isValid());
  } else {
    mbox1.setLockType(MBox::MuttDotlock);
    QVERIFY(!mbox1.isValid());
    mbox1.setLockType(MBox::MuttDotlockPrivileged);
    QVERIFY(!mbox1.isValid());
  }

  mbox1.setLockType(MBox::None);
  QVERIFY(mbox1.isValid());

  MBox mbox2(fileName(), false);
  QVERIFY(mbox2.isValid());

  MBox mbox3("2_Non-ExistingFile", true);
  QVERIFY(!mbox3.isValid());

  MBox mbox4("2_Non-ExistingFile", false);
  QVERIFY(!mbox4.isValid());
}

void MboxTest::testProcMailLock()
{
  // It really only makes sense to test this if the lockfile executable can be
  // found.
  MBox mbox(fileName(), true);
  mbox.setLockType(MBox::ProcmailLockfile);
  if (!KStandardDirs::findExe("lockfile").isEmpty()) {
    QVERIFY(!QFile(fileName() + ".lock").exists());
    QCOMPARE(mbox.open(), 0);
    QVERIFY(QFile(fileName() + ".lock").exists());
    mbox.close();
    QVERIFY(!QFile(fileName() + ".lock").exists());
  } else {
    QVERIFY(!QFile(fileName() + ".lock").exists());
    QVERIFY(mbox.open() != 0);
    QEXPECT_FAIL("", "This only works when procmail is installed.", Continue);
    QVERIFY(QFile(fileName() + ".lock").exists());
    mbox.close();
    QVERIFY(!QFile(fileName() + ".lock").exists());
  }
}

void MboxTest::cleanupTestCase()
{
  mTempDir->unlink();
}
