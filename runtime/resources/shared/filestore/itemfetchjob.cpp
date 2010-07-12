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

using namespace Akonadi;

class FileStore::ItemFetchJob::Private
{
  public:
    explicit Private( FileStore::ItemFetchJob *parent )
      : mParent( parent )
    {
    }

  public:
    ItemFetchScope mFetchScope;

    Item::List mItems;

    Collection mCollection;
    Item mItem;

  private:
    FileStore::ItemFetchJob *mParent;
};

FileStore::ItemFetchJob::ItemFetchJob( const Collection &collection, FileStore::AbstractJobSession *session )
  : FileStore::Job( session ), d( new Private( this ) )
{
  d->mCollection = collection;

  session->addJob( this );
}

FileStore::ItemFetchJob::ItemFetchJob( const Item &item, FileStore::AbstractJobSession *session )
  : FileStore::Job( session ), d( new Private( this ) )
{
  d->mItem = item;

  session->addJob( this );
}

FileStore::ItemFetchJob::~ItemFetchJob()
{
  delete d;
}

Collection FileStore::ItemFetchJob::collection() const
{
  return d->mCollection;
}

Item FileStore::ItemFetchJob::item() const
{
  return d->mItem;
}

void FileStore::ItemFetchJob::setFetchScope( const ItemFetchScope &fetchScope )
{
  d->mFetchScope = fetchScope;
}

ItemFetchScope &FileStore::ItemFetchJob::fetchScope()
{
  return d->mFetchScope;
}

Item::List FileStore::ItemFetchJob::items() const
{
  return d->mItems;
}

bool FileStore::ItemFetchJob::accept( FileStore::Job::Visitor *visitor )
{
  return visitor->visit( this );
}

void FileStore::ItemFetchJob::handleItemsReceived( const Item::List &items )
{
  d->mItems << items;

  emit itemsReceived( items );
}

#include "itemfetchjob.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
