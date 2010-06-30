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

#include "imaptestbase.h"

QString ImapTestBase::defaultPassword()
{
  return "foobar";
}

ImapAccount *ImapTestBase::createDefaultAccount()
{
  ImapAccount *account = new ImapAccount;

  account->setServer( "127.0.0.1" );
  account->setPort( 5989 );
  account->setUserName( "test@kdab.com" );
  account->setSubscriptionEnabled( true );
  account->setEncryptionMode( KIMAP::LoginJob::Unencrypted );
  account->setAuthenticationMode( KIMAP::LoginJob::ClearText );

  return account;
}

DummyPasswordRequester *ImapTestBase::createDefaultRequester()
{
  DummyPasswordRequester *requester = new DummyPasswordRequester( this );
  requester->setPassword( defaultPassword() );
  return requester;
}

void ImapTestBase::setupTestCase()
{
  qRegisterMetaType<ImapAccount*>();
  qRegisterMetaType<DummyPasswordRequester*>();
  qRegisterMetaType<DummyResourceState::Ptr>();
  qRegisterMetaType<KIMAP::Session*>();
}

QList<QByteArray> ImapTestBase::defaultAuthScenario()
{
  QList<QByteArray> scenario;

  scenario << FakeServer::greeting()
           << "C: A000001 LOGIN test@kdab.com foobar"
           << "S: A000001 OK User Logged in";

  return scenario;
}

QList<QByteArray> ImapTestBase::defaultPoolConnectionScenario()
{
  QList<QByteArray> scenario;

  scenario << defaultAuthScenario()
           << "C: A000002 CAPABILITY"
           << "S: * CAPABILITY IMAP4 IMAP4rev1 UIDPLUS IDLE"
           << "S: A000002 OK Completed";

  return scenario;
}

bool ImapTestBase::waitForSignal( QObject *obj, const char *member, int timeout )
{
  QEventLoop loop;
  QTimer timer;

  connect( &timer, SIGNAL(timeout()), &loop, SLOT(quit()) );

  QSignalSpy spy( obj, member );
  connect( obj, member, &loop, SLOT(quit()) );

  timer.setSingleShot( true );
  timer.start( timeout );
  loop.exec();
  timer.stop();

  return spy.count()==1;
}

#include "imaptestbase.moc"
