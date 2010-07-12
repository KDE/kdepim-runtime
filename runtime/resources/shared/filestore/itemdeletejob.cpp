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

#include "itemdeletejob.h"

#include "session_p.h"

using namespace Akonadi;

class FileStore::ItemDeleteJob::Private
{
  public:
    explicit Private( FileStore::ItemDeleteJob *parent )
      : mParent( parent )
    {
    }

  public:
    Item mItem;

  private:
    FileStore::ItemDeleteJob *mParent;
};

FileStore::ItemDeleteJob::ItemDeleteJob( const Item &item, FileStore::AbstractJobSession *session )
  : FileStore::Job( session ), d( new Private( this ) )
{
  d->mItem = item;

  session->addJob( this );
}

FileStore::ItemDeleteJob::~ItemDeleteJob()
{
  delete d;
}

Item FileStore::ItemDeleteJob::item() const
{
  return d->mItem;
}

bool FileStore::ItemDeleteJob::accept( FileStore::Job::Visitor *visitor )
{
  return visitor->visit( this );
}

void FileStore::ItemDeleteJob::handleItemDeleted( const Item &item )
{
  d->mItem = item;
}

#include "itemdeletejob.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
