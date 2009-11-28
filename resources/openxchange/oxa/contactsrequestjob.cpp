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

#include "contactsrequestjob.h"

#include "davmanager.h"
#include "davutils.h"
#include "oxutils.h"

#include <kio/davjob.h>

#include <QtCore/QDebug>

using namespace OXA;

ContactsRequestJob::ContactsRequestJob( qlonglong folderId, QObject *parent )
  : KJob( parent ), mFolderId( folderId )
{
}

void ContactsRequestJob::start()
{
  QDomDocument document;
  QDomElement multistatus = DAVUtils::addDavElement( document, document, QLatin1String( "multistatus" ) );
  QDomElement prop = DAVUtils::addDavElement( document, multistatus, QLatin1String( "prop" ) );

  DAVUtils::addOxElement( document, prop, QLatin1String( "lastsync" ), OXUtils::writeNumber( 0 ) );
  DAVUtils::addOxElement( document, prop, QLatin1String( "objectmode" ), OXUtils::writeString( "NEW_AND_MODIFIED" ) );
  DAVUtils::addOxElement( document, prop, QLatin1String( "folder_id" ), OXUtils::writeNumber( mFolderId ) );

  const QString path = QLatin1String( "/servlet/webdav.contacts" );

  KIO::DavJob *job = DavManager::self()->createFindJob( path, document );
  connect( job, SIGNAL( result( KJob* ) ), SLOT( davJobFinished( KJob* ) ) );
}

void ContactsRequestJob::davJobFinished( KJob *job )
{
  if ( job->error() ) {
    setError( job->error() );
    setErrorText( job->errorText() );
    emitResult();
    qDebug("error occured");
    return;
  }

  KIO::DavJob *davJob = qobject_cast<KIO::DavJob*>( job );

  const QDomDocument &document = davJob->response();
  qDebug() << document.toString();

  emitResult();
}

#include "contactsrequestjob.moc"
