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

DavItemDeleteJob::DavItemDeleteJob( const DavUtils::DavUrl &url, const DavItem &item, QObject *parent )
  : KJob( parent ), mUrl( url ), mItem( item )
{
}

void DavItemDeleteJob::start()
{
  KIO::DeleteJob *job = KIO::del( mUrl.url(), KIO::HideProgressInfo | KIO::DefaultFlags );
  job->addMetaData( "PropagateHttpHeader", "true" );
  job->addMetaData( "customHTTPHeader", "If-Match: " + mItem.etag() );

  connect( job, SIGNAL( result( KJob* ) ), this, SLOT( davJobFinished( KJob* ) ) );
}

void DavItemDeleteJob::davJobFinished( KJob *job )
{
  if ( job->error() ) {
    setError( job->error() );
    setErrorText( job->errorText() );
  }

  KIO::DeleteJob *deleteJob = qobject_cast<KIO::DeleteJob*>( job );
  const QString httpStatus = deleteJob->queryMetaData( "HTTP-Headers" ).split( "\n" ).at( 0 );

  if ( httpStatus.contains( "HTTP/1.1 5" ) ) {
    // Server-side error, unrecoverable
    setError( 1 );
    setErrorText( httpStatus );
  }

  emitResult();
}

#include "davitemdeletejob.moc"
