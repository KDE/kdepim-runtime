/*
   Copyright (c) 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
   Author: Kevin Ottens <kevin@kdab.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or ( at your option ) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#ifndef IMAPTESTBASE_H
#define IMAPTESTBASE_H

#include <qtest_kde.h>

#include <kimaptest/fakeserver.h>

#include "dummypasswordrequester.h"
#include "dummyresourcestate.h"
#include "imapaccount.h"
#include "resourcetask.h"
#include "sessionpool.h"

Q_DECLARE_METATYPE(ImapAccount*)
Q_DECLARE_METATYPE(DummyPasswordRequester*)
Q_DECLARE_METATYPE(DummyResourceState::Ptr)
Q_DECLARE_METATYPE(KIMAP::Session*)
Q_DECLARE_METATYPE(QVariant)

class ImapTestBase : public QObject
{
  Q_OBJECT

public:
  ImapTestBase( QObject *parent = 0 );

protected:
  QString defaultUserName() const;
  QString defaultPassword() const;
  ImapAccount *createDefaultAccount() const;
  DummyPasswordRequester *createDefaultRequester();
  QList<QByteArray> defaultAuthScenario() const;
  QList<QByteArray> defaultPoolConnectionScenario() const;

  bool waitForSignal( QObject *obj, const char *member, int timeout = 500 ) const;

private slots:
  void setupTestCase();
};

// Taken from Qt 5:
#if QT_VERSION < 0x050000

// Will try to wait for the expression to become true while allowing event processing
#define QTRY_VERIFY(__expr) \
do { \
    const int __step = 50; \
    const int __timeout = 5000; \
    if (!(__expr)) { \
        QTest::qWait(0); \
    } \
    for (int __i = 0; __i < __timeout && !(__expr); __i+=__step) { \
        QTest::qWait(__step); \
    } \
    QVERIFY(__expr); \
} while (0)

// Will try to wait for the comparison to become successful while allowing event processing
#define QTRY_COMPARE(__expr, __expected) \
do { \
    const int __step = 50; \
    const int __timeout = 5000; \
    if ((__expr) != (__expected)) { \
        QTest::qWait(0); \
    } \
    for (int __i = 0; __i < __timeout && ((__expr) != (__expected)); __i+=__step) { \
        QTest::qWait(__step); \
    } \
    QCOMPARE(__expr, __expected); \
} while (0)

#endif

#endif
