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

#include "davitemmodifyjob.h"

#include "davmanager.h"

#include <kio/job.h>
#include <klocale.h>

static QString etagFromHeaders( const QString &headers )
{
  const QStringList allHeaders = headers.split( '\n' );

  QString etag;
  foreach ( const QString &header, allHeaders ) {
    if ( header.startsWith( QLatin1String( "etag:" ), Qt::CaseInsensitive ) )
      etag = header.section( ' ', 1 );
  }

  return etag;
}


DavItemModifyJob::DavItemModifyJob( const DavUtils::DavUrl &url, const DavItem &item, QObject *parent )
  : KJob( parent ), mUrl( url ), mItem( item )
{
}

void DavItemModifyJob::start()
{
  QString headers = "Content-Type: ";
  headers += mItem.contentType();
  headers += "\r\n";
  headers += "If-Match: " + mItem.etag();

  KIO::StoredTransferJob *job = KIO::storedPut( mItem.data(), mUrl.url(), -1, KIO::HideProgressInfo | KIO::DefaultFlags );
  job->addMetaData( "PropagateHttpHeader", "true" );
  job->addMetaData( "customHTTPHeader", headers );
  job->addMetaData( "cookies", "none" );
  job->addMetaData( "no-auth-prompt", "true" );

  connect( job, SIGNAL(result(KJob*)), this, SLOT(davJobFinished(KJob*)) );
}

DavItem DavItemModifyJob::item() const
{
  return mItem;
}

void DavItemModifyJob::davJobFinished( KJob *job )
{
  if ( job->error() ) {
    setError( job->error() );
    setErrorText( job->errorText() );
    emitResult();
    return;
  }

  KIO::StoredTransferJob *storedJob = qobject_cast<KIO::StoredTransferJob*>( job );

  const int responseCode = storedJob->queryMetaData( "responsecode" ).toInt();

  if ( responseCode > 499 && responseCode < 600 ) {
    // Server-side error, unrecoverable
    setError( UserDefinedError );
    setErrorText( i18n( "The server encountered an error that prevented it from completing your request" ) );
    emitResult();
    return;
  } else if ( responseCode == 423 ) {
    // The remote resource has been locked
    setError( UserDefinedError );
    setErrorText( i18n( "The remote item has been locked, try again later" ) );
    emitResult();
    return;
  } else if ( responseCode > 399 && responseCode < 500 ) {
    // User-side error
    setError( UserDefinedError );
    setErrorText( i18n( "There was a problem with the request - the item was not modified on the server. Error %1.", responseCode ) );
    emitResult();
    return;
  }

  // The 'Location:' HTTP header is used to indicate the new URL
  const QStringList allHeaders = storedJob->queryMetaData( "HTTP-Headers" ).split( '\n' );
  QString location;
  foreach ( const QString &header, allHeaders ) {
    if ( header.startsWith( QLatin1String( "location:" ), Qt::CaseInsensitive  ) )
      location = header.section( ' ', 1 );
  }

  KUrl url;
  if ( location.isEmpty() )
    url = storedJob->url();
  else
    url = location;

  url.setUser( QString() );
  mItem.setUrl( url.prettyUrl() );
  mItem.setEtag( etagFromHeaders( storedJob->queryMetaData( "HTTP-Headers" ) ) );

  emitResult();
}

#include "davitemmodifyjob.moc"
