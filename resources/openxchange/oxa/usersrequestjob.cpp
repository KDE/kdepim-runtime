/*
    This file is part of oxaccess.

    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

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

#include "usersrequestjob.h"

#include "davmanager.h"
#include "davutils.h"
#include "oxutils.h"

#include <kio/davjob.h>

using namespace OXA;

UsersRequestJob::UsersRequestJob( QObject *parent )
  : KJob( parent )
{
}

void UsersRequestJob::start()
{
  QDomDocument document;
  QDomElement multistatus = DAVUtils::addDavElement( document, document, QLatin1String( "multistatus" ) );
  QDomElement prop = DAVUtils::addDavElement( document, multistatus, QLatin1String( "prop" ) );
  DAVUtils::addOxElement( document, prop, QLatin1String( "user" ), QLatin1String( "*" ) );

  const QString path = QLatin1String( "/servlet/webdav.groupuser" );

  KIO::DavJob *job = DavManager::self()->createFindJob( path, document );
  connect( job, SIGNAL( result( KJob* ) ), SLOT( davJobFinished( KJob* ) ) );

  job->start();
}

User::List UsersRequestJob::users() const
{
  return mUsers;
}

void UsersRequestJob::davJobFinished( KJob *job )
{
  if ( job->error() ) {
    setError( job->error() );
    setErrorText( job->errorText() );
    emitResult();
    return;
  }

  KIO::DavJob *davJob = qobject_cast<KIO::DavJob*>( job );

  const QDomDocument document = davJob->response();

  QDomElement multistatus = document.documentElement();
  QDomElement response = multistatus.firstChildElement( QLatin1String( "response" ) );
  QDomElement propstat = response.firstChildElement( QLatin1String( "propstat" ) );
  QDomElement prop = propstat.firstChildElement( QLatin1String( "prop" ) );
  QDomElement users = prop.firstChildElement( QLatin1String( "users" ) );

  QDomElement userElement = users.firstChildElement( QLatin1String( "user" ) );
  while ( !userElement.isNull() ) {
    User user;

    QDomElement element = userElement.firstChildElement();
    while ( !element.isNull() ) {
      if ( element.tagName() == QLatin1String( "uid" ) ) {
        user.setUid( OXUtils::readNumber( element.text() ) );
      } else if ( element.tagName() == QLatin1String( "email1" ) ) {
        user.setEmail( OXUtils::readString( element.text() ) );
      } else if ( element.tagName() == QLatin1String( "displayname" ) ) {
        user.setName( OXUtils::readString( element.text() ) );
      }

      element = element.nextSiblingElement();
    }

    mUsers.append( user );

    userElement = userElement.nextSiblingElement( QLatin1String( "user" ) );
  }

  emitResult();
}

#include "usersrequestjob.moc"
