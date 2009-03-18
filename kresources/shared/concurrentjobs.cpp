/*
    This file is part of kdepim.
    Copyright (c) 2009 Kevin Krammer <kevin.krammer@gmx.at>

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

#include "concurrentjobs.h"

#include "itemsavecontext.h"

#include <akonadi/itemfetchscope.h>

using namespace Akonadi;

const ConcurrentCollectionFetchJob *ConcurrentCollectionFetchJob::operator->() const
{
  return this;
}

Collection::List ConcurrentCollectionFetchJob::collections() const
{
  return mCollections;
}

void ConcurrentCollectionFetchJob::createJob()
{
  mJob = new CollectionFetchJob( Collection::root(), CollectionFetchJob::Recursive );
}

void ConcurrentCollectionFetchJob::handleSuccess()
{
  mCollections = mJob->collections();
}

ConcurrentItemFetchJob::ConcurrentItemFetchJob( const Collection &collection )
  : ConcurrentJob<ItemFetchJob>(),
    mCollection( collection )
{
}

const ConcurrentItemFetchJob *ConcurrentItemFetchJob::operator->() const
{
  return this;
}

Item::List ConcurrentItemFetchJob::items() const
{
  return mItems;
}

void ConcurrentItemFetchJob::createJob()
{
  mJob = new ItemFetchJob( mCollection );
  mJob->fetchScope().fetchFullPayload();
}

void ConcurrentItemFetchJob::handleSuccess()
{
  mItems = mJob->items();
}

ConcurrentItemSaveJob::ConcurrentItemSaveJob( const ItemSaveContext &saveContext )
  : ConcurrentJob<ItemSaveJob>(),
    mSaveContext( saveContext )
{
}

const ConcurrentItemSaveJob *ConcurrentItemSaveJob::operator->() const
{
 return this;
}

void ConcurrentItemSaveJob::createJob()
{
  mJob = new ItemSaveJob( mSaveContext );
}

void ConcurrentItemSaveJob::handleSuccess()
{
}

ConcurrentCollectionCreateJob::ConcurrentCollectionCreateJob( const Akonadi::Collection &collection )
  : ConcurrentJob<CollectionCreateJob>(),
    mCollection( collection )
{
}

const ConcurrentCollectionCreateJob *ConcurrentCollectionCreateJob::operator->() const
{
  return this;
}

void ConcurrentCollectionCreateJob::createJob()
{
  mJob = new CollectionCreateJob( mCollection );
}

void ConcurrentCollectionCreateJob::handleSuccess()
{
}

ConcurrentCollectionDeleteJob::ConcurrentCollectionDeleteJob( const Akonadi::Collection &collection )
  : ConcurrentJob<CollectionDeleteJob>(),
    mCollection( collection )
{
}

const ConcurrentCollectionDeleteJob *ConcurrentCollectionDeleteJob::operator->() const
{
  return this;
}

void ConcurrentCollectionDeleteJob::createJob()
{
  mJob = new CollectionDeleteJob( mCollection );
}

void ConcurrentCollectionDeleteJob::handleSuccess()
{
}

// kate: space-indent on; indent-width 2; replace-tabs on;
