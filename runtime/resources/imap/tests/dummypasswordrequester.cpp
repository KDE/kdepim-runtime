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

#include "dummypasswordrequester.h"

#include <qtest_kde.h>

#include <QtCore/QTimer>

DummyPasswordRequester::DummyPasswordRequester( QObject *parent)
  : PasswordRequesterInterface(parent)
{
  for ( int i=0; i<10; ++i ) {
    m_calls << StandardRequest;
    m_results << PasswordRetrieved;
  }
}

QString DummyPasswordRequester::password() const
{
  return m_password;
}

void DummyPasswordRequester::setPassword( const QString &password )
{
  m_password = password;
}

void DummyPasswordRequester::setScenario( const QList<RequestType> &expectedCalls,
                                          const QList<ResultType> &results )
{
  Q_ASSERT( expectedCalls.size() == results.size() );

  m_calls = expectedCalls;
  m_results = results;
}

void DummyPasswordRequester::requestPassword( RequestType request,
                                              const QString &/*serverError*/ )
{
  QVERIFY2( !m_calls.isEmpty(), QString("Got unexpected call: %1").arg( request ).toUtf8().constData() );
  QCOMPARE( m_calls.takeFirst(), request );

  QTimer::singleShot( 20, this, SLOT(emitResult()) );
}

void DummyPasswordRequester::emitResult()
{
  ResultType result = m_results.takeFirst();

  if ( result == PasswordRetrieved ) {
    emit done( result, m_password );
  } else {
    emit done( result );
  }
}

#include "dummypasswordrequester.moc"
