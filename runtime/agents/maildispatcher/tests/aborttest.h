/*
    Copyright 2009 Constantin Berzan <exit3219@gmail.com>

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

#ifndef ABORTTEST_H
#define ABORTTEST_H

#include <QtCore/QObject>

#include <Akonadi/Collection>

namespace Akonadi {
  class Monitor;
}

/**
  This attempts to send a large message, then aborts it, then tries to send
  it again and verify that it succeeds.
 */
class AbortTest : public QObject
{
  Q_OBJECT

  private Q_SLOTS:
    void initTestCase();
    void testAbort();
    void testAbortWhileIdle();

  private:
    int akoTid;
    int smtpTid;
    Akonadi::Collection outbox;
    Akonadi::Collection sink;
    Akonadi::Monitor *monitor;

};


#endif
