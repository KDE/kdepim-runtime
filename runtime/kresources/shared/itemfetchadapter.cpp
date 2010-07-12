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

#include "itemfetchadapter.h"

#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>

using namespace Akonadi;

ItemFetchAdapter::ItemFetchAdapter( const Collection &collection, QObject *parent )
  : QObject( parent ), mCollection( collection )
{
  ItemFetchJob *job = new ItemFetchJob( mCollection, this );
  job->fetchScope().fetchFullPayload();

  connect( job, SIGNAL( itemsReceived( Akonadi::Item::List ) ),
           SLOT( jobItemsReceived( Akonadi::Item::List ) ) ),
  connect( job,  SIGNAL( result( KJob* ) ),
           SLOT ( jobResult( KJob* ) ) );

  connect( this, SIGNAL( itemsReceived( Akonadi::Collection, Akonadi::Item::List ) ),
           parent, SLOT( asyncItemsReceived( Akonadi::Collection, Akonadi::Item::List ) ) );

  connect( this, SIGNAL( result( ItemFetchAdapter*, KJob* ) ),
           parent, SLOT( asyncItemsResult( ItemFetchAdapter*, KJob* ) ) );
}

Collection ItemFetchAdapter::collection() const
{
  return mCollection;
}

void ItemFetchAdapter::jobItemsReceived( const Item::List &items )
{
  emit itemsReceived( mCollection, items );
}

void ItemFetchAdapter::jobResult( KJob *job )
{
  emit result( this, job );
}

#include "itemfetchadapter.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
