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
  : DavJobBase( parent ), mUrl( url ), mItem( item )
{
}

void DavItemDeleteJob::start()
{
  KIO::DeleteJob *job = KIO::del( mUrl.url(), KIO::HideProgressInfo | KIO::DefaultFlags );
  job->addMetaData( QLatin1String("PropagateHttpHeader"), QLatin1String("true") );
  job->addMetaData( QLatin1String("customHTTPHeader"), QLatin1String("If-Match: ") + mItem.etag() );
  job->addMetaData( QLatin1String("cookies"), QLatin1String("none") );
  job->addMetaData( QLatin1String("no-auth-prompt"), QLatin1String("true") );

  connect( job, SIGNAL(result(KJob*)), this, SLOT(davJobFinished(KJob*)) );
}

void DavItemDeleteJob::davJobFinished( KJob *job )
{
  KIO::DeleteJob *deleteJob = qobject_cast<KIO::DeleteJob*>( job );

  if ( deleteJob->error() && deleteJob->error() != KIO::ERR_NO_CONTENT ) {
    const int responseCode = deleteJob->queryMetaData( QLatin1String("responsecode") ).isEmpty() ?
                              0 :
                              deleteJob->queryMetaData( QLatin1String("responsecode") ).toInt();

    if ( responseCode != 404 && responseCode != 410 ) {
      QString err;
      if ( deleteJob->error() != KIO::ERR_SLAVE_DEFINED )
        err = KIO::buildErrorString( deleteJob->error(), deleteJob->errorText() );
      else
        err = deleteJob->errorText();

      setLatestResponseCode( responseCode );
      setError( UserDefinedError + responseCode );
      setErrorText( i18n( "There was a problem with the request. The item has not been deleted from the server.\n"
                          "%1 (%2).", err, responseCode ) );
    }
  }

  emitResult();
}

