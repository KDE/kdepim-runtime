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

#include "davcollectionsmultifetchjob.h"

#include "davcollectionsfetchjob.h"

DavCollectionsMultiFetchJob::DavCollectionsMultiFetchJob( const DavUtils::DavUrl::List &urls, QObject *parent )
  : KJob( parent ), mUrls( urls ), mSubJobCount( 0 )
{
}

void DavCollectionsMultiFetchJob::start()
{
  if ( mUrls.isEmpty() )
    emitResult();

  foreach ( const DavUtils::DavUrl &url, mUrls ) {
    DavCollectionsFetchJob *job = new DavCollectionsFetchJob( url, this );
    connect( job, SIGNAL(result(KJob*)), SLOT(davJobFinished(KJob*)) );
    connect( job, SIGNAL(collectionDiscovered(int,QString,QString)),
             SIGNAL(collectionDiscovered(int,QString,QString)) );
    job->start();

    ++mSubJobCount;
  }
}

DavCollection::List DavCollectionsMultiFetchJob::collections() const
{
  return mCollections;
}

DavUtils::DavUrl::List DavCollectionsMultiFetchJob::urlsWithTemporaryError() const
{
  return mUrlsWithTemporaryError;
}

void DavCollectionsMultiFetchJob::davJobFinished( KJob *job )
{
  --mSubJobCount;
  DavCollectionsFetchJob *fetchJob = qobject_cast<DavCollectionsFetchJob*>( job );
  if ( fetchJob->hasTemporaryError() )
    mUrlsWithTemporaryError << fetchJob->davUrl();

  if ( job->error() ) {
    if ( !mSubJobSuccessful ) {
      setError( job->error() );
      setErrorText( job->errorText() );
    }
    if ( mSubJobCount == 0 )
      emitResult();
    return;
  }

  if ( !mSubJobSuccessful ) {
    setError( 0 ); // nope, everything went fine
    mSubJobSuccessful = true;
  }

  mCollections << fetchJob->collections();

  if ( mSubJobCount == 0 )
    emitResult();
}

#include "davcollectionsmultifetchjob.moc"
