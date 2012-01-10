/*
    Copyright (c) 2010 Grégory Oestreicher <greg@kamago.net>

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

#include "davcollectiondeletejob.h"

#include <kio/deletejob.h>
#include <klocale.h>

DavCollectionDeleteJob::DavCollectionDeleteJob(const DavUtils::DavUrl &url, QObject *parent )
  : KJob( parent ), mUrl( url )
{
}

void DavCollectionDeleteJob::start()
{
  KIO::DeleteJob *job = KIO::del( mUrl.url(), KIO::HideProgressInfo | KIO::DefaultFlags );
  job->addMetaData( "PropagateHttpHeader", "true" );
  job->addMetaData( "cookies", "none" );
  job->addMetaData( "no-auth-prompt", "true" );

  connect( job, SIGNAL(result(KJob*)), this, SLOT(davJobFinished(KJob*)) );
}

void DavCollectionDeleteJob::davJobFinished( KJob *job )
{
  KIO::DeleteJob *deleteJob = qobject_cast<KIO::DeleteJob*>( job );

  if ( deleteJob->error() && deleteJob->error() != KIO::ERR_NO_CONTENT ) {
    if ( deleteJob->queryMetaData( "responsecode" ).isEmpty() ) {
      setError( deleteJob->error() );
      setErrorText( deleteJob->errorText() );
    }
    else {
      const int responseCode = deleteJob->queryMetaData( "responsecode" ).toInt();
      setError( UserDefinedError + responseCode );
      setErrorText( i18n( "There was a problem with the request. The collection has not been deleted from the server.\n"
                          "%1 (%2).", deleteJob->errorString(), responseCode ) );
    }
  }

  emitResult();
}

#include "davcollectiondeletejob.moc"
