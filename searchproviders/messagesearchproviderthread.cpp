/*
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>

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

#include "messagesearchproviderthread.h"

#include <QtCore/QStringList>
#include <QtCore/QUuid>

using namespace Akonadi;

Akonadi::MessageSearchProviderThread::MessageSearchProviderThread(int ticket, QObject * parent) :
  SearchProviderThread( ticket, parent )
{
}

QStringList Akonadi::MessageSearchProviderThread::generateFetchResponse(const QList< int > uids, const QString & field)
{
  QStringList result;

  // ### FIXME
  const QByteArray date( "\"Wed, 1 Feb 2006 13:37:19 UT\"" );
  const QByteArray subject( "\"IMPORTANT: Akonadi Test\"" );
  const QByteArray from( "\"Tobias Koenig\" NIL \"tokoe\" \"kde.org\"" );
  const QByteArray sender = from;
  const QByteArray replyTo( "NIL" );
  const QByteArray to( "\"Ingo Kloecker\" NIL \"kloecker\" \"kde.org\"" );
  const QByteArray cc( "NIL" );
  const QByteArray bcc( "NIL" );
  const QByteArray inReplyTo( "NIL" );
  const QByteArray messageId( '<' + QUuid::createUuid().toString().toLatin1() + "@server.kde.org>" );

  QByteArray response( '('+date+' '+subject+" (("+from+")) (("+sender+")) "+replyTo+" (("+to+")) "+cc+' '+bcc+' '+inReplyTo+' '+messageId+')' );

  for ( int i = 0; i < uids.count(); ++i )
    result << QString::fromUtf8(response);
  return result;
}

