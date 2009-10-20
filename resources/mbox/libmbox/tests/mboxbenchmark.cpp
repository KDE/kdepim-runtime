/*
  Copyright (c) 2009 Bertjan Broeksema <b.broeksema@kdemail.net>

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

#include "mboxbenchmark.h"
#include "mboxbenchmark.moc"

#include <qtest_kde.h>
#include <kstandarddirs.h>
#include <ktempdir.h>

QTEST_KDEMAIN_CORE(MBoxBenchmark)

#include "test-entries.h"

static const char * testDir = "libmbox-unit-test";
static const char * testFile = "test-mbox-file";

QString MBoxBenchmark::fileName()
{
  return mTempDir->name() + testFile;
}

void MBoxBenchmark::initTestCase()
{
  mTempDir = new KTempDir( KStandardDirs::locateLocal("tmp", testDir ) );
  mMail1 = MessagePtr( new KMime::Message );
  mMail1->setContent( KMime::CRLFtoLF( sEntry1 ) );
  mMail1->parse();
}

void MBoxBenchmark::cleanupTestCase()
{
  mTempDir->unlink();
}

void MBoxBenchmark::testNoLockPerformance()
{
  MBox mbox;
  mbox.setLockType(MBox::None);
  mbox.load(fileName());

  for (int i = 0; i < 1000; ++i)
    mbox.appendEntry(mMail1);

  mbox.save(fileName());


  QBENCHMARK {
    MBox mbox2;
    mbox2.setLockType(MBox::None);
    mbox2.setUnlockTimeout(5000);
    mbox2.load(fileName());
    foreach (MsgInfo const &info, mbox2.entryList()) {
      mbox2.readEntry(info.first);
    }
  }
}

void MBoxBenchmark::testProcfileLockPerformance()
{
  mMail1 = MessagePtr( new KMime::Message );
  mMail1->setContent( KMime::CRLFtoLF( sEntry1 ) );
  mMail1->parse();

  MBox mbox;
  mbox.setLockType(MBox::ProcmailLockfile);
  mbox.load(fileName());
  for (int i = 0; i < 1000; ++i)
    mbox.appendEntry(mMail1);

  mbox.save(fileName());

  QBENCHMARK {
    MBox mbox2;
    mbox2.setLockType(MBox::ProcmailLockfile);
    mbox2.load(fileName());
    mbox2.setUnlockTimeout(5000); // Keep the mbox locked for five seconds.

    foreach (MsgInfo const &info, mbox2.entryList())
      mbox2.readEntry(info.first);
  }
}
