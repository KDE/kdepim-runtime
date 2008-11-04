/*
   Copyright (C) 2008 Omat Holding B.V. <info@omat.nl>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#ifndef IMAPLIBTEST_H
#define IMAPLIBTEST_H

#include <QtCore/QObject>
#include "../imaplib.h"
#include "fakeserver.h"

class ImaplibTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void testCapabilities();
    void testLogin();
    void testMailboxList();
    void testMailboxCreateAndDelete();
    void testGetHeaders();

private:
    FakeServer* m_fake;
    Imaplib* m_imap;
};

#endif
