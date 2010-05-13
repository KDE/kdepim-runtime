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

using namespace Akonadi::FileStore;

AbstractJobSession::AbstractJobSession( QObject *parent )
  : QObject( parent )
{
}

AbstractJobSession::~AbstractJobSession()
{
}

void AbstractJobSession::notifyCollectionsReceived( Job* job, const Collection::List &collections )
{
  CollectionFetchJob *fetchJob = dynamic_cast<CollectionFetchJob*>( job );
  if ( fetchJob != 0 ){
    fetchJob->handleCollectionsReceived( collections );
  }
}

void AbstractJobSession::notifyCollectionCreated( Job *job, const Collection &collection )
{
  CollectionCreateJob *createJob = dynamic_cast<CollectionCreateJob*>( job );
  if ( createJob != 0 ) {
    createJob->handleCollectionCreated( collection );
  }
}

void AbstractJobSession::notifyCollectionDeleted( Job *job, const Collection &collection )
{
  CollectionDeleteJob *deleteJob = dynamic_cast<CollectionDeleteJob*>( job );
  if ( deleteJob != 0 ) {
    deleteJob->handleCollectionDeleted( collection );
  }
}

void AbstractJobSession::notifyCollectionModified( Job *job, const Collection &collection )
{
  CollectionModifyJob *modifyJob = dynamic_cast<CollectionModifyJob*>( job );
  if ( modifyJob != 0 ) {
    modifyJob->handleCollectionModified( collection );
  }
}

void AbstractJobSession::notifyCollectionMoved( Job *job, const Collection &collection )
{
  CollectionMoveJob *moveJob = dynamic_cast<CollectionMoveJob*>( job );
  if ( moveJob != 0 ) {
    moveJob->handleCollectionMoved( collection );
  }
}

void AbstractJobSession::notifyItemsReceived( Job* job, const Item::List &items )
{
  ItemFetchJob *fetchJob = dynamic_cast<ItemFetchJob*>( job );
  if ( fetchJob != 0 ){
    fetchJob->handleItemsReceived( items );
  }
}

void AbstractJobSession::notifyItemCreated( Job *job, const Item &item )
{
  ItemCreateJob *createJob = dynamic_cast<ItemCreateJob*>( job );
  if ( createJob != 0 ){
    createJob->handleItemCreated( item );
  }
}

void AbstractJobSession::notifyItemModified( Job *job, const Item &item )
{
  ItemModifyJob *modifyJob = dynamic_cast<ItemModifyJob*>( job );
  if ( modifyJob != 0 ){
    modifyJob->handleItemModified( item );
  }
}

void AbstractJobSession::setError( Job *job, int errorCode, const QString& errorText )
{
  job->setError( errorCode );
  job->setErrorText( errorText );
}

void AbstractJobSession::emitResult( Job *job )
{
  removeJob( job );

  job->emitResult();
}

#include "session_p.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
