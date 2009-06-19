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

#ifndef MBOXTEST_H
#define MBOXTEST_H

#include <QObject>

#include "../mbox.h"

class KTempDir;

class MboxTest : public QObject
{
  Q_OBJECT
  private Q_SLOTS:
    void initTestCase();
    void testSetLockMethod();
    void testLockBeforeLoad();
    void testProcMailLock();
    void testAppend();
    void testSaveAndLoad();
    void testBlankLines();
    void cleanupTestCase();
    void testEntries();

  private:
    QString fileName();
    QString lockFileName();
    void removeTestFile();

  private:
    KTempDir *mTempDir;
    MessagePtr mMail1;
    MessagePtr mMail2;
};

#endif // MBOXTEST_H
