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

#ifndef MESSAGEQUEUEJOBTEST_H
#define MESSAGEQUEUEJOBTEST_H

#include <QtCore/QObject>

#include <Akonadi/Collection>


/**
  This tests the ability to queue messages (MessageQueueJob class).
  Note that the actual sending of messages is the MDA's job, and is not tested
  here.
 */
class MessageQueueJobTest : public QObject
{
  Q_OBJECT

  private Q_SLOTS:
    void initTestCase();
    void testAddressesFromMime();
    void testValidMessages();
    void testInvalidMessages();

  private:
    void verifyOutboxContents( qlonglong count );

    Akonadi::Collection outbox;

};


#endif
