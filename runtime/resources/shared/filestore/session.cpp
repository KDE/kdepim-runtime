/*  This file is part of the KDE project
    Copyright (C) 2009 Kevin Krammer <kevin.krammer@gmx.at>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "session_p.h"

#include "collectioncreatejob.h"
#include "collectiondeletejob.h"
#include "collectionfetchjob.h"
#include "collectionmodifyjob.h"
#include "collectionmovejob.h"
#include "itemcreatejob.h"
#include "itemfetchjob.h"
#include "itemmodifyjob.h"
#include "itemmovejob.h"
#include "storecompactjob.h"

using namespace Akonadi;

FileStore::AbstractJobSession::AbstractJobSession( QObject *parent )
  : QObject( parent )
{
}

FileStore::AbstractJobSession::~AbstractJobSession()
{
}

void FileStore::AbstractJobSession::notifyCollectionsReceived( FileStore::Job* job, const Collection::List &collections )
{
  FileStore::CollectionFetchJob *fetchJob = dynamic_cast<FileStore::CollectionFetchJob*>( job );
  if ( fetchJob != 0 ) {
    fetchJob->handleCollectionsReceived( collections );
  }
}

void FileStore::AbstractJobSession::notifyCollectionCreated( FileStore::Job *job, const Collection &collection )
{
  FileStore::CollectionCreateJob *createJob = dynamic_cast<FileStore::CollectionCreateJob*>( job );
  if ( createJob != 0 ) {
    createJob->handleCollectionCreated( collection );
  }
}

void FileStore::AbstractJobSession::notifyCollectionDeleted( FileStore::Job *job, const Collection &collection )
{
  FileStore::CollectionDeleteJob *deleteJob = dynamic_cast<FileStore::CollectionDeleteJob*>( job );
  if ( deleteJob != 0 ) {
    deleteJob->handleCollectionDeleted( collection );
  }
}

void FileStore::AbstractJobSession::notifyCollectionModified( FileStore::Job *job, const Collection &collection )
{
  FileStore::CollectionModifyJob *modifyJob = dynamic_cast<FileStore::CollectionModifyJob*>( job );
  if ( modifyJob != 0 ) {
    modifyJob->handleCollectionModified( collection );
  }
}

void FileStore::AbstractJobSession::notifyCollectionMoved( FileStore::Job *job, const Collection &collection )
{
  FileStore::CollectionMoveJob *moveJob = dynamic_cast<FileStore::CollectionMoveJob*>( job );
  if ( moveJob != 0 ) {
    moveJob->handleCollectionMoved( collection );
  }
}

void FileStore::AbstractJobSession::notifyItemsReceived( FileStore::Job* job, const Item::List &items )
{
  FileStore::ItemFetchJob *fetchJob = dynamic_cast<FileStore::ItemFetchJob*>( job );
  if ( fetchJob != 0 ) {
    fetchJob->handleItemsReceived( items );
  }
}

void FileStore::AbstractJobSession::notifyItemCreated( FileStore::Job *job, const Item &item )
{
  FileStore::ItemCreateJob *createJob = dynamic_cast<FileStore::ItemCreateJob*>( job );
  if ( createJob != 0 ) {
    createJob->handleItemCreated( item );
  }
}

void FileStore::AbstractJobSession::notifyItemModified( FileStore::Job *job, const Item &item )
{
  FileStore::ItemModifyJob *modifyJob = dynamic_cast<FileStore::ItemModifyJob*>( job );
  if ( modifyJob != 0 ) {
    modifyJob->handleItemModified( item );
  }
}

void FileStore::AbstractJobSession::notifyItemMoved( FileStore::Job *job, const Item &item )
{
  FileStore::ItemMoveJob *moveJob = dynamic_cast<FileStore::ItemMoveJob*>( job );
  if ( moveJob != 0 ) {
    moveJob->handleItemMoved( item );
  }
}

void FileStore::AbstractJobSession::notifyCollectionsChanged( FileStore::Job *job, const Collection::List &collections )
{
  FileStore::StoreCompactJob *compactJob = dynamic_cast<FileStore::StoreCompactJob*>( job );
  if ( compactJob != 0 ) {
    compactJob->handleCollectionsChanged( collections );
  }
}

void FileStore::AbstractJobSession::notifyItemsChanged( FileStore::Job *job, const Item::List &items )
{
  FileStore::StoreCompactJob *compactJob = dynamic_cast<FileStore::StoreCompactJob*>( job );
  if ( compactJob != 0 ) {
    compactJob->handleItemsChanged( items );
  }
}

void FileStore::AbstractJobSession::setError( FileStore::Job *job, int errorCode, const QString& errorText )
{
  job->setError( errorCode );
  job->setErrorText( errorText );
}

void FileStore::AbstractJobSession::emitResult( FileStore::Job *job )
{
  removeJob( job );

  job->emitResult();
}

#include "session_p.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
