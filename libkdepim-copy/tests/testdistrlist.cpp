/* This file is part of the KDE project
   Copyright (C) 2004 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "testdistrlist.h"

#include <distributionlist.h>
using KPIM::DistributionList;

#include <config.h>

#include <kabc/stdaddressbook.h>
#include <kurl.h>
#include <kapplication.h>
#include <kio/netaccess.h>
#include <kio/job.h>
#include <kdebug.h>
#include <kcmdlineargs.h>

#include <qdir.h>
#include <qfileinfo.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

int main(int argc, char *argv[])
{
    // Use another directory than the real one, just to keep things clean
    // KDEHOME needs to be writable though, for a ksycoca database
    setenv( "KDEHOME", QFile::encodeName( QDir::homeDirPath() + "/.kde-testdistrlist" ), true );
    setenv( "KDE_FORK_SLAVES", "yes", true ); // simpler, for the final cleanup

    KApplication::disableAutoDcopRegistration();
    KCmdLineArgs::init(argc,argv,"testdistrlist", 0, 0, 0, 0);
    KApplication app;

    TestDistrList test;
    test.setup();
    test.runAll();
    test.cleanup();
    kdDebug() << "All tests OK." << endl;
    return 0;
}

void TestDistrList::setup()
{
    // We need a std addressbook
    KABC::AddressBook *ab = KABC::StdAddressBook::self();
    KABC::StdAddressBook::setAutomaticSave( false );

    // and two contacts
    KABC::Addressee addr1;
    addr1.setName( "addr1" );
    addr1.setFormattedName( "addr1" );
    addr1.insertEmail( "addr1@kde.org", true );
    addr1.insertEmail( "addr1-alternate@kde.org" );
    ab->insertAddressee( addr1 );
    assert( addr1.emails().count() == 2 );

    KABC::Addressee addr2;
    addr2.setName( "addr2" );
    addr2.insertEmail( "addr2@kde.org", true );
    addr2.insertEmail( "addr2-alternate@kde.org" );
    ab->insertAddressee( addr2 );
    assert( addr2.emails().count() == 2 );

    assert( !ab->findByName( "addr1" ).isEmpty() );
    assert( !ab->findByName( "addr2" ).isEmpty() );
}

void TestDistrList::runAll()
{
    testEmpty();
    testNewList();
    testInsertEntry();
    testRemoveEntry();
    testDuplicate();
    testDeleteList();
}

bool TestDistrList::check(const QString& txt, QString a, QString b)
{
    if (a.isEmpty())
        a = QString::null;
    if (b.isEmpty())
        b = QString::null;
    if (a == b) {
        kdDebug() << txt << " : checking '" << a << "' against expected value '" << b << "'... " << "ok" << endl;
    }
    else {
        kdDebug() << txt << " : checking '" << a << "' against expected value '" << b << "'... " << "KO !" << endl;
        cleanup();
        exit(1);
    }
    return true;
}

void TestDistrList::cleanup()
{
    kdDebug() << k_funcinfo << endl;
    KABC::AddressBook *ab = KABC::StdAddressBook::self();
    ab->clear();
    KABC::StdAddressBook::close();

    QString kdehome = QFile::decodeName( getenv("KDEHOME") );
    KURL urlkdehome; urlkdehome.setPath( kdehome );
    KIO::NetAccess::del( urlkdehome, 0 );
}

void TestDistrList::testEmpty()
{
    kdDebug() << k_funcinfo << endl;
    DistributionList dl;
    assert( dl.isEmpty() );
}

void TestDistrList::testNewList()
{
    kdDebug() << k_funcinfo << endl;
    DistributionList dl;
    dl.setName( "foo" );
    assert( !dl.isEmpty() );
    check( "name set", dl.formattedName(), "foo" );
    assert( DistributionList::isDistributionList( dl ) );

    KABC::AddressBook *ab = KABC::StdAddressBook::self();
    ab->insertAddressee( dl );
#if 0 // can't do that until we have KABC::AddressBook::findByFormattedName, or we use setName()
    KABC::Addressee::List addrList = ab->findByName( "foo" );
    assert( addrList.count() == 1 );
    KABC::Addressee addr = addrList.first();
    assert( !addr.isEmpty() );
    check( "correct name", addr.name(), "foo" );
    assert( DistributionList::isDistributionList( addr ) );
#else
    KABC::Addressee addr = dl;
#endif

    DistributionList dl2 = DistributionList::findByName( ab, "foo" );
    assert( !dl2.isEmpty() );
    check( "correct name", dl2.formattedName(), "foo" );
    assert( DistributionList::isDistributionList( dl2 ) );

    // Test the ctor that takes an addressee
    DistributionList dl3( addr );
    assert( !dl3.isEmpty() );
    assert( DistributionList::isDistributionList( dl3 ) );
    check( "correct name", dl3.formattedName(), "foo" );
}

void TestDistrList::testInsertEntry()
{
    kdDebug() << k_funcinfo << endl;
    KABC::AddressBook *ab = KABC::StdAddressBook::self();
    DistributionList dl = DistributionList::findByName( ab, "foo" );
    assert( !dl.isEmpty() );

#if 0 // the usual method
    KABC::Addressee addr1 = ab->findByName( "addr1" ).first();
    assert( !addr1.isEmpty() );
    dl.insertEntry( addr1 );
#else // the kolab-resource method
    dl.insertEntry( "addr1" );
#endif

    KABC::Addressee addr2 = ab->findByName( "addr2" ).first();
    assert( !addr2.isEmpty() );
    dl.insertEntry( addr2, "addr2-alternate@kde.org" );

    // Try inserting it again, should do nothing
    dl.insertEntry( addr2, "addr2-alternate@kde.org" );

    // And insert it with another email address
    dl.insertEntry( addr2, "addr2@kde.org" );

    // Test entries()
    DistributionList::Entry::List entries = dl.entries( ab );
    check( "entries count", QString::number( entries.count() ), "3" );
    check( "first entry", entries[0].addressee.name(), "addr1" );
    check( "first entry", entries[0].email, QString::null );
    check( "second entry", entries[1].addressee.name(), "addr2" );
    check( "second entry", entries[1].email, "addr2-alternate@kde.org" );
    check( "third entry", entries[2].addressee.name(), "addr2" );
    check( "third entry", entries[2].email, "addr2@kde.org" );

    // Test emails()
    QStringList emails = dl.emails( ab );
    kdDebug() << emails << endl;
    assert( emails.count() == 3 );
    check( "first email", emails[0], "addr1 <addr1@kde.org>" );
    check( "second email", emails[1], "addr2 <addr2-alternate@kde.org>" );
    check( "third email", emails[2], "addr2 <addr2@kde.org>" );

    // Commit changes to the addressbook !!
    ab->insertAddressee( dl );
}

void TestDistrList::testRemoveEntry()
{
    kdDebug() << k_funcinfo << endl;
    KABC::AddressBook *ab = KABC::StdAddressBook::self();
    DistributionList dl = DistributionList::findByName( ab, "foo" );
    assert( !dl.isEmpty() );
    DistributionList::Entry::List entries = dl.entries( ab );
    check( "entries count before removeEntry", QString::number( entries.count() ), "3" );

    // Removing an empty entry shouldn't do anything
    dl.removeEntry( KABC::Addressee() );
    check( "entries count after removing empty entry", QString::number( dl.entries(ab).count() ), "3" );

    KABC::Addressee addr1 = ab->findByName( "addr1" ).first();
    assert( !addr1.isEmpty() );
    // Removing an entry with the wrong email passed, shouldn't do anything
    dl.removeEntry( addr1, "foo@foobar.com" );
    check( "entries count after removing entry with invalid email", QString::number( dl.entries(ab).count() ), "3" );

    // Now remove entry correctly
    dl.removeEntry( addr1 );
    check( "removeEntry(addr1) worked", QString::number( dl.entries(ab).count() ), "2" );
    QStringList emails = dl.emails( ab );
    assert( emails.count() == 2 );
    check( "first email", emails[0], "addr2 <addr2-alternate@kde.org>" );

    // Now move on to addr2. First remove with no or a wrong email (nothing should happen)
    KABC::Addressee addr2 = ab->findByName( "addr2" ).first();
    assert( !addr2.isEmpty() );
    dl.removeEntry( addr2 );
    check( "entries count after removing entry with no email", QString::number( dl.entries(ab).count() ), "2" );

    // Now remove addr2 correctly
    dl.removeEntry( addr2, "addr2@kde.org" );
    check( "entries count after removing addr2", QString::number( dl.entries(ab).count() ), "1" );
    dl.removeEntry( addr2, "addr2-alternate@kde.org" );
    check( "entries count after removing alternate addr2", QString::number( dl.entries(ab).count() ), "0" );
    assert( dl.entries(ab).isEmpty() );
    assert( dl.emails(ab).isEmpty() );
    assert( DistributionList::isDistributionList( dl ) );

    ab->insertAddressee( dl );
}

void TestDistrList::testDuplicate()
{
    kdDebug() << k_funcinfo << endl;
    // This is a special test for the case where we have a contact and a distr list with the same name
    KABC::AddressBook *ab = KABC::StdAddressBook::self();
    KABC::Addressee addr;
    addr.setName( "foo" );
    addr.insertEmail( "foo@kde.org", true );
    ab->insertAddressee( addr );

#if 0 // we need a findByFormattedName
    KABC::Addressee::List addrList = ab->findByName( "foo" );
    assert( addrList.count() == 2 );

    bool a = DistributionList::isDistributionList( addrList.first() );
    bool b = DistributionList::isDistributionList( addrList.last() );
    // one is a distr list, but not both
    assert( a || b );
    //
    assert( ! ( a && b ) );
#endif

    DistributionList dl = DistributionList::findByName( ab, "foo" );
    assert( !dl.isEmpty() );
    assert( DistributionList::isDistributionList( dl ) );
    assert( dl.formattedName() == "foo" );
}

void TestDistrList::testDeleteList()
{
    kdDebug() << k_funcinfo << endl;

    KABC::AddressBook *ab = KABC::StdAddressBook::self();
    DistributionList dl = DistributionList::findByName( ab, "foo" );
    assert( !dl.isEmpty() );
    ab->removeAddressee( dl );
    dl = DistributionList::findByName( ab, "foo" );
    assert( dl.isEmpty() );
}

#include "testdistrlist.moc"
