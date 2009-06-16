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
    m_fake->setResponse( list );
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

void ImaplibTest::testMailboxList()
{
    QSignalSpy spy1( m_imap, SIGNAL( currentFolders( const QStringList&, const QStringList& ) ) );

    QStringList list;
    list << "* LIST (\\HasChildren) \".\" \"TestCases\"\n" // simple test
            "* LIST (\\HasNoChildren) \".\" \"TestCases.Cl&AOE-udio\"\n" // encoding test
            "* LIST (\\HasNoChildren) \".\" \"TestCases.Frode R&APg-nning\"\n" // encoding test
            "* LIST (\\HasNoChildren \\Noselect) \".\" \"TestCases.a02 in body\"\n" // a02 & noselect 
            "* LIST (\\HasNoChildren) \".\" \"TestCases.tom &- jerry\"\n" // & encoding
            "* LIST (\\HasNoChildren) \".\" TestCases.tom\n" // no quotes around the bane
            "a02 OK Completed";
    m_fake->setResponse( list );
    m_imap->getMailBoxList();

    QEventLoop loop;
    connect( m_imap, SIGNAL( currentFolders( const QStringList&, const QStringList& ) ), &loop, SLOT( quit() ) );
    loop.exec();

    QCOMPARE( spy1.count(), 1 );

    QList<QVariant> arguments = spy1.takeFirst();
    QStringList folders = arguments.at(0).toStringList();
    QStringList noselectfolders = arguments.at(1).toStringList();

    QCOMPARE( folders.count(), 6);
    QCOMPARE( noselectfolders.count(), 1);

    QStringList list1;
    list1 << "TestCases" << QString::fromUtf8("TestCases.Cláudio") << QString::fromUtf8("TestCases.Frode Rønning")
         << "TestCases.a02 in body" << "TestCases.tom & jerry" << "TestCases.tom";
    
    QCOMPARE( folders, list1 );

    list1.clear();
    list1 << "TestCases.a02 in body";

    QCOMPARE( noselectfolders, list1 );
}

void ImaplibTest::testMailboxCreateAndDelete()
{
    QSignalSpy spy1( m_imap, SIGNAL( mailBoxAdded( bool ) ) );
    QSignalSpy spy2( m_imap, SIGNAL( mailBoxDeleted( bool, QString ) ) );

    QStringList list;
    list << "a02 OK" << "a02 NO" << "a02 OK" << "a02 BAD";
    m_fake->setResponse( list );
    m_imap->createMailBox( "tom" );

    QEventLoop loop;
    connect( m_imap, SIGNAL( mailBoxAdded( bool ) ), &loop, SLOT( quit() ) );
    connect( m_imap, SIGNAL( mailBoxDeleted( bool, QString ) ), &loop, SLOT( quit() ) );
    loop.exec();

    QCOMPARE( spy1.count(), 1 );
    QCOMPARE( spy2.count(), 0 );
    QCOMPARE( spy1.takeFirst().at(0).toBool(), true );

    m_imap->createMailBox( "tom" );
    loop.exec();

    QCOMPARE( spy1.count(), 1 );
    QCOMPARE( spy2.count(), 0 );
    QCOMPARE( spy1.takeFirst().at(0).toBool(), false );

    m_imap->deleteMailBox( "tom &" );
    loop.exec();

    QCOMPARE( spy1.count(), 0 );
    QCOMPARE( spy2.count(), 1 );
    QCOMPARE( spy2.at(0).at(0).toBool(), true );
    QCOMPARE( spy2.takeFirst().at(1).toString(), QString("tom &") );

    m_imap->deleteMailBox( QString::fromUtf8("tøm") );
    loop.exec();

    QCOMPARE( spy1.count(), 0 );
    QCOMPARE( spy2.count(), 1 );
    QCOMPARE( spy2.at(0).at(0).toBool(), false );
    QCOMPARE( spy2.takeFirst().at(1).toString(), QString::fromUtf8("tøm") );
}

void ImaplibTest::testGetHeaders()
{
    QSignalSpy spy1( m_imap, SIGNAL( headersInFolder( Imaplib*, const QString&, const QStringList& ) ) );

    QStringList list;
    list << "a02 OK" 
         << "* 1 FETCH (UID 1 RFC822.SIZE 5083 "
                "BODY[HEADER.FIELDS (\"FROM\" \"TO\" \"CC\" \"SUBJECT\" \"DATE\" \"IN-REPLY-TO\" \"MESSAGE-ID\")] {185}\n"
            "Message-Id: <E2HJ2E2-22222c-B2@somat.nl>\n"
            "From: \"Tom Albers\" <noreply@omat.nl>\n"
            "Subject: [tracker] [ Bugs-1660 ] Nice test case?\n"
            "To: noreply@omat.nl\n"
            "Date: Mon, 19 Feb 2007 02:58:45 -0800\n"
            "* 2 FETCH (UID 2 RFC822.SIZE 64100 " 
                "BODY[HEADER.FIELDS (\"FROM\" \"TO\" \"CC\" \"SUBJECT\" \"DATE\" \"IN-REPLY-TO\" \"MESSAGE-ID\")] {142}\n"
            "Message-Id: <E2HJ2Kj-2222P2-VI@omat.nl>\n"
            "From: info@omat.nl\n"
            "Subject: [patch] Not really!\n"
            "To: info@omat.nl\n"
            "Date: Mon, 19 Feb 2007 03:05:25 -0800\r\n"
            "a02 OK FETCH completed.";
    m_fake->setResponse( list );

    list.clear();
    list << QString::number(0) << QString::number(1);
    m_imap->getHeaders( "Tom", list ); 

    QEventLoop loop;
    connect( m_imap, SIGNAL( headersInFolder( Imaplib*, const QString&, const QStringList& ) ), 
             &loop, SLOT( quit() ) );
    loop.exec();
    
    list.clear();
    list << "1" << "Tom" 
         << "Message-Id: <E2HJ2E2-22222c-B2@somat.nl>\n"
            "From: \"Tom Albers\" <noreply@omat.nl>\n"
            "Subject: [tracker] [ Bugs-1660 ] Nice test case?\n"
            "To: noreply@omat.nl\n"
            "Date: Mon, 19 Feb 2007 02:58:45 -0800" << "5083"
         << "2" << "Tom" 
         << "Message-Id: <E2HJ2Kj-2222P2-VI@omat.nl>\n"
            "From: info@omat.nl\n"
            "Subject: [patch] Not really!\n"
            "To: info@omat.nl\n"
            "Date: Mon, 19 Feb 2007 03:05:25 -0800" << "64100";


    QCOMPARE( spy1.count(), 1 );
    QCOMPARE( spy1.at(0).at(1).toString(), QString( "Tom" ) );
    QCOMPARE( spy1.takeFirst().at(2).toStringList(), list );
}


