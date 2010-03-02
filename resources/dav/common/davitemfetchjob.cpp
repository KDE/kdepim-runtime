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

#include "davitemfetchjob.h"

#include "davmanager.h"

#include <kio/davjob.h>
#include <kio/job.h>
#include <klocale.h>

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


DavItemFetchJob::DavItemFetchJob( const DavUtils::DavUrl &url, const DavItem &item, QObject *parent )
  : KJob( parent ), mUrl( url ), mItem( item )
{
}

void DavItemFetchJob::start()
{
  KIO::StoredTransferJob *job = KIO::storedGet( mUrl.url(), KIO::Reload, KIO::HideProgressInfo | KIO::DefaultFlags );
  job->addMetaData( "PropagateHttpHeader", "true" );
  // Work around a strange bug in Zimbra (seen at least on CE 5.0.18) : if the user-agent
  // contains "Mozilla", some strange debug data is displayed in the shared calendars.
  // This kinda mess up the events parsing...
  job->addMetaData( "UserAgent", "KDE DAV groupware client" );

  connect( job, SIGNAL( result( KJob* ) ), this, SLOT( davJobFinished( KJob* ) ) );
}

DavItem DavItemFetchJob::item() const
{
  return mItem;
}

void DavItemFetchJob::davJobFinished( KJob *job )
{
  if ( job->error() ) {
    setError( job->error() );
    setErrorText( job->errorText() );
    emitResult();
    return;
  }

  KIO::StoredTransferJob *storedJob = qobject_cast<KIO::StoredTransferJob*>( job );
  const QString httpStatus = storedJob->queryMetaData( "HTTP-Headers" ).split( "\n" ).at( 0 );

  if ( httpStatus.contains( "HTTP/1.1 5" ) ) {
    // Server-side error, unrecoverable
    setError( 1 );
    setErrorText( httpStatus );
    emitResult();
    return;
  }

  mItem.setData( storedJob->data() );
  mItem.setContentType( storedJob->queryMetaData( "content-type" ) );
  mItem.setEtag( etagFromHeaders( storedJob->queryMetaData( "HTTP-Headers" ) ) );

  emitResult();
}

#include "davitemfetchjob.moc"
