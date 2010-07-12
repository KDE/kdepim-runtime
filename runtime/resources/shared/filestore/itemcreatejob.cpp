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

#include "itemcreatejob.h"

#include "session_p.h"

using namespace Akonadi;

class FileStore::ItemCreateJob::Private
{
  public:
    explicit Private( FileStore::ItemCreateJob *parent )
      : mParent( parent )
    {
    }

  public:
    Item mItem;
    Collection mCollection;

  private:
    FileStore::ItemCreateJob *mParent;
};

FileStore::ItemCreateJob::ItemCreateJob( const Item &item, const Collection &collection, FileStore::AbstractJobSession *session )
  : FileStore::Job( session ), d( new Private( this ) )
{
  d->mItem = item;
  d->mCollection = collection;

  session->addJob( this );
}

FileStore::ItemCreateJob::~ItemCreateJob()
{
  delete d;
}

Collection FileStore::ItemCreateJob::collection() const
{
  return d->mCollection;
}

Item FileStore::ItemCreateJob::item() const
{
  return d->mItem;
}

bool FileStore::ItemCreateJob::accept( FileStore::Job::Visitor *visitor )
{
  return visitor->visit( this );
}

void FileStore::ItemCreateJob::handleItemCreated( const Item &item )
{
  d->mItem = item;
}

#include "itemcreatejob.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
