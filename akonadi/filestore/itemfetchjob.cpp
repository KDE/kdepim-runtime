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

#include "itemfetchjob.h"

#include "session_p.h"

#include <akonadi/itemfetchscope.h>

using namespace Akonadi::FileStore;

class ItemFetchJob::ItemFetchJob::Private
{
  public:
    explicit Private( ItemFetchJob *parent )
      : mParent( parent )
    {
    }

  public:
    ItemFetchScope mFetchScope;

    Item::List mItems;

    Akonadi::Collection mCollection;
    Akonadi::Item mItem;

  private:
    ItemFetchJob *mParent;
};

ItemFetchJob::ItemFetchJob( const Akonadi::Collection &collection, AbstractJobSession *session )
  : Job( session ), d( new Private( this ) )
{
  d->mCollection = collection;

  session->addJob( this );
}

ItemFetchJob::ItemFetchJob( const Akonadi::Item &item, AbstractJobSession *session )
  : Job( session ), d( new Private( this ) )
{
  d->mItem = item;

  session->addJob( this );
}

ItemFetchJob::~ItemFetchJob()
{
  delete d;
}

Akonadi::Collection ItemFetchJob::collection() const
{
  return d->mCollection;
}

Akonadi::Item ItemFetchJob::item() const
{
  return d->mItem;
}

void ItemFetchJob::setFetchScope( const Akonadi::ItemFetchScope &fetchScope )
{
  d->mFetchScope = fetchScope;
}

Akonadi::ItemFetchScope &ItemFetchJob::fetchScope()
{
  return d->mFetchScope;
}

Akonadi::Item::List ItemFetchJob::items() const
{
  return d->mItems;
}

bool ItemFetchJob::accept( Visitor *visitor )
{
  return visitor->visit( this );
}

void ItemFetchJob::handleItemsReceived( const Akonadi::Item::List &items )
{
  d->mItems << items;

  emit itemsReceived( items );
}

#include "itemfetchjob.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
