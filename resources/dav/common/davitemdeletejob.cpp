/*
    Copyright (c) 2010 Tobias Koenig <tokoe@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include "davitemdeletejob.h"

#include "davmanager.h"

#include <kio/deletejob.h>
#include <klocale.h>

DavItemDeleteJob::DavItemDeleteJob( const DavUtils::DavUrl &url, const DavItem &item, QObject *parent )
  : KJob( parent ), mUrl( url ), mItem( item )
{
}

void DavItemDeleteJob::start()
{
  KIO::DeleteJob *job = KIO::del( mUrl.url(), KIO::HideProgressInfo | KIO::DefaultFlags );
  job->addMetaData( "PropagateHttpHeader", "true" );
  job->addMetaData( "customHTTPHeader", "If-Match: " + mItem.etag() );
  job->addMetaData( "cookies", "none" );
  job->addMetaData( "no-auth-prompt", "true" );

  connect( job, SIGNAL(result(KJob*)), this, SLOT(davJobFinished(KJob*)) );
}

void DavItemDeleteJob::davJobFinished( KJob *job )
{
  if ( job->error() ) {
    setError( job->error() );
    setErrorText( job->errorText() );
  }

  KIO::DeleteJob *deleteJob = qobject_cast<KIO::DeleteJob*>( job );

  const int responseCode = deleteJob->queryMetaData( "responsecode" ).toInt();

  if ( responseCode > 499 && responseCode < 600 ) {
    // Server-side error, unrecoverable
    setError( UserDefinedError );
    setErrorText( i18n( "The server encountered an error that prevented it from completing your request" ) );
  } else if ( responseCode == 404 ) {
    // We don't mind getting a 404 error as the that's what we want in the end.
  } else if ( responseCode == 423 ) {
    // The remote resource has been locked
    setError( UserDefinedError );
    setErrorText( i18n( "The remote item has been locked, try again later" ) );
    emitResult();
    return;
  } else if ( responseCode > 399 && responseCode < 500 ) {
    // User-side error
    QString extraMessage;
    if ( responseCode == 401 )
      extraMessage = i18n( "Invalid username/password" );
    else if ( responseCode == 403 )
      extraMessage = i18n( "Acess forbidden" );
    else if ( responseCode == 404 )
      extraMessage = i18n( "Resource not found" );
    else
      extraMessage = i18n( "HTTP error" );

    setError( UserDefinedError );
    setErrorText( i18n( "There was a problem with the request. The item has not been deleted from the server.\n"
                        "%1 (%2).", extraMessage, responseCode ) );
    emitResult();
    return;
  }

  emitResult();
}

#include "davitemdeletejob.moc"
