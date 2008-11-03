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

#include <qtest_kde.h>

#include "imaplibtest.h"
#include "imaplibtest.moc"
#include "fakeserver.h"
#include "../imaplib.h"

#include <QTcpSocket>
#include <QtTest>

QTEST_KDEMAIN( ImaplibTest, NoGUI )

void ImaplibTest::initTestCase()
{
    qRegisterMetaType<Imaplib*>( "Imaplib*" );

    m_fake = new FakeServer( this );
    m_fake->setResponse( QStringList( "* CAPABILITY IMAP4 IMAP4rev1 ACL QUOTA LITERAL+ MAILBOX-REFERRALS NAMESPACE UIDPLUS ID NO_ATOMIC_RENAME UNSELECT CHILDREN MULTIAPPEND BINARY SORT THREAD=ORDEREDSUBJECT THREAD=REFERENCES ANNOTATEMORE IDLE STARTTLS" ) );
    m_fake->start();

    m_imap = new Imaplib( 0, "testconnection" );
    m_imap->startConnection( "127.0.0.1", 5989, 1 );

    QEventLoop loop;
    connect( m_imap, SIGNAL( login( Imaplib* ) ), &loop, SLOT( quit() ) );
    loop.exec();
}

void ImaplibTest::testCapabilities()
{
    QCOMPARE( m_imap->capable( "acl" ), true );
    QCOMPARE( m_imap->capable( "starttls" ), true );
    QCOMPARE( m_imap->capable( "uidnext" ), false );
    QCOMPARE( m_imap->capable( "uidplus" ), true );
    QCOMPARE( m_imap->capable( "CHILDREN" ), true );
}

void ImaplibTest::testLogin()
{
    QSignalSpy spy1( m_imap, SIGNAL( loginFailed( Imaplib* ) ) );
    QSignalSpy spy2( m_imap, SIGNAL( loginOk( Imaplib* ) ) );

    QStringList list;
    list << "a02 NO Login failed: authentication failure" << "a02 OK User logged in";
    m_fake->setResponse( QStringList( list ) );
    m_imap->login( "toma", "imaplib" );

    QEventLoop loop;
    connect( m_imap, SIGNAL( loginFailed( Imaplib* ) ), &loop, SLOT( quit() ) );
    connect( m_imap, SIGNAL( loginOk( Imaplib* ) ), &loop, SLOT( quit() ) );
    loop.exec();

    QCOMPARE( spy1.count(), 1 );
    QCOMPARE( spy2.count(), 0 );

    m_imap->login( "toma", "imaplib" );

    loop.exec();

    QCOMPARE( spy1.count(), 1 );
    QCOMPARE( spy2.count(), 1 );
}
