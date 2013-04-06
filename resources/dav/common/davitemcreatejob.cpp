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

#include "davitemcreatejob.h"

#include "davitemfetchjob.h"
#include "davmanager.h"

#include <kio/davjob.h>
#include <kio/job.h>
#include <klocale.h>

#include <QtCore/QDebug>

DavItemCreateJob::DavItemCreateJob( const DavUtils::DavUrl &url, const DavItem &item, QObject *parent )
  : KJob( parent ), mUrl( url ), mItem( item )
{
}

void DavItemCreateJob::start()
{
  QString headers = "Content-Type: ";
  headers += mItem.contentType();
  headers += "\r\n";
  headers += "If-None-Match: *";

  KIO::StoredTransferJob *job = KIO::storedPut( mItem.data(), mUrl.url(), -1, KIO::HideProgressInfo | KIO::DefaultFlags );
  job->addMetaData( "PropagateHttpHeader", "true" );
  job->addMetaData( "customHTTPHeader", headers );
  job->addMetaData( "cookies", "none" );
  job->addMetaData( "no-auth-prompt", "true" );

  connect( job, SIGNAL(result(KJob*)), this, SLOT(davJobFinished(KJob*)) );
}

DavItem DavItemCreateJob::item() const
{
  return mItem;
}

void DavItemCreateJob::davJobFinished( KJob *job )
{
  KIO::StoredTransferJob *storedJob = qobject_cast<KIO::StoredTransferJob*>( job );

  if ( storedJob->error() ) {
    const int responseCode = storedJob->queryMetaData( "responsecode" ).isEmpty() ?
                              0 :
                              storedJob->queryMetaData( "responsecode" ).toInt();

    QString err;
    if ( storedJob->error() != KIO::ERR_SLAVE_DEFINED )
      err = KIO::buildErrorString( storedJob->error(), storedJob->errorText() );
    else
      err = storedJob->errorText();

    setError( UserDefinedError + responseCode );
    setErrorText( i18n( "There was a problem with the request. The item has not been created on the server.\n"
                        "%1 (%2).", err, responseCode ) );

    emitResult();
    return;
  }

  // The 'Location:' HTTP header is used to indicate the new URL
  const QStringList allHeaders = storedJob->queryMetaData( "HTTP-Headers" ).split( '\n' );
  QString location;
  foreach ( const QString &header, allHeaders ) {
    if ( header.startsWith( QLatin1String( "location:" ), Qt::CaseInsensitive ) )
      location = header.section( ' ', 1 );
  }

  KUrl url;
  if ( location.isEmpty() )
    url = storedJob->url();
  else if ( location.startsWith( '/' ) ) {
    url = storedJob->url();
    url.setPath( location );
  } else
    url = location;

  url.setUser( QString() );
  mItem.setUrl( url.prettyUrl() );

  DavItemFetchJob *fetchJob = new DavItemFetchJob( mUrl, mItem );
  connect( fetchJob, SIGNAL(result(KJob*)), this, SLOT(itemRefreshed(KJob*)) );
  fetchJob->start();
}

void DavItemCreateJob::itemRefreshed( KJob *job )
{
  if ( !job->error() ) {
    DavItemFetchJob *fetchJob = qobject_cast<DavItemFetchJob*>( job );
    mItem.setEtag( fetchJob->item().etag() );
  }
  emitResult();
}

#include "davitemcreatejob.moc"
