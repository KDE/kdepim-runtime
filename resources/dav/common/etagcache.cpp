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

#include "etagcache.h"

#include <akonadi/collection.h>
#include <akonadi/item.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>
#include <kdebug.h>
#include <kjob.h>

EtagCache::EtagCache()
{
}

void EtagCache::sync( const Akonadi::Collection &collection )
{
  Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob( collection );
  job->fetchScope().fetchFullPayload( false ); // We only need the remote id and the revision
  connect( job, SIGNAL(result(KJob*)), this, SLOT(onItemFetchJobFinished(KJob*)) );
  job->start();
}

void EtagCache::setEtag( const QString &remoteId, const QString &etag )
{
  mCache[ remoteId ] = etag;

  if ( mChangedRemoteIds.contains( remoteId ) )
    mChangedRemoteIds.remove( remoteId );
}

bool EtagCache::etagChanged( const QString &remoteId, const QString &refEtag )
{
  const bool changed = (mCache.value( remoteId ) != refEtag);

  if ( changed )
    mChangedRemoteIds.insert( remoteId );

  return changed;
}

bool EtagCache::isOutOfDate( const QString &remoteId ) const
{
  return mChangedRemoteIds.contains( remoteId );
}

QStringList EtagCache::changedRemoteIds() const
{
  return mChangedRemoteIds.toList();
}

void EtagCache::onItemFetchJobFinished( KJob *job )
{
  if ( job->error() )
    return;

  const Akonadi::ItemFetchJob *fetchJob = qobject_cast<Akonadi::ItemFetchJob*>( job );
  const Akonadi::Item::List items = fetchJob->items();

  foreach ( const Akonadi::Item &item, items )
    setEtag( item.remoteId(), item.remoteRevision() );
}

#include "etagcache.moc"
