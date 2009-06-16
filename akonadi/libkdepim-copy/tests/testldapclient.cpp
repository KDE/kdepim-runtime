/* This file is part of the KDE project
   Copyright (C) 2005 David Faure <faure@kde.org>

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

#include "testldapclient.h"

#include <kapplication.h>
#include <kdebug.h>
#include <kcmdlineargs.h>

#include <QEventLoop>

#include <assert.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
  // KApplication::disableAutoDcopRegistration();
  KCmdLineArgs::init(argc,argv,"testldapclient", 0, KLocalizedString(), 0, KLocalizedString());
  KApplication app;

  TestLDAPClient test;
  test.setup();
  test.runAll();
  test.cleanup();
  kDebug() <<"All tests OK.";
  return 0;
}

void TestLDAPClient::setup()
{
}

void TestLDAPClient::runAll()
{
  testIntevation();
}

bool TestLDAPClient::check(const QString& txt, QString a, QString b)
{
  if (a.isEmpty())
    a.clear();
  if (b.isEmpty())
    b.clear();
  if (a == b) {
    kDebug() << txt <<" : checking '" << a <<"' against expected value '" << b <<"'..." <<"ok";
  }
  else {
    kDebug() << txt <<" : checking '" << a <<"' against expected value '" << b <<"'..." <<"KO !";
    cleanup();
    exit(1);
  }
  return true;
}

void TestLDAPClient::cleanup()
{
  mClient = 0;
}

void TestLDAPClient::testIntevation()
{
  kDebug() ;
  mClient = new LdapClient( 0, this );

#ifdef __GNUC__
#warning TODO!
#endif
#if 0
  mClient->setHost( "ca.intevation.de" );
  mClient->setPort( "389" );
  mClient->setBase( "o=Intevation GmbH,c=de" );
#endif

  // Same list as in kaddressbook's ldapsearchdialog
  QStringList attrs;
  attrs << "l" << "Company" << "co" << "department" << "description" << "mail" << "facsimileTelephoneNumber" << "cn" << "homePhone" << "mobile" << "o" << "pager" << "postalAddress" << "st" << "street" << "title" << "uid" << "telephoneNumber" << "postalCode" << "objectClass";
  // the list from ldapclient.cpp
  //attrs << "cn" << "mail" << "givenname" << "sn" << "objectClass";
  mClient->setAttrs( attrs );

  // Taken from LdapSearch
  //QString mSearchText = QString::fromUtf8( "Till" );
  //QString filter = QString( "&(|(objectclass=person)(objectclass=groupOfNames)(mail=*))(|(cn=%1*)(mail=%2*)(givenName=%3*)(sn=%4*))" )
  //                 .arg( mSearchText ).arg( mSearchText ).arg( mSearchText ).arg( mSearchText );

  // For some reason a fromUtf8 broke the search for me (no results).
  // But this certainly looks fishy, it might break on non-utf8 systems.
  QString filter = "&(|(objectclass=person)(objectclass=groupofnames)(mail=*))(|(cn=*Ägypten MDK*)(sn=*Ägypten MDK*))";

  connect( mClient, SIGNAL( result( const LdapClient&, const KLDAP::LdapObject& ) ),
           this, SLOT( slotLDAPResult( const LdapClient&, const KLDAP::LdapObject& ) ) );
  connect( mClient, SIGNAL( done() ),
           this, SLOT( slotLDAPDone() ) );
  connect( mClient, SIGNAL( error( const QString& ) ),
           this, SLOT( slotLDAPError( const QString& ) ) );
  mClient->startQuery( filter );

  QEventLoop eventLoop;
  connect(this, SIGNAL(leaveModality()),
          &eventLoop, SLOT(quit()));
  eventLoop.exec(QEventLoop::ExcludeUserInputEvents);

  delete mClient; mClient = 0;
}

// from kaddressbook... ugly though...
static QString asUtf8( const QByteArray &val )
{
  if ( val.isEmpty() )
    return QString();

  const char *data = val.data();

  //QString::fromUtf8() bug workaround
  if ( data[ val.size() - 1 ] == '\0' )
    return QString::fromUtf8( data, val.size() - 1 );
  else
    return QString::fromUtf8( data, val.size() );
}

static QString join( const KLDAP::LdapAttrValue& lst, const QString& sep )
{
  QString res;
  bool already = false;
  for ( KLDAP::LdapAttrValue::ConstIterator it = lst.begin(); it != lst.end(); ++it ) {
    if ( already )
      res += sep;
    already = true;
    res += asUtf8( *it );
  }
  return res;
}

void TestLDAPClient::slotLDAPResult( const LdapClient&, const KLDAP::LdapObject& obj )
{
  QString cn = join( obj.attributes()[ "cn" ], ", " );
  kDebug() <<" cn:" << cn;
  assert( !obj.attributes()[ "mail" ].isEmpty() );
  QString mail = join( obj.attributes()[ "mail" ], ", " );
  kDebug() <<" mail:" << mail;
  assert( mail.contains( '@' ) );
}

void TestLDAPClient::slotLDAPError( const QString& err )
{
  kDebug() << err;
  ::exit( 1 );
}

void TestLDAPClient::slotLDAPDone()
{
  kDebug() ;
  emit leaveModality();
}

#include "testldapclient.moc"
