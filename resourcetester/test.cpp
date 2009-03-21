/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

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

#include "test.h"
#include "resource.h"

#include <KDebug>
#include <stdlib.h>

Test::Test(QObject* parent) :
  QObject( parent )
{
}

void Test::assert(bool value)
{
  if ( !value )
    fail( "Assertion failed." );
}

void Test::verify( QObject* object, const QString &slot )
{
  kDebug() << object << slot;
  bool result = false;
  if ( !QMetaObject::invokeMethod( object, slot.toLatin1(), Q_RETURN_ARG( bool, result ) ) )
    fail( "Unable to call method " + slot );

  if ( result )
    return;

  QString lastError = QString( "Call to method " + slot + " returned false." );
  QMetaObject::invokeMethod( object, "lastError", Q_RETURN_ARG( QString, lastError ) );
  fail( lastError );
}

void Test::fail(const QString& error)
{
  kError() << error;
  abort();
}

void Test::abort()
{
  if ( Resource::instance() )
    Resource::instance()->destroy();
  exit( -1 );
}

#include "test.moc"
