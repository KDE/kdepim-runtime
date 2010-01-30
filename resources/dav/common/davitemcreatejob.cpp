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

#include "davmanager.h"

#include <kio/davjob.h>
#include <kio/job.h>

#include <QtCore/QDebug>

static QString etagFromHeaders( const QString &headers )
{
  const QStringList allHeaders = headers.split( "\n" );

  QString etag;
  foreach ( const QString &header, allHeaders ) {
    if ( header.startsWith( "etag:", Qt::CaseInsensitive ) )
      etag = header.section( ' ', 1 );
  }

  return etag;
}


DavItemCreateJob::DavItemCreateJob( const DavItem &item, QObject *parent )
  : KJob( parent ), mItem( item )
{
}

void DavItemCreateJob::start()
{
  KUrl url( mItem.url() );
  url.setUser( DavManager::self()->user() );
  url.setPassword( DavManager::self()->password() );

  QString headers = "Content-Type: ";
  headers += mItem.contentType();
  headers += "\r\n";
  headers += "If-None-Match: *";

  KIO::StoredTransferJob *job = KIO::storedPut( mItem.data(), url, -1, KIO::HideProgressInfo | KIO::DefaultFlags );
  job->addMetaData( "PropagateHttpHeader", "true" );
  job->addMetaData( "customHTTPHeader", headers );

  connect( job, SIGNAL( result( KJob* ) ), this, SLOT( davJobFinished( KJob* ) ) );
}

DavItem DavItemCreateJob::item() const
{
  return mItem;
}

void DavItemCreateJob::davJobFinished( KJob *job )
{
  if ( job->error() ) {
    setError( job->error() );
    setErrorText( job->errorText() );
    emitResult();
    return;
  }

  KIO::StoredTransferJob *storedJob = qobject_cast<KIO::StoredTransferJob*>( job );

  // The 'Location:' HTTP header is used to indicate the new URL
  const QStringList allHeaders = storedJob->queryMetaData( "HTTP-Headers" ).split( "\n" );

  QString location;
  foreach ( const QString &header, allHeaders ) {
    if ( header.startsWith( "Location:" ) )
      location = header.section( ' ', 1 );
  }

  KUrl url;
  if ( location.isEmpty() )
    url = storedJob->url();
  else
    url = location;

  mItem.setUrl( url.prettyUrl() );
  mItem.setEtag( etagFromHeaders( storedJob->queryMetaData( "HTTP-Headers" ) ) );

  emitResult();
}

#include "davitemcreatejob.moc"
