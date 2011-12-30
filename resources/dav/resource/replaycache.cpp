/*
 *  Copyright (c) 2012 Grégory Oestreicher <greg@kamago.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "replaycache.h"
#include "davitem.h"
#include "davitemcreatejob.h"
#include "davitemdeletejob.h"
#include "davitemmodifyjob.h"
#include "davutils.h"
#include "settings.h"

#include <Akonadi/Collection>
#include <Akonadi/Item>
#include <Akonadi/ItemCreateJob>
#include <Akonadi/ItemDeleteJob>
#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemFetchScope>
#include <Akonadi/ItemModifyJob>
#include <kdebug.h>

ReplayCache::ReplayCache()
{
}

void ReplayCache::addReplayEntry( const QString &collectionUrl, ReplayCache::ReplayType type, const Akonadi::Item &item )
{
  if ( !mReplayEntries.contains( collectionUrl ) ) {
    mReplayEntries[collectionUrl] = ReplayEntry::List();
  }

  ReplayEntry entry;
  entry.type = type;
  entry.id = item.id();
  if ( !item.remoteId().isEmpty() )
    entry.url = item.remoteId();
  if ( !item.remoteRevision().isEmpty() )
    entry.etag = item.remoteRevision();

  mReplayEntries[collectionUrl] << entry;
}

void ReplayCache::delReplayEntry( const QString &collectionUrl, Akonadi::Item::Id id )
{
  if ( !mReplayEntries.contains( collectionUrl ) )
    return;

  int index = 0;
  foreach ( const ReplayEntry &entry, mReplayEntries[collectionUrl] ) {
    if ( entry.id == id )
      break;
    ++index;
  }
  mReplayEntries[collectionUrl].removeAt( index );
}

bool ReplayCache::hasReplayEntries( const QString &collectionUrl ) const
{
  return mReplayEntries.contains( collectionUrl ) && !mReplayEntries[collectionUrl].isEmpty();
}

ReplayCache::ReplayEntry::List ReplayCache::replayEntries( const QString &collectionUrl ) const
{
  if ( mReplayEntries.contains( collectionUrl ) )
    return mReplayEntries[collectionUrl];
  else
    return ReplayEntry::List();
}

void ReplayCache::flush( const QString &collectionUrl )
{
  ReplayEntry::List entries = replayEntries( collectionUrl );
  if ( entries.isEmpty() )
    return;

  foreach ( const ReplayEntry &entry, entries ) {
    if ( entry.type == ReplayCache::ItemAdded || entry.type == ReplayCache::ItemChanged ) {
      // Here we have to fetch the item from the backend
      Akonadi::Item item( entry.id );
      Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob( item );
      job->fetchScope().fetchFullPayload();
      job->fetchScope().setAncestorRetrieval( Akonadi::ItemFetchScope::Parent );

      job->setProperty( "replayCollectionRemoteId", QVariant::fromValue( collectionUrl ) );
      job->setProperty( "replayItemId", QVariant::fromValue( entry.id ) );

      if ( entry.type == ReplayCache::ItemAdded )
        job->setProperty( "replayType", QVariant::fromValue( QString( "add" ) ) );
      else
        job->setProperty( "replayType", QVariant::fromValue( QString( "change" ) ) );

      connect( job, SIGNAL(result(KJob*)), this, SLOT(onItemFetchFinished(KJob*)) );
      job->start();
    }
    else if ( entry.type == ReplayCache::ItemRemoved ) {
      // Easiest case, we just have to create the appropriate job
      const DavUtils::DavUrl davUrl = Settings::self()->davUrlFromCollectionUrl( collectionUrl, entry.url );
      DavItem davItem;
      davItem.setUrl( entry.url );
      davItem.setEtag( entry.etag );

      DavItemDeleteJob *job = new DavItemDeleteJob( davUrl, davItem );
      job->setProperty( "replayCollectionRemoteId", QVariant::fromValue( collectionUrl ) );
      job->setProperty( "replayItemId", QVariant::fromValue( entry.id ) );
      connect( job, SIGNAL(result(KJob*)), SLOT(onItemDeleteFinished(KJob*)) );
      job->start();
    }
    else {
      kError() << "Unknown replay type for item id" << entry.id;
      delReplayEntry( collectionUrl, entry.id );
    }
  }
}

void ReplayCache::onItemFetchFinished( KJob *job )
{
  if ( job->error() )
    return; // We'll just retry later

  Akonadi::ItemFetchJob *fetchJob = qobject_cast<Akonadi::ItemFetchJob*>( job );
  QString collectionUrl = job->property( "replayCollectionRemoteId" ).toString();
  Akonadi::Item::Id itemId = job->property( "replayItemId" ).toLongLong();

  const Akonadi::Item::List items = fetchJob->items();
  if ( items.isEmpty() ) {
    // The item may have been deleted, just remove this from the replay cache
    delReplayEntry( collectionUrl, itemId );
    return;
  }

  Akonadi::Item item = items.at( 0 );
  DavItem davItem = DavUtils::createDavItem( item, item.parentCollection() );
  if ( davItem.data().isEmpty() ) {
    // Invalid payload was returned, let's forget about this item
    delReplayEntry( collectionUrl, itemId );
    return;
  }

  QString urlStr = davItem.url();
  const DavUtils::DavUrl davUrl = Settings::self()->davUrlFromCollectionUrl( collectionUrl, urlStr );

  QString type = job->property( "replayType" ).toString();
  if ( type == QLatin1String( "add" ) ) {
    DavItemCreateJob *createJob = new DavItemCreateJob( davUrl, davItem );
    createJob->setProperty( "item", QVariant::fromValue( item ) );
    createJob->setProperty( "replayCollectionRemoteId", QVariant::fromValue( collectionUrl ) );
    createJob->setProperty( "replayItemId", QVariant::fromValue( itemId ) );
    connect( createJob, SIGNAL(result(KJob*)), this, SLOT(onItemAddFinished(KJob*)) );
    createJob->start();
  }
  else {
    DavItemModifyJob *modifyJob = new DavItemModifyJob( davUrl, davItem );
    modifyJob->setProperty( "item", QVariant::fromValue( item ) );
    modifyJob->setProperty( "replayCollectionRemoteId", QVariant::fromValue( collectionUrl ) );
    modifyJob->setProperty( "replayItemId", QVariant::fromValue( itemId ) );
    connect( modifyJob, SIGNAL(result(KJob*)), this, SLOT(onItemChangeFinished(KJob*)) );
    modifyJob->start();
  }
}

void ReplayCache::onItemAddFinished( KJob *job )
{
  if ( !job->error() ) {
    DavItemCreateJob *createJob = qobject_cast<DavItemCreateJob*>( job );
    const DavItem davItem = createJob->item();
    Akonadi::Item item = createJob->property( "item" ).value<Akonadi::Item>();
    Akonadi::Item newItem( item );
    newItem.setRemoteRevision( davItem.etag() );
    newItem.setRemoteId( davItem.url() );
    newItem.setMimeType( davItem.contentType() );
    emit etagChanged( item.remoteId(), davItem.etag() );

    // We just can't use an ItemModifyJob here, so we create a new item
    // and delete the old one
    Akonadi::ItemCreateJob *cJob = new Akonadi::ItemCreateJob( newItem, item.parentCollection() );
    cJob->start();

    Akonadi::Item::Id itemId = job->property( "replayItemId" ).toLongLong();
    Akonadi::ItemDeleteJob *dJob = new Akonadi::ItemDeleteJob( Akonadi::Item( itemId ) );
    dJob->start();

    QString collectionRemoteId = createJob->property( "replayCollectionRemoteId" ).toString();
    delReplayEntry( collectionRemoteId, item.id() );
  }
}

void ReplayCache::onItemChangeFinished( KJob *job )
{
  if ( !job->error() ) {
    DavItemModifyJob *modJob = qobject_cast<DavItemModifyJob*>( job );
    const DavItem davItem = modJob->item();
    Akonadi::Item item = modJob->property( "item" ).value<Akonadi::Item>();
    item.setRemoteRevision( davItem.etag() );
    item.setRemoteId( davItem.url() );
    emit etagChanged( item.remoteId(), davItem.etag() );
    Akonadi::ItemModifyJob *modifyJob = new Akonadi::ItemModifyJob( item );
    modifyJob->start();

    QString collectionId = job->property( "replayCollectionRemoteId" ).toString();
    delReplayEntry( collectionId, item.id() );
  }
}

void ReplayCache::onItemDeleteFinished( KJob *job )
{
  if ( !job->error() ) {
    QString collectionUrl = job->property( "replayCollectionRemoteId" ).toString();
    Akonadi::Item::Id itemId = job->property( "replayItemId" ).toLongLong();
    delReplayEntry( collectionUrl, itemId );
  }
}

#include "replaycache.moc"
